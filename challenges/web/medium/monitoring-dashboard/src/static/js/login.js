// Login form handling
document.addEventListener('DOMContentLoaded', function() {
    const loginForm = document.getElementById('loginForm');
    const errorMessage = document.getElementById('error-message');

    // Check if user is already logged in
    checkExistingSession();

    loginForm.addEventListener('submit', async function(e) {
        e.preventDefault();

        const username = document.getElementById('username').value;
        const password = document.getElementById('password').value;

        hideError();

        try {
            const response = await fetch('/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    username: username,
                    password: password
                }),
            });

            const data = await response.json();

            if (data.success) {
                // Store user info in localStorage for dashboard
                localStorage.setItem('userInfo', JSON.stringify({
                    username: data.username
                }));

                // Redirect to dashboard
                window.location.href = '/static/dashboard.html';
            } else {
                showError(data.message || 'Login failed');
            }
        } catch (error) {
            console.error('Login error:', error);
            showError('Network error. Please try again.');
        }
    });

    function showError(message) {
        errorMessage.textContent = message;
        errorMessage.classList.remove('hidden');
    }

    function hideError() {
        errorMessage.classList.add('hidden');
    }

    async function checkExistingSession() {
        try {
            // Check if user has a valid session using the /me endpoint
            const response = await fetch('/me');

            if (response.ok) {
                const data = await response.json();
                if (data.success && data.username) {
                    // Store user info in localStorage for dashboard
                    localStorage.setItem('userInfo', JSON.stringify({
                        username: data.username
                    }));

                    // User is already logged in, redirect to dashboard
                    window.location.href = '/static/dashboard.html';
                }
            }
        } catch (error) {
            // Session is not valid, stay on login page
            console.log('No valid session found');
        }
    }
});
