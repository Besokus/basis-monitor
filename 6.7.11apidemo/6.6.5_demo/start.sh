#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
RUNTIME_ROOT="${HOME}/.cache/ctp_demo_linux"
BUILD_DIR="${RUNTIME_ROOT}/build"

cp -f "${SCRIPT_DIR}/config.ini" "${RUNTIME_ROOT}/config.ini"
rm -rf "${RUNTIME_ROOT}/flow"
ln -s "${SCRIPT_DIR}/flow" "${RUNTIME_ROOT}/flow"

cd "${BUILD_DIR}"
sudo ./testprogram
