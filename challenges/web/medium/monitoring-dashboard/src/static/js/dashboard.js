// Dashboard functionality
document.addEventListener('DOMContentLoaded', function() {
    const userWelcome = document.getElementById('user-welcome');
    const logoutBtn = document.getElementById('logout-btn');
    const searchInput = document.getElementById('search-input');
    const searchBtn = document.getElementById('search-btn');
    const clearBtn = document.getElementById('clear-btn');
    const hostCount = document.getElementById('host-count');
    const loading = document.getElementById('loading');
    const errorMessage = document.getElementById('error-message');
    const hostsTable = document.getElementById('hosts-table');
    const hostsTbody = document.getElementById('hosts-tbody');

    let hosts = [];

    // Initialize dashboard
    init();

    async function init() {
        // Check if user is logged in
        const userInfo = localStorage.getItem('userInfo');
        if (!userInfo) {
            redirectToLogin();
            return;
        }

        const user = JSON.parse(userInfo);
        userWelcome.textContent = `Welcome, ${user.username}`;

        // Load hosts initially without filter
        await loadHosts();

        // Set up event listeners
        setupEventListeners();
    }

    function setupEventListeners() {
        logoutBtn.addEventListener('click', handleLogout);
        searchBtn.addEventListener('click', handleSearch);
        clearBtn.addEventListener('click', handleClear);
        searchInput.addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                handleSearch();
            }
        });
        // Remove the input event listener for real-time search
    }

    async function loadHosts(hostFilter = '') {
        showLoading();
        hideError();

        try {
            // Build URL with query parameters
            let url = '/icinga2/v1/objects/hosts';
            if (hostFilter) {
                url += `?host=${encodeURIComponent(hostFilter)}`;
            }

            const response = await fetch(url);

            if (response.status === 401) {
                redirectToLogin();
                return;
            }

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            const data = await response.json();

            if (data.error) {
                showError(`Error: ${data.status || 'Failed to load hosts'}`);
                hosts = [];
            } else {
                hosts = data.results || [];
            }

            displayHosts();
            updateHostCount();
        } catch (error) {
            console.error('Error loading hosts:', error);
            showError('Failed to load hosts. Please try again.');
            hosts = [];
            displayHosts();
            updateHostCount();
        } finally {
            hideLoading();
        }
    }

    function displayHosts() {
        hostsTbody.innerHTML = '';

        if (hosts.length === 0) {
            hostsTable.classList.add('hidden');
            return;
        }

        hosts.forEach(host => {
            const row = createHostRow(host);
            hostsTbody.appendChild(row);
        });

        hostsTable.classList.remove('hidden');
    }

    function createHostRow(host) {
        const row = document.createElement('tr');

        const name = host.name || 'Unknown';
        const address = host.attrs?.address || 'N/A';
        const groups = host.attrs?.groups ? host.attrs.groups.join(', ') : 'N/A';
        const state = getHostState(host.attrs?.state);
        const lastCheck = getLastCheck(host.attrs?.last_check);

        row.innerHTML = `
            <td>${escapeHtml(name)}</td>
            <td>${escapeHtml(address)}</td>
            <td>${escapeHtml(groups)}</td>
            <td><span class="host-state ${state.class}">${state.text}</span></td>
            <td>${escapeHtml(lastCheck)}</td>
        `;

        return row;
    }

    function getHostState(state) {
        switch (state) {
            case 0:
                return { text: 'UP', class: 'up' };
            case 1:
                return { text: 'DOWN', class: 'down' };
            case 2:
                return { text: 'UNREACHABLE', class: 'unreachable' };
            default:
                return { text: 'UNKNOWN', class: 'unreachable' };
        }
    }

    function getLastCheck(timestamp) {
        if (!timestamp) return 'Never';

        try {
            const date = new Date(timestamp * 1000);
            return date.toLocaleString();
        } catch (error) {
            return 'Invalid';
        }
    }

    async function handleSearch() {
        const query = searchInput.value.trim();

        // Make API call with the search query
        await loadHosts(query);
    }

    async function handleClear() {
        searchInput.value = '';

        // Load all hosts without filter
        await loadHosts();
    }

    function updateHostCount() {
        const count = hosts.length;

        hostCount.textContent = `${count} host${count !== 1 ? 's' : ''} found`;
    }

    async function handleLogout() {
        try {
            await fetch('/logout', {
                method: 'DELETE',
            });
        } catch (error) {
            console.error('Logout error:', error);
        } finally {
            localStorage.removeItem('userInfo');
            redirectToLogin();
        }
    }

    function redirectToLogin() {
        window.location.href = '/static/login.html';
    }

    function showLoading() {
        loading.classList.remove('hidden');
        hostsTable.classList.add('hidden');
    }

    function hideLoading() {
        loading.classList.add('hidden');
    }

    function showError(message) {
        errorMessage.textContent = message;
        errorMessage.classList.remove('hidden');
    }

    function hideError() {
        errorMessage.classList.add('hidden');
    }

    function escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }
});
