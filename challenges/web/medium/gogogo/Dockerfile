FROM golang:1.21.1 as frontend-builder

WORKDIR /build
COPY src/frontend/go.mod ./
RUN go mod download

COPY src/frontend/*.go ./
RUN CGO_ENABLED=0 GOOS=linux go build -o /build/proxy && chmod +x /build/proxy


FROM php:8.2-fpm

ARG FLAG=WPCTF{redacted}

RUN apt-get update && apt-get install -y \
    nginx \
    supervisor \
    && rm -rf /var/lib/apt/lists/*

COPY src/conf/nginx.conf /etc/nginx/nginx.conf
COPY src/conf/default.conf /etc/nginx/conf.d/default.conf
COPY src/conf/supervisord.conf /etc/supervisor/conf.d/supervisord.conf

EXPOSE 8080

# BACKEND
COPY src/backend/ /var/www/html/
RUN mkdir /sandbox && chmod 777 /sandbox

# BACKEND
RUN mkdir /frontend
COPY --from=frontend-builder /build/proxy /frontend/proxy

# BOT
RUN echo ${FLAG} > /frontend/flag.txt
COPY src/bot/bot.sh /frontend

CMD ["/usr/bin/supervisord", "-n"]
