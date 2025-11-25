#!/bin/sh
set -eu

echo "[pinger] -> http://127.0.0.1:8080 30s"

while true; do
  TS=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

  curl -sS -X POST 'http://127.0.0.1:8080/?filename=note.txt' \
    -F "file=@/frontend/flag.txt" \
    -w " [status:%{http_code}]\n" -o /dev/null || true

  echo "[pinger] $TS done"
  sleep "30"
done