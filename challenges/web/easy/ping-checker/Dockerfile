FROM ubuntu:22.04@sha256:4e0171b9275e12d375863f2b3ae9ce00a4c53ddda176bd55868df97ac6f21a6e

ENV DEBIAN_FRONTEND=noninteractive

# Install required packages
RUN apt-get update && apt-get install -y \
    apache2 \
    php \
    php-cli \
    iputils-ping \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# Enable Apache modules
RUN a2enmod rewrite

# Accept flag as build argument
ARG FLAG

# Setup flag in two parts with subtle discovery chain
RUN mkdir -p /var/backups /opt/logs /tmp/system \
    && echo "$FLAG" | cut -c1-25 > /tmp/flag_part1.tmp \
    && echo "$FLAG" | cut -c26- > /tmp/flag_part2.tmp \
    && echo "# System Configuration File" > /var/backups/system.conf \
    && echo "# Generated: $(date '+%Y-%m-%d %H:%M:%S')" >> /var/backups/system.conf \
    && echo "# ====================================" >> /var/backups/system.conf \
    && echo "" >> /var/backups/system.conf \
    && echo "# Network Diagnostics Configuration" >> /var/backups/system.conf \
    && echo "PING_COUNT=4" >> /var/backups/system.conf \
    && echo "TIMEOUT=5" >> /var/backups/system.conf \
    && echo "MAX_HOPS=30" >> /var/backups/system.conf \
    && echo "" >> /var/backups/system.conf \
    && echo "# Service Configuration" >> /var/backups/system.conf \
    && echo "SERVICE_TOKEN=$(cat /tmp/flag_part1.tmp)" >> /var/backups/system.conf \
    && echo "SERVICE_MODE=production" >> /var/backups/system.conf \
    && echo "" >> /var/backups/system.conf \
    && echo "# Storage Paths" >> /var/backups/system.conf \
    && echo "LOG_PATH=/opt/logs/" >> /var/backups/system.conf \
    && echo "BACKUP_PATH=/var/backups/" >> /var/backups/system.conf \
    && echo "DATA_FILE=data.log" >> /var/backups/system.conf \
    && echo "" >> /var/backups/system.conf \
    && echo "# Maintenance" >> /var/backups/system.conf \
    && echo "CLEANUP_ENABLED=true" >> /var/backups/system.conf \
    && echo "RETENTION_DAYS=7" >> /var/backups/system.conf \
    && rm /tmp/flag_part1.tmp

# Create log files with second part (more subtle)
RUN echo "=== Service Log ===" > /opt/logs/data.log \
    && echo "2025-09-16 17:41:22 [INFO] Service initialized" >> /opt/logs/data.log \
    && echo "2025-09-16 17:41:22 [INFO] Configuration loaded" >> /opt/logs/data.log \
    && echo "2025-09-16 17:41:22 [INFO] Network interface ready" >> /opt/logs/data.log \
    && echo "" >> /opt/logs/data.log \
    && cat /tmp/flag_part2.tmp | base64 >> /opt/logs/data.log \
    && echo "" >> /opt/logs/data.log \
    && echo "2025-09-16 17:41:23 [INFO] Startup complete" >> /opt/logs/data.log \
    && rm /tmp/flag_part2.tmp

# Create additional decoy files
RUN echo "2025-09-16 System backup completed" > /var/backups/backup.log \
    && echo "Status: OK" >> /var/backups/backup.log \
    && echo "Files: 42" >> /var/backups/backup.log \
    && echo "Size: 1.2MB" >> /var/backups/backup.log \
    && echo "" > /var/backups/.hidden \
    && echo "[2025-09-16 17:41:20] Apache service started" > /opt/logs/service.log \
    && echo "[2025-09-16 17:41:21] PHP module loaded" >> /opt/logs/service.log \
    && echo "[2025-09-16 17:41:22] PingChecker initialized" >> /opt/logs/service.log \
    && echo "[2025-09-16 17:41:23] Listening on port 80" >> /opt/logs/service.log \
    && echo "[2025-09-16 17:41:24] Ready for connections" >> /opt/logs/service.log \
    && echo "<?php // Legacy API endpoint - deprecated ?>" > /var/www/html/api.php.bak \
    && echo "Error log initialized" > /opt/logs/error.log \
    && echo "No errors reported" >> /opt/logs/error.log

RUN echo "#!/bin/bash" > /var/backups/cleanup.sh \
    && echo "# Weekly cleanup script" >> /var/backups/cleanup.sh \
    && echo "find /opt/logs -mtime +7 -delete" >> /var/backups/cleanup.sh \
    && chmod 644 /var/backups/cleanup.sh \
    && echo "temp_data_20250916" > /opt/logs/cache.tmp \
    && echo "session_store_empty" > /opt/logs/sessions.dat

RUN chmod 644 /var/backups/* /opt/logs/* \
    && chmod 755 /var/backups /opt/logs

RUN echo "ServerName localhost" >> /etc/apache2/apache2.conf \
    && echo "ServerTokens Prod" >> /etc/apache2/apache2.conf \
    && echo "ServerSignature Off" >> /etc/apache2/apache2.conf

# Copy web application files
COPY src/ /var/www/html/

# Set proper permissions
RUN chown -R www-data:www-data /var/www/html/ \
    && chmod -R 755 /var/www/html/ \
    && rm -f /var/www/html/index.html

EXPOSE 80

CMD ["apache2ctl", "-D", "FOREGROUND"]
