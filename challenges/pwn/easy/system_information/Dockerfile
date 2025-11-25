FROM debian:bookworm AS builder

WORKDIR /app
COPY src/ /app

ARG flag
ARG password

RUN apt-get update && apt-get install -y clang make
RUN make -C /app FLAG="${flag}" PASSWORD="${password}"

FROM debian:bookworm
WORKDIR /app
COPY --from=builder /app/system_information /app/system_information
RUN apt-get update && apt-get install -y socat && apt-get clean

EXPOSE 4444

CMD ["socat", "TCP-LISTEN:4444,fork", "EXEC:/app/system_information"]
