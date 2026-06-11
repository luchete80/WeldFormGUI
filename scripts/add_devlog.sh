#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "Uso: $0 \"tu comentario\"" >&2
  exit 1
fi

today="$(date +%Y%m%d)"
printf '%s - %s\n' "$today" "$1"
