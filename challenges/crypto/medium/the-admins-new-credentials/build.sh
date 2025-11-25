#!/bin/bash

CHALLENGE_NAME=$(basename $(pwd))

ADMIN_TOKEN=$(openssl rand -base64 32)

zip "$CHALLENGE_NAME.zip" -r . -x@.zipignore -x@.gitignore
docker build . --build-arg ADMIN_TOKEN=${ADMIN_TOKEN} --tag "wpctf2025-challenges:$CHALLENGE_NAME"
