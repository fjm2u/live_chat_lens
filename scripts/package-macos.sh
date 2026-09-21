#!/bin/sh
set -eu
# Relink to Qt already shipped with OBS; never load Homebrew Qt into OBS.
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
bundle="$root/build/obs-comment-dock.plugin"
binary="$bundle/Contents/MacOS/obs-comment-dock"
[ -f "$binary" ] || { echo 'Build the plugin first.' >&2; exit 1; }
for framework in QtCore QtGui QtWidgets QtNetwork; do
    old=$(otool -L "$binary" | awk -v name="$framework.framework" 'index($1,name) {print $1; exit}')
    [ -z "$old" ] || install_name_tool -change "$old" "@rpath/$framework.framework/Versions/A/$framework" "$binary"
done
codesign --force --sign - "$bundle"
echo "$bundle"
