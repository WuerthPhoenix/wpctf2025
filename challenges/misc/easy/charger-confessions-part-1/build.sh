#!/bin/bash

FLAG=$(cat ./flag.txt)
CH_NAME=${PWD##*/}

docker build --file Dockerfile --build-arg FLAG="$FLAG" --tag wpctf2025-challenges:${CH_NAME} .