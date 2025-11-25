FROM icinga/icinga2@sha256:9705397c92fc8d197b7f5c5b01fb024ebf05fd3d1538f354dcba10d85eaadeb7

ARG FLAG=WPCTF{R3D4CT3D}

COPY src/runtime.php /opt/runtime.php
COPY src/icinga_client.php /opt/icinga_client.php
COPY src/entrypoint.sh /opt/entrypoint.sh
COPY src/static/index.html /opt/index.html
COPY src/static/fonts /opt/fonts

EXPOSE 9090

USER root
RUN export ICINGA_PASS="$(openssl rand -base64 32)" \
    && echo "$ICINGA_PASS" >> /icinga_pass
RUN apt-get update
RUN apt-get install -y --no-install-recommends php php-curl

# Generate flag
RUN echo ${FLAG} > /flag.txt


USER icinga

CMD [ "bash", "/opt/entrypoint.sh" ]
