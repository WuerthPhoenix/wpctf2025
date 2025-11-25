#!/bin/bash

CHALLENGE_NAME=$(basename $(pwd))

# Get flag from file and trim
FLAG=$(cat flag.txt | tr -d '\n')
SECRET=$(cat secret.txt | tr -d '\n')

zip "$CHALLENGE_NAME.zip" -r . -x@.zipignore
docker build . --no-cache \
    --build-arg "FLAG=${FLAG}" \
    --build-arg "SECRET=${SECRET}" \
    --tag wpctf2025-challenges:$CHALLENGE_NAME
