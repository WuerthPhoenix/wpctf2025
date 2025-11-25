#!/bin/bash

CHALLENGE_NAME=${PWD##*/}
SCRIPT_DIR=$(pwd)
WORK_DIR=`mktemp -d -p "$DIR"`
if [[ ! "$WORK_DIR" || ! -d "$WORK_DIR" ]]; then
  echo "Could not create temp dir"
  exit 1
fi
echo "Created temp dir at $WORK_DIR"

function cleanup {
  rm -rf "$WORK_DIR"
  echo "Deleted temp working directory $WORK_DIR"
}

trap cleanup EXIT

rm -f "$SCRIPT_DIR/$CHALLENGE_NAME.zip"

cd $WORK_DIR

# Copy files to the temporary folder
cp -r $SCRIPT_DIR/src .
cp $SCRIPT_DIR/description.md .
cp $SCRIPT_DIR/Dockerfile .

zip -r "$SCRIPT_DIR/$CHALLENGE_NAME.zip" ./

cd $SCRIPT_DIR

docker build --file Dockerfile --no-cache --build-arg FLAG=$(cat ./flag.txt) --tag wpctf2025-challenges:$CHALLENGE_NAME .

