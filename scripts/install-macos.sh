#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cmake_cmd=${CMAKE_COMMAND:-cmake}
destination="$HOME/Library/Application Support/obs-studio/plugins"
"$root/scripts/package-macos.sh"
"$cmake_cmd" --install "$root/build" --prefix "$destination"
# CMake changes the install RPATH, so sign the installed bundle again.
codesign --force --sign - "$destination/obs-comment-dock.plugin"
codesign --verify --deep --strict "$destination/obs-comment-dock.plugin"
