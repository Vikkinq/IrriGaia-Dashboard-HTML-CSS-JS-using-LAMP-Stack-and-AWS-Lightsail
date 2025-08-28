document.addEventListener('DOMContentLoaded', function() {
    const passcodeInput = document.getElementById('passcode');
    const togglePasswordButton = document.getElementById('toggle-password');
    const loginButton = document.getElementById('login-button');
    const errorMessage = document.getElementById('error-message');
    
    // Correct passcode
    const correctPasscode = '1234';
    
    // Toggle password visibility
    togglePasswordButton.addEventListener('click', function() {
        const type = passcodeInput.getAttribute('type') === 'password' ? 'text' : 'password';
        passcodeInput.setAttribute('type', type);
        
        // Toggle icon
        const icon = togglePasswordButton.querySelector('i');
        icon.classList.toggle('fa-eye');
        icon.classList.toggle('fa-eye-slash');
    });
    
    // Handle login button click
    loginButton.addEventListener('click', function() {
        validatePasscode();
    });
    
    // Handle Enter key press
    passcodeInput.addEventListener('keypress', function(e) {
        if (e.key === 'Enter') {
            validatePasscode();
        }
    });
    
    // Validate passcode
    function validatePasscode() {
        const passcode = passcodeInput.value;
        
        if (passcode === '') {
            showError('Please enter your passcode');
            return;
        }
        
        if (passcode === correctPasscode) {
            // Success
            showSuccess();
            
            // Redirect to dashboard after animation
            setTimeout(function() {
                window.location.href = 'dashboard.html';
            }, 1500);
        } else {
            // Error
            showError('Incorrect passcode. Please try again.');
            
            // Shake effect on input
            passcodeInput.classList.add('shake');
            setTimeout(function() {
                passcodeInput.classList.remove('shake');
            }, 500);
        }
    }
    
    // Show error message
    function showError(message) {
        errorMessage.textContent = message;
        errorMessage.style.opacity = '1';
        
        // Add error styling to input
        passcodeInput.style.borderColor = 'var(--error-color)';
        passcodeInput.style.boxShadow = '0 0 0 3px rgba(255, 82, 82, 0.2)';
    }
    
    // Show success animation
    function showSuccess() {
        // Hide form elements
        passcodeInput.style.display = 'none';
        togglePasswordButton.style.display = 'none';
        loginButton.style.display = 'none';
        errorMessage.style.display = 'none';
        
        // Create success message
        const successDiv = document.createElement('div');
        successDiv.className = 'success-message';
        successDiv.innerHTML = `
            <div class="success-checkmark">
                <div class="check-icon"></div>
            </div>
            <h3>Access Granted</h3>
            <p>Redirecting to dashboard...</p>
        `;
        
        // Add success message to form
        document.querySelector('.login-form').appendChild(successDiv);
        
        // Show checkmark animation
        document.querySelector('.success-checkmark').style.display = 'block';
        
        // Add success styles
        successDiv.style.textAlign = 'center';
        successDiv.style.color = 'var(--success-color)';
        successDiv.querySelector('h3').style.margin = '1rem 0';
        successDiv.querySelector('p').style.color = 'var(--text-light)';
        
        // Animate checkmark
        const checkIcon = document.querySelector('.check-icon');
        checkIcon.style.animation = 'scale-up 0.5s ease-in-out forwards';
    }
    
    // Add CSS for shake animation
    const style = document.createElement('style');
    style.textContent = `
        @keyframes shake {
            0%, 100% { transform: translateX(0); }
            10%, 30%, 50%, 70%, 90% { transform: translateX(-5px); }
            20%, 40%, 60%, 80% { transform: translateX(5px); }
        }
        
        .shake {
            animation: shake 0.5s cubic-bezier(0.36, 0.07, 0.19, 0.97) both;
        }
        
        @keyframes scale-up {
            0% { transform: scale(0); }
            100% { transform: scale(1); }
        }
        
        .success-message {
            animation: fade-in 0.5s ease-in-out forwards;
        }
        
        @keyframes fade-in {
            0% { opacity: 0; }
            100% { opacity: 1; }
        }
    `;
    document.head.appendChild(style);
});