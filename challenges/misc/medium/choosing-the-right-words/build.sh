#!/bin/bash

git lfs install
git lfs pull

CHALLENGE_NAME=${PWD##*/}

rm -rf "$CHALLENGE_NAME.zip"
zip -r "$CHALLENGE_NAME.zip" challenge Dockerfile


# Get flag from file and trim
FLAG=$(cat flag.txt | tr -d '\n')

docker build --no-cache --build-arg FLAG="$FLAG"  --tag wpctf2025-challenges:$CHALLENGE_NAME -f Dockerfile .