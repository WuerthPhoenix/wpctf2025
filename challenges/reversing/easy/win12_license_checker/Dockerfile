FROM rust:1.90.0 AS builder

WORKDIR /build

COPY src ./src
COPY Cargo.toml Cargo.lock ./

RUN cargo build --release
