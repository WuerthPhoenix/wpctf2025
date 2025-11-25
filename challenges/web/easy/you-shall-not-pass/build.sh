#!/bin/bash

CHALLENGE_NAME=${PWD##*/}

# Get flag from file and trim
FLAG=$(cat flag.txt | tr -d '\n')

docker build --no-cache --build-arg FLAG="$FLAG"  --tag wpctf2025-challenges:$CHALLENGE_NAME -f Dockerfile .
