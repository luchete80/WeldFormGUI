#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "Uso: $0 \"tu comentario\"" >&2
  exit 1
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
devlog_path="$repo_root/docs/DEVLOG.txt"

if [ ! -f "$devlog_path" ]; then
  echo "No se encontro $devlog_path" >&2
  exit 1
fi

today="$(date +%Y%m%d)"
entry="$(printf '%s - %s' "$today" "$1")"
printf '%s\n' "$entry" >> "$devlog_path"
printf '%s\n' "$entry"
