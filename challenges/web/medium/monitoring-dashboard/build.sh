#!/bin/bash

CHALLENGE_NAME=${PWD##*/}
SCRIPT_DIR=$(pwd)
WORK_DIR=`mktemp -d -p "$DIR"`
if [[ ! "$WORK_DIR" || ! -d "$WORK_DIR" ]]; then
  echo "Could not create temp dir"
  exit 1
fi

function cleanup {
  rm -rf "$WORK_DIR"
  echo "Deleted temp working directory $WORK_DIR"
}

trap cleanup EXIT

# Get flag from file and trim
FLAG=$(cat flag.txt | tr -d '\n')

rm -f "$SCRIPT_DIR/$CHALLENGE_NAME.zip"

cp . $WORK_DIR -r
cd $WORK_DIR

# Remove files based on patterns in .zipignore
while IFS= read -r pattern; do
  # Skip empty lines and comments
  if [[ -z "$pattern" || "$pattern" =~ ^[[:space:]]*# ]]; then
    continue
  fi

  # Remove leading/trailing whitespace
  pattern=$(echo "$pattern" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
  find . -name "$pattern" -exec rm -rf {} + 2>/dev/null
done < .zipignore

zip -r "$SCRIPT_DIR/$CHALLENGE_NAME.zip" ./
docker build --file Dockerfile --no-cache --build-arg FLAG="$FLAG"  --tag wpctf2025-challenges:$CHALLENGE_NAME .
