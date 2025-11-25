FROM ubuntu:24.04 AS builder

RUN apt-get update \
    && apt-get -y --no-install-recommends install \
    build-essential \
    libssl-dev

WORKDIR /build
COPY Makefile ./
COPY ./src ./src

RUN make

FROM ubuntu:24.04
ARG EXEC_NAME=pwnlemetry
ENV ADMIN_TOKEN="28faccbebf5b673a33bb00f80dd12636"

RUN apt-get update && apt-get -y --no-install-recommends install socat

WORKDIR /app

COPY --from=builder /build/${EXEC_NAME} ./${EXEC_NAME}

RUN useradd -m limiteduser \
    && chown -R limiteduser:limiteduser ./${EXEC_NAME} ./${EXEC_NAME} \
    && chmod 740 ./${EXEC_NAME}

USER limiteduser

COPY ./monitors.dat .

EXPOSE 31337

CMD ["/app/pwnlemetry", "--server"]
