#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cmake_cmd=${CMAKE_COMMAND:-cmake}
"$cmake_cmd" --build "$root/build" --target load-tests -j4
mkdir -p "$root/build/reports"
QT_QPA_PLATFORM=offscreen "$root/build/load-tests" > "$root/build/reports/load-test.json"
cat "$root/build/reports/load-test.json"
