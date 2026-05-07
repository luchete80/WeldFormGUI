#!/usr/bin/env bash

set -u

DEST_DIR="${HOME}/Numerico/WeldFormGUI_bin/solvers"
SRC_DIR="$(pwd)"

mkdir -p "${DEST_DIR}"

copy_one() {
  local name="$1"
  if [ -f "${SRC_DIR}/${name}" ]; then
    cp -f "${SRC_DIR}/${name}" "${DEST_DIR}/${name}"
    echo "Copied ${name} -> ${DEST_DIR}/${name}"
  else
    echo "Missing ${SRC_DIR}/${name}"
  fi
}

copy_one "weldform_exp"
copy_one "weldform_exp_std"
copy_one "weldform_imp"
copy_one "weldform_imp_std"

echo "Done."
