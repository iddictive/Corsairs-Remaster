#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
profile_dir="$repo_root/experiments/native-metal/.cache/diagnostics"

printf 'Через 5 секунд начнётся запись на 10 секунд. Оставь нужную сцену открытой.\n'
sleep 5

game_pid=$(/usr/bin/pgrep -x metal-engine || true)
case "$game_pid" in
    ''|*[!0-9]*)
        printf 'Нужен один запущенный процесс установленной игры.\n' >&2
        exit 1
        ;;
esac
if [ "$(/bin/ps -p "$game_pid" -o comm=)" != '/Applications/Corsairs Iddictive Remaster.app/Contents/MacOS/metal-engine' ]; then
    printf 'Запущен другой движок; нужен процесс установленной игры.\n' >&2
    exit 1
fi

mkdir -p "$profile_dir"
profile_file="$profile_dir/game-$(date +%Y%m%d-%H%M%S)-$game_pid.sample.txt"
/usr/bin/sample "$game_pid" 10 5 -file "$profile_file"
printf 'Профиль сохранён: %s\n' "$profile_file"
