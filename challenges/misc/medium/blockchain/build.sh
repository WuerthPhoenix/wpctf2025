#!/usr/bin/env bash

set -euo pipefail

CHALLENGE_NAME="blockchain"

# Create Docker image
docker build --no-cache --tag wpctf2025-challenges:$CHALLENGE_NAME .

# Create zip file
zip -r $CHALLENGE_NAME.zip ./foundry.toml ./src/Setup.sol ./src/Target.sol
