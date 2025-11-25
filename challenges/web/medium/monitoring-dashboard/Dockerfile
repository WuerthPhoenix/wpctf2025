FROM rust:1.89@sha256:9e1b362e100b2c510355314491708bdc59d79b8ed93e94580aba9e4a370badab AS builder

WORKDIR /app
COPY src/proxy/ ./
RUN cargo build --release

FROM ubuntu:plucky@sha256:57665ab8178042ef197191fd77d21d8a2f7f535acd26ff7bd548b1f340f081d7

# Accept FLAG argument with default value
ARG FLAG=WPCTF{REDACTED}

# Set environment variables to avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Update package list and install necessary packages
RUN apt update && \
    apt -y install apt-transport-https wget supervisor sqlite3

# Download and install Icinga2 keyring
RUN wget -O icinga-archive-keyring.deb "https://packages.icinga.com/icinga-archive-keyring_latest+ubuntu$(. /etc/os-release; echo "$VERSION_ID").deb" && \
    apt install -y ./icinga-archive-keyring.deb

# Add Icinga2 repository
COPY src/plucky-icinga.list /etc/apt/sources.list.d/plucky-icinga.list

# Update package list and install Icinga2
RUN apt update && \
    apt install -y icinga2=2.15.0-1+ubuntu25.04 \
                     icinga2-bin=2.15.0-1+ubuntu25.04 \
                     icinga2-common=2.15.0-1+ubuntu25.04 \
                     icinga2-doc=2.15.0-1+ubuntu25.04

# Enable Icinga2 API and features
RUN icinga2 api setup

# Create required directories for Icinga2 with proper permissions
RUN mkdir -p /run/icinga2 /var/log/icinga2 /var/log/supervisor /var/cache/icinga2 /var/spool/icinga2 /app && \
    chown -R nagios:nagios /run/icinga2 /var/log/icinga2 /var/cache/icinga2 /var/spool/icinga2 /var/lib/icinga2 && \
    chmod 755 /run/icinga2 /var/log/icinga2 /var/cache/icinga2 /var/spool/icinga2

# Fix permissions for Icinga2 configuration and data
RUN chown -R nagios:nagios /etc/icinga2 /var/lib/icinga2 && \
    chmod -R 750 /etc/icinga2 && \
    chmod 644 /etc/icinga2/icinga2.conf

# Copy tenant hosts configuration and create-hosts script
COPY src/icinga2/tenant_hosts.conf /etc/icinga2/conf.d/tenant_hosts.conf
COPY src/icinga2/create-hosts.sh /tmp/create-hosts.sh

# Replace @@FLAG@@ with the actual flag value directly in the final location
RUN sed -i "s/@@FLAG@@/${FLAG}/g" /etc/icinga2/conf.d/tenant_hosts.conf && \
    chown nagios:nagios /etc/icinga2/conf.d/tenant_hosts.conf && \
    chmod 644 /etc/icinga2/conf.d/tenant_hosts.conf

# Make create-hosts.sh executable, run it, set permissions, and clean up
RUN chmod +x /tmp/create-hosts.sh && \
    /bin/bash /tmp/create-hosts.sh && \
    chown nagios:nagios /etc/icinga2/conf.d/hosts.conf && \
    chmod 644 /etc/icinga2/conf.d/hosts.conf && \
    rm -f /tmp/create-hosts.sh

# Limit concurrent checks to 1
RUN echo "const MaxConcurrentChecks = 1" >> /etc/icinga2/constants.conf

# Copy and create SQLite database for user configuration
COPY src/init_db.sql /tmp/init_db.sql
RUN sqlite3 /app/users.db < /tmp/init_db.sql && \
    chmod 644 /app/users.db && \
    rm -f /tmp/init_db.sql

# Copy proxy binary from builder stage
COPY --from=builder /app/target/release/proxy /usr/local/bin/proxy
RUN chmod +x /usr/local/bin/proxy

# Copy static files for web UI
COPY src/static /app/static

# Copy proxy environment configuration
COPY src/proxy/.env /app/.env

# Extract Icinga2 password and update .env file
RUN ICINGA2_PASSWORD=$(cat /etc/icinga2/conf.d/api-users.conf | grep 'password = ' | sed 's/.*password = "\([^"]*\)".*/\1/') && \
    sed -i "s/@@ICINGA2_PASSWORD@@/${ICINGA2_PASSWORD}/g" /app/.env

# Generate rocket secret key and update .env file
RUN ROCKET_SECRET_KEY=$(openssl rand -base64 32) && \
    sed -i "s|@@SECRET_KEY@@|${ROCKET_SECRET_KEY}|g" /app/.env

# Copy supervisor configuration
COPY src/supervisord.conf /etc/supervisor/conf.d/supervisord.conf

# Expose Icinga2 and proxy ports
EXPOSE 8000

# Start supervisord
CMD ["/usr/bin/supervisord", "-n"]
