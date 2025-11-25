#!/usr/bin/env bash

set -euo pipefail

find . -type f -name "build.sh" | while read -r script; do
    echo ">>> Running $script"
    dir=$(dirname "$script")
    (
        cd "$dir"
        sudo bash ./build.sh
    )
done
