#!/bin/bash

# Generate MQTT password file
echo "Generating MQTT password file..."
mkdir -p /mosquitto/config /mosquitto/data /mosquitto/log
chmod 755 /mosquitto/config /mosquitto/data /mosquitto/log
mosquitto_passwd -c -b /mosquitto/config/passwd "$ADMIN_USERNAME" "$ADMIN_PASSWORD"
chmod 644 /mosquitto/config/passwd
chown root:root /mosquitto/config/passwd
# Make log directory writable
touch /mosquitto/log/mosquitto.log
chmod 666 /mosquitto/log/mosquitto.log
echo "MQTT password file generated"

# Generate htpasswd file for Apache
echo "Generating htpasswd file..."
mkdir -p /etc/apache2/conf/auth
htpasswd -cb /etc/apache2/conf/auth/.htpasswd "$CHARGER_USERNAME" "$CHARGER_PASSWORD"
echo "htpasswd file generated"

# Start supervisor in background
supervisord -c /etc/supervisor/conf.d/supervisord.conf &

# Wait a moment for supervisor to be ready
sleep 2

# Set proper permissions for mosquitto
chmod 666 /mosquitto/config/passwd

# Start services in order
echo "Starting mosquitto..."
supervisorctl -c /etc/supervisor/conf.d/supervisord.conf start mosquitto

# Wait for mosquitto to be ready
sleep 3

echo "Starting apache2..."
supervisorctl -c /etc/supervisor/conf.d/supervisord.conf start apache2

# Wait for apache to be ready  
sleep 2

echo "Starting client..."
supervisorctl -c /etc/supervisor/conf.d/supervisord.conf start client

echo "Starting credential-server..."
supervisorctl -c /etc/supervisor/conf.d/supervisord.conf start credential-server

# Keep the container running
wait