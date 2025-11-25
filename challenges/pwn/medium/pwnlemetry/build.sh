#!/usr/bin/env bash
set -e

CHALLENGE_NAME=$(basename $(pwd))

CONTAINER_RUNTIME="podman"
if command -v docker &>/dev/null; then
    CONTAINER_RUNTIME="docker"
fi

"$CONTAINER_RUNTIME" build . \
    --tag "wpctf2025-challenges:$CHALLENGE_NAME"

tmp_container_id=$($CONTAINER_RUNTIME create wpctf2025-challenges:$CHALLENGE_NAME)
$CONTAINER_RUNTIME cp "$tmp_container_id":/app/pwnlemetry .

$CONTAINER_RUNTIME rm "$tmp_container_id"

if [ -f "$CHALLENGE_NAME".zip ]; then
    rm "$CHALLENGE_NAME".zip
fi

zip "$CHALLENGE_NAME".zip pwnlemetry
