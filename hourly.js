// Initialize hourly data retrieval
function fetchHourlyData() {
    fetch("https://irrigaia.site/getData.php", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify({
            range: "hourly",
            table: "drip_data"
        })
    })
    .then(response => response.json())
    .then(dataDrip => {
        if (dataDrip.status === "success") {
            fetch("https://irrigaia.site/getData.php", {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    range: "hourly",
                    table: "mist_data"
                })
            })
            .then(response => response.json())
            .then(dataMist => {
                if (dataMist.status === "success") {
                    processHourlyData(dataDrip.data, dataMist.data);
                } else {
                    console.error("Failed to fetch mist data.");
                }
            })
            .catch(error => console.error("AJAX Error (Mist Data):", error));
        } else {
            console.error("Failed to fetch drip data.");
        }
    })
    .catch(error => console.error("AJAX Error (Drip Data):", error));
}

// Trim decimals without rounding
function trimDecimals(val, maxDecimals = 4) {
    val = parseFloat(val);
    const str = val.toString();
    if (str.includes(".")) {
        const [intPart, decPart] = str.split(".");
        return `${intPart}.${decPart.substring(0, maxDecimals)}`;
    }
    return str; // it's a whole number
}

// Format functions for water and energy
function formatWater(val) {
    return trimDecimals(val, 4);
}

function formatEnergy(val) {
    return trimDecimals(val, 4);
}

// Process and update UI with the fetched data
function processHourlyData(dripData, mistData) {
    let labels = [];
    let waterData = { mist: [], drip: [] };
    let energyData = { mist: [], drip: [] };

    dripData.forEach((entry, index) => {
        labels.push(entry.formatted_timestamp.substring(11, 16)); // Extract HH:MM format
        waterData.drip.push(parseFloat(entry.water_consumption));
        energyData.drip.push(parseFloat(entry.energy_consumption));
    });

    mistData.forEach((entry, index) => {
        if (index < labels.length) {
            waterData.mist.push(parseFloat(entry.water_consumption));
            energyData.mist.push(parseFloat(entry.energy_consumption));
        }
    });

    // Update Charts
    initializeCharts(labels, waterData, energyData);

    // Format data for tables
    const tableData = labels.map((label, index) => ({
        label,
        water: {
            mist: waterData.mist[index] || 0,
            drip: waterData.drip[index] || 0,
        },
        energy: {
            mist: energyData.mist[index] || 0,
            drip: energyData.drip[index] || 0,
        },
    }));

    // Populate Tables
    populateTables(tableData);
}

// Populate the tables with updated data
function populateTables(tableData) {
    const waterTableBody = document.getElementById("waterTableBody");
    const energyTableBody = document.getElementById("energyTableBody");

    // Clear existing rows
    waterTableBody.innerHTML = "";
    energyTableBody.innerHTML = "";

    tableData.forEach((data) => {
        // Water Data Row
        const waterRow = document.createElement("tr");
        waterRow.innerHTML = `
            <td>${data.label}</td>
            <td>${formatWater(data.water.mist)} L</td>
            <td>${formatWater(data.water.drip)} L</td>
        `;
        waterTableBody.appendChild(waterRow);

        // Energy Data Row
        const energyRow = document.createElement("tr");
        energyRow.innerHTML = `
            <td>${data.label}</td>
            <td>${formatEnergy(data.energy.mist)} kWh</td>
            <td>${formatEnergy(data.energy.drip)} kWh</td>
        `;
        energyTableBody.appendChild(energyRow);
    });
}

// Initialize and refresh every 2 minutes
setInterval(fetchHourlyData, 2 * 60 * 1000);
fetchHourlyData();
