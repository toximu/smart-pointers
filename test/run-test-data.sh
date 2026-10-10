#!/bin/bash
set -euo pipefail
[ "$#" -ge 1 ] || (echo "Expected at least one argument: command to run" && exit 1)
COMMAND=("$@")
TIMEOUT=15s  # Careful of Valgrind

timeout -k 0.1s "$TIMEOUT" "${COMMAND[@]}"
