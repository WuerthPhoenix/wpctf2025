#!/bin/bash

FLAG="$(cat flag.txt)"
CH_NAME="wpctftpd"
IMAGE_NAME="wpctf2025-challenges"

docker build --file Dockerfile.build --build-arg CH_NAME="$CH_NAME" -o "${CH_NAME}" .

docker build --file Dockerfile --build-arg FLAG="$FLAG" --tag ${IMAGE_NAME}:${CH_NAME} ./${CH_NAME}
#docker build --file Dockerfile.devel --build-arg FLAG="$FLAG" --tag ${IMAGE_NAME}:${CH_NAME}-devel ./${CH_NAME}

zip -r -j ${CH_NAME}.zip ${CH_NAME}/wpctftpd Dockerfile
