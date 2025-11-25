FROM ethereum/client-go:v1.16.5@sha256:32b878e4144a8de06a1003a2d4b0ba067fd578e0080a719338f18b229cc1e53c AS geth

FROM ghcr.io/foundry-rs/foundry:v1.4.3@sha256:409c397a8d9675d8c911e564727b959d0c76e5efb3193f512a69bf8b89a7f85f

COPY --from=geth /usr/local/bin/geth /usr/local/bin/geth

USER root

RUN apt-get update && apt-get install --no-install-recommends -y jq socat \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

USER foundry

WORKDIR /home/foundry

COPY ./src /home/foundry/src
COPY ./entrypoint.sh /home/foundry/entrypoint.sh
COPY ./flag.txt /home/foundry/flag.txt

EXPOSE 1337
EXPOSE 8545

ENTRYPOINT ["socat", "TCP-LISTEN:1337,reuseaddr,fork", "EXEC:/home/foundry/entrypoint.sh"]
