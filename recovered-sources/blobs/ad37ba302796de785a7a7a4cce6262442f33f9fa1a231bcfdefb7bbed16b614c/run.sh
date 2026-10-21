#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
WINE="/Applications/Wine Staging.app/Contents/Resources/wine/bin/wine"
WINEPREFIX="$HOME/Library/Application Support/Корсары/wineprefix"

if [ ! -x "$WINE" ]; then
    echo "Ошибка: Wine не найден по пути /Applications/Wine Staging.app" >&2
    exit 1
fi

cd "$DIR/game" || exit 1
export WINEPREFIX
exec "$WINE" ENGINE.exe
