FROM eclipse-mosquitto:2.0 AS mosquitto

# Second stage for building the unified container
FROM python:3.9-slim@sha256:914169c7c8398b1b90c0b0ff921c8027445e39d7c25dc440337e56ce0f2566e6

ARG FLAG

# Install required packages
RUN apt-get update && \
    apt-get install -y \
        apache2 \
        apache2-utils \
        supervisor \
        && rm -rf /var/lib/apt/lists/*

# Install mosquitto
RUN apt-get update && \
    apt-get install -y mosquitto mosquitto-clients && \
    rm -rf /var/lib/apt/lists/*

# Install Python dependencies
COPY src/client/requirements.txt /tmp/client_requirements.txt
COPY src/credential-server/requirements.txt /tmp/credential_requirements.txt
RUN pip install --no-cache-dir -r /tmp/client_requirements.txt
RUN pip install --no-cache-dir -r /tmp/credential_requirements.txt

# Create application directories
RUN mkdir -p /app/client /app/credential-server /app/mock-hypercharger
RUN mkdir -p /mosquitto/config /mosquitto/data /mosquitto/log
RUN mkdir -p /var/www/html/cgi-bin /etc/apache2/conf/auth
RUN chmod 755 /mosquitto /mosquitto/config /mosquitto/data /mosquitto/log

# Copy Python applications
COPY src/client/spammer.py /app/client/
COPY src/credential-server/credential_server.py /app/credential-server/

# Copy mosquitto configuration
COPY src/mosquitto-new.conf /mosquitto/config/mosquitto.conf
COPY src/acl /mosquitto/config/acl
# Create empty password and log files that will be populated by startup script
RUN touch /mosquitto/config/passwd && chmod 644 /mosquitto/config/passwd
RUN touch /mosquitto/log/mosquitto.log && chmod 666 /mosquitto/log/mosquitto.log

# Copy web content
COPY src/mock-hypercharger/index.html /var/www/html/
COPY src/mock-hypercharger/styles/ /var/www/html/styles/
COPY src/mock-hypercharger/cgi-bin/ /var/www/html/cgi-bin/
RUN chmod +x /var/www/html/cgi-bin/*.cgi

# Apache configuration
RUN a2enmod cgi
RUN a2disconf serve-cgi-bin
RUN echo 'AuthType Basic\nAuthName "HyperCharger Access"\nAuthUserFile /etc/apache2/conf/auth/.htpasswd\nRequire valid-user' > /var/www/html/.htaccess
RUN echo 'AuthType Basic\nAuthName "HyperCharger Access"\nAuthUserFile /etc/apache2/conf/auth/.htpasswd\nRequire valid-user' > /var/www/html/cgi-bin/.htaccess

# Configure Apache to listen on port 8080 instead of 80
RUN sed -i 's/Listen 80/Listen 8080/' /etc/apache2/ports.conf
RUN sed -i 's/:80>/:8080>/' /etc/apache2/sites-available/000-default.conf

# Configure Apache for .htaccess and CGI
RUN echo '\n\
<Directory "/var/www/html">\n\
    AllowOverride All\n\
</Directory>\n\
\n\
<Directory "/var/www/html/cgi-bin">\n\
    AllowOverride All\n\
    Options +ExecCGI\n\
    AddHandler cgi-script .cgi\n\
</Directory>\n\
\n\
# CGI Script Alias\n\
ScriptAlias /cgi-bin/ /var/www/html/cgi-bin/\n\
\n\
# Pass FLAG environment variable to CGI scripts\n\
PassEnv FLAG' >> /etc/apache2/apache2.conf

# Create startup script
COPY src/startup.sh /startup.sh
RUN chmod +x /startup.sh

# Create supervisor configuration
RUN echo '[supervisord]\n\
nodaemon=true\n\
user=root\n\
\n\
[supervisorctl]\n\
serverurl=unix:///tmp/supervisor.sock\n\
\n\
[unix_http_server]\n\
file=/tmp/supervisor.sock\n\
\n\
[rpcinterface:supervisor]\n\
supervisor.rpcinterface_factory = supervisor.rpcinterface:make_main_rpcinterface\n\
\n\
[program:mosquitto]\n\
command=mosquitto -c /mosquitto/config/mosquitto.conf\n\
autostart=false\n\
autorestart=true\n\
priority=10\n\
stdout_logfile=/dev/stdout\n\
stdout_logfile_maxbytes=0\n\
stderr_logfile=/dev/stderr\n\
stderr_logfile_maxbytes=0\n\
\n\
[program:apache2]\n\
command=apache2ctl -D FOREGROUND\n\
autostart=false\n\
autorestart=true\n\
priority=20\n\
stdout_logfile=/dev/stdout\n\
stdout_logfile_maxbytes=0\n\
stderr_logfile=/dev/stderr\n\
stderr_logfile_maxbytes=0\n\
\n\
[program:client]\n\
command=python /app/client/spammer.py\n\
directory=/app/client\n\
autostart=false\n\
autorestart=true\n\
priority=30\n\
startsecs=5\n\
stdout_logfile=/dev/stdout\n\
stdout_logfile_maxbytes=0\n\
stderr_logfile=/dev/stderr\n\
stderr_logfile_maxbytes=0\n\
environment=MQTT_BROKER="localhost",MQTT_PORT="1883"\n\
\n\
[program:credential-server]\n\
command=python /app/credential-server/credential_server.py\n\
directory=/app/credential-server\n\
autostart=false\n\
autorestart=true\n\
priority=40\n\
startsecs=5\n\
stdout_logfile=/dev/stdout\n\
stdout_logfile_maxbytes=0\n\
stderr_logfile=/dev/stderr\n\
stderr_logfile_maxbytes=0\n\
environment=MQTT_BROKER="localhost",MQTT_PORT="1883",ADMIN_USERNAME="%(ENV_ADMIN_USERNAME)s",ADMIN_PASSWORD="%(ENV_ADMIN_PASSWORD)s",CHARGER_USERNAME="%(ENV_CHARGER_USERNAME)s",CHARGER_PASSWORD="%(ENV_CHARGER_PASSWORD)s",ENCRYPTED_UNLOCK_PHRASE="%(ENV_ENCRYPTED_UNLOCK_PHRASE)s"\n\
' > /etc/supervisor/conf.d/supervisord.conf

# Set default environment variables
ENV ADMIN_USERNAME=admin_user
ENV ADMIN_PASSWORD=S3cur3_P@ssw0rd_2025
ENV CHARGER_USERNAME=charger_webui
ENV CHARGER_PASSWORD=Alm0st_ChALL3ng3_S0lv3d!
ENV ENCRYPTED_UNLOCK_PHRASE=5573767475206e66204a20626e207569662062656e6a6f
ENV FLAG=$FLAG

EXPOSE 1883 8080

CMD ["/startup.sh"]