#!/usr/bin/env bash

CHALLENGE_NAME=$(basename $(pwd))

PASSWORD="my-password-$(openssl rand -hex 16)"
FLAG="$(cat flag.txt)"

docker build . \
    --build-arg "password=${PASSWORD}" \
    --build-arg "flag=${FLAG}" \
    --tag "wpctf2025-challenges:$CHALLENGE_NAME"

docker build . \
    --build-arg "password=PWD-REDACTED" \
    --build-arg "flag=WPCTF{REDACTED}" \
    --tag "wpctf2025-challenges:$CHALLENGE_NAME.tmp"

tmp_container_id=$(docker create wpctf2025-challenges:$CHALLENGE_NAME.tmp)
docker cp "$tmp_container_id":/app/system_information .

docker rm "$tmp_container_id"
docker image rm wpctf2025-challenges:$CHALLENGE_NAME.tmp

echo "$CHALLENGE_NAME".zip
if [ -f "$CHALLENGE_NAME".zip ]; then
    rm "$CHALLENGE_NAME".zip
fi

zip "$CHALLENGE_NAME".zip system_information
