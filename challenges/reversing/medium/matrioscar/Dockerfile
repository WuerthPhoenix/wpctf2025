FROM rust:1.90.0 AS builder

WORKDIR /build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    libpython3.13 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=ghcr.io/astral-sh/uv:0.9.2 /uv /uvx /bin/

COPY src ./src
COPY matrioscar_py ./matrioscar_py
COPY Cargo.toml Cargo.lock pyproject.toml uv.lock ./

RUN uv sync \
    && . .venv/bin/activate \
    && maturin build \
    && pyinstaller matrioscar_py/main.py --onefile


FROM debian@sha256:78924202067b925b0a57cc75193015e0a1587cdf0e0bb50e09ddace13ea01985

RUN apt-get update \
    && apt-get install -y --no-install-recommends socat libpython3.13 \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m limiteduser -s /bin/sh

COPY --from=builder --chown=limiteduser:limiteduser --chmod=760 /build/dist/main /matrioscar
COPY --chown=limiteduser:limiteduser --chmod=640 flag.txt /flag.txt

EXPOSE 27381

USER limiteduser

CMD ["socat", "TCP-LISTEN:27381,reuseaddr,fork", "EXEC:/matrioscar"]
