FROM php:8.2-apache

ARG FLAG=WPCTF{redacted}

RUN apt-get update && apt-get install -y mariadb-server build-essential \
    && docker-php-ext-install pdo pdo_mysql \
    && rm -rf /var/lib/apt/lists/*
RUN a2enmod rewrite


COPY src/init-db.sh /init-db.sh
RUN chmod +x /init-db.sh

EXPOSE 80
COPY src/web /var/www/html/

# COPY FLAG
RUN echo ${FLAG} > /flag.txt
COPY src/readflag.c /
RUN chmod 440 /flag.txt
RUN gcc /readflag.c -o /readflag && chmod 4555 /readflag && rm /readflag.c


CMD service mariadb start && /init-db.sh && exec apache2-foreground

