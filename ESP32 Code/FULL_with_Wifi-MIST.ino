    // 🧠 Includes and Definitions
    #include <WiFi.h>
    #include <HTTPClient.h>
    #include <PZEM004Tv30.h>
    #include <Preferences.h>
    #include <time.h>

    const char* ssid = "Trojan";
    const char* password = "Celestino2025";

    const char* phpURL = "https://irrigaia.site/esp-data.php";
    const char* systemType = "mist";

    #define FLOW_SENSOR_PIN 27
    #define SOIL_MOISTURE_PIN 34
    #define RELAY_SOLENOID 26
    #define RELAY_PUMP 25

    PZEM004Tv30 pzem(Serial2, 16, 17);

    volatile int flowCount = 0;
    float totalLiters = 0.0;
    float totalEnergy = 0.0;
    unsigned long lastMillis = 0;
    unsigned long lastFlowMillis = 0;

    Preferences prefs;
    int wateringCount = 0;
    bool wasWatering = false;
    String todayDate = "";
    String lastDateWatered = "";
    const int maxWateringPerDay = 7;

    // NTP
    const long gmtOffset_sec = 8 * 3600;
    const int daylightOffset_sec = 0;

    void setupTime() {
      configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov", "time.google.com");
      Serial.print("Syncing time");

      int retries = 0;
      while (time(nullptr) < 100000 && retries < 40) {
        Serial.print(".");
        delay(500);
        retries++;
      }

      if (time(nullptr) >= 100000) {
        Serial.println("\n✅ Time synced successfully!");
        struct tm timeinfo;
        getLocalTime(&timeinfo);
        char buffer[30];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.print("📅 Current DateTime: ");
        Serial.println(buffer);
      } else {
        Serial.println("\n❌ Failed to sync time. Device may not reset counter today.");
      }
    }

    String getCurrentDate() {
      struct tm timeinfo;
      if (!getLocalTime(&timeinfo)) return "unknown";
      char buffer[11];
      strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
      return String(buffer);
    }

    void setupWateringMemory() {
      prefs.begin(systemType, false);
      wateringCount = prefs.getInt("count", 0);
      lastDateWatered = prefs.getString("lastDate", "unknown");
      todayDate = getCurrentDate();

      if (todayDate != lastDateWatered && todayDate != "unknown") {
        wateringCount = 0;
        prefs.putInt("count", 0);
        prefs.putString("lastDate", todayDate);
        Serial.println("📅 New day detected at setup. Watering counter RESET.");
      } else {
        Serial.println("✅ Same day or unknown time. Current watering count: " + String(wateringCount));
      }
      prefs.end();
    }

    void IRAM_ATTR countPulse() {
      flowCount++;
    }

    void setup() {
      Serial.begin(115200);
      pinMode(RELAY_SOLENOID, OUTPUT);
      pinMode(RELAY_PUMP, OUTPUT);
      digitalWrite(RELAY_SOLENOID, LOW);
      digitalWrite(RELAY_PUMP, LOW);
      pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
      attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), countPulse, RISING);

      WiFi.begin(ssid, password);
      Serial.print("Connecting to WiFi");
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        Serial.print(".");
        delay(500);
        attempts++;
      }
      Serial.println(WiFi.status() == WL_CONNECTED ? "\n✅ WiFi connected." : "\n❌ WiFi failed.");

      setupTime();

      prefs.begin(systemType, false);
      if (!prefs.getBool("initialized", false)) {
        prefs.putInt("count", 0);
        prefs.putString("lastDate", getCurrentDate());
        prefs.putBool("initialized", true);
        Serial.println("🧼 Auto-reset: Fresh upload, count set to 0");
      }
      prefs.end();

      setupWateringMemory();
      lastMillis = millis();
    }

    void loop() {
      // 🧠 DAILY CHECK: Reset if New Day Detected (even if boot NTP failed)
      todayDate = getCurrentDate();
      prefs.begin(systemType, false);
      lastDateWatered = prefs.getString("lastDate", "unknown");
      if (todayDate != lastDateWatered && todayDate != "unknown") {
        wateringCount = 0;
        prefs.putInt("count", 0);
        prefs.putString("lastDate", todayDate);
        Serial.println("📅 New day detected during loop. Watering counter RESET.");
      }
      prefs.end();

      // 🌱 Soil Moisture Logic
      int rawSoil = analogRead(SOIL_MOISTURE_PIN);
      int soilMoisture = map(rawSoil, 4095, 0, 0, 100);
      Serial.print("🌱 Soil Moisture: "); Serial.println(soilMoisture);

      bool currentlyWatering = false;
      if (soilMoisture <= 55 && wateringCount < maxWateringPerDay) {
        digitalWrite(RELAY_SOLENOID, HIGH);
        digitalWrite(RELAY_PUMP, HIGH);
        currentlyWatering = true;
      } else if (soilMoisture >= 64 || wateringCount >= maxWateringPerDay) {
        digitalWrite(RELAY_SOLENOID, LOW);
        digitalWrite(RELAY_PUMP, LOW);
        currentlyWatering = false;
      }

      if (!wasWatering && currentlyWatering) {
        wateringCount++;
        prefs.begin(systemType, false);
        prefs.putInt("count", wateringCount);
        prefs.putString("lastDate", todayDate);
        prefs.end();
        Serial.println("💧 Watering STARTED (Count incremented)");
      }

      wasWatering = currentlyWatering;

      // 💧 Flow Calculation
      unsigned long currentMillis = millis();
      float durationSeconds = (currentMillis - lastFlowMillis) / 1000.0;
      lastFlowMillis = currentMillis;

      float flowLiters = (flowCount / 7.5) * (durationSeconds / 60.0);
      totalLiters += flowLiters;
      flowCount = 0;

      // ⚡ Manual Energy Calculation
    float voltage = pzem.voltage();
    float current = pzem.current();
    float power = voltage * current;
    float elapsedTime = (currentMillis - lastMillis) / (1000.0 * 3600.0); // in hours
    lastMillis = currentMillis;

    // Only add to totalEnergy if readings are valid (not NaN or INF)
    if (!isnan(power) && !isnan(elapsedTime) && power >= 0 && elapsedTime >= 0) {
      totalEnergy += (power * elapsedTime) / 1000.0;  // kWh = (W × h) / 1000
    }


      // 🧠 Debug Logs
      Serial.printf("🧠 Voltage: %.2f V\n", voltage);
      Serial.printf("🧠 Current: %.2f A\n", current);
      Serial.printf("🧠 Power: %.2f W\n", power);
      Serial.printf("💧 Cycle Water: %.2f L\n", flowLiters);
      Serial.printf("💧 Total: %.2f L | ⚡ Energy: %.4f kWh\n", totalLiters, totalEnergy);
      Serial.printf("🧠 Water Count: %d\n", wateringCount);
      Serial.println("----------------------------");

      // 📡 Send to PHP
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(phpURL);
        http.addHeader("Content-Type", "application/json");

        String payload = "{";
        payload += "\"device\":\"" + String(systemType) + "\","; 
        payload += "\"water\":" + String(totalLiters, 2) + ","; 
        payload += "\"energy\":" + String(totalEnergy, 4) + ","; 
        payload += "\"moisture\":" + String(soilMoisture);
        payload += "}";

        int responseCode = http.POST(payload);
        String response = http.getString();

        if (responseCode > 0) {
          Serial.printf("✅ Data sent. Server responded: %d\n", responseCode);
          Serial.println("📡 Server reply: " + response);
        } else {
          Serial.printf("❌ Failed to send data. Error: %s\n", http.errorToString(responseCode).c_str());
        }
        http.end();
      } else {
        Serial.println("⚠️ Skipped upload: No WiFi connection");
      }

      delay(5000); // adjust depending on traffic control
    }
