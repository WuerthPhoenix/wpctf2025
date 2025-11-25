# Stage 1: Build stage
FROM fedora:42 AS builder

WORKDIR /app
RUN dnf install -y clang make openssl openssl-devel llhttp-devel
COPY . .
RUN make all

# Stage 2: Runtime stage
FROM fedora:42

ARG ADMIN_TOKEN="admin123"
ENV ADMIN_TOKEN=${ADMIN_TOKEN}

WORKDIR /app
COPY --from=builder /app/server /app/server.key /app/server.crt ./
COPY --from=builder /app/templates /app/templates
RUN dnf install -y openssl llhttp && dnf clean all

EXPOSE 9443

# Run the server binary
CMD ["./server"]
