#!/bin/bash

set -euxo pipefail

CHALLENGE_NAME="win12_license_checker"
IMAGE_NAME="wpctf2025-challenges:$CHALLENGE_NAME"

CONTAINER_RUNTIME="podman"
 if command -v docker &>/dev/null; then
     CONTAINER_RUNTIME="docker"
 fi

"$CONTAINER_RUNTIME" build --file Dockerfile --tag "$IMAGE_NAME" .
tmp_container_id=$($CONTAINER_RUNTIME create "$IMAGE_NAME")
"$CONTAINER_RUNTIME" cp "$tmp_container_id:/build/target/release/$CHALLENGE_NAME" .
"$CONTAINER_RUNTIME" rm "$tmp_container_id"

if [ -f "$CHALLENGE_NAME".zip ]; then
    rm "$CHALLENGE_NAME".zip
fi

zip -r $CHALLENGE_NAME.zip $CHALLENGE_NAME
