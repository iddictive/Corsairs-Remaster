#!/bin/zsh

set -euo pipefail

crossover_app="${CROSSOVER_APP:-${HOME}/Applications/CrossOver.app}"
bottle="GAMES"
game_dir="${CORSAIRS_GAME_DIR:-/REQUIRED_EXTERNAL_INPUT/Library/Application Support/CrossOver/Bottles/GAMES/drive_c/Games/KVL-Codex-Test}"
peer_engine="${CORSAIRS_PEER_ENGINE:-/REQUIRED_EXTERNAL_INPUT/Library/Application Support/CrossOver/Bottles/GAMES/drive_c/Games/KVL/engine.exe}"
runtime_label="${CORSAIRS_RUNTIME_LABEL:-Test}"
expected_basename="${CORSAIRS_EXPECTED_BASENAME:-KVL-Codex-Test}"
wine="${crossover_app}/Contents/SharedSupport/CrossOver/bin/wine"
engine="${game_dir}/engine.exe"
lifecycle_lock="${game_dir}/.codex-engine-lifecycle.lock"
pending_install="${game_dir}/.codex-engine-install.pending"

release_lock() {
  [[ -f "${lifecycle_lock}" && ! -L "${lifecycle_lock}" && "$(<"${lifecycle_lock}")" == "$$" ]] || return 0
  unlink "${lifecycle_lock}"
}

reclaim_stale_lock() {
  local owner=""
  if [[ -f "${lifecycle_lock}" && ! -L "${lifecycle_lock}" ]]; then
    owner="$(<"${lifecycle_lock}")"
    if [[ "${owner}" =~ '^[0-9]+$' ]]; then
      kill -0 "${owner}" 2>/dev/null && return 1
      unlink "${lifecycle_lock}"
      return 0
    fi
  elif [[ -d "${lifecycle_lock}" && ! -L "${lifecycle_lock}" ]]; then
    if [[ -f "${lifecycle_lock}/pid" ]]; then
      owner="$(<"${lifecycle_lock}/pid")"
      if [[ "${owner}" =~ '^[0-9]+$' ]] && ! kill -0 "${owner}" 2>/dev/null; then
        unlink "${lifecycle_lock}/pid"
        rmdir "${lifecycle_lock}"
        return 0
      fi
    fi
  else
    return 1
  fi

  local modified
  local now
  modified="$(stat -f %m "${lifecycle_lock}" 2>/dev/null)" || return 1
  now="$(date +%s)"
  (( now - modified > 60 )) || return 1
  if [[ -f "${lifecycle_lock}" && ! -L "${lifecycle_lock}" ]]; then
    unlink "${lifecycle_lock}"
  elif [[ -d "${lifecycle_lock}" && ! -L "${lifecycle_lock}" && ! -e "${lifecycle_lock}/pid" ]]; then
    rmdir "${lifecycle_lock}"
  else
    return 1
  fi
}

assert_engine_idle() {
  local output=""
  local command_status=0
  output="$(lsof -t -- "${engine}" 2>&1)" || command_status=$?
  if [[ ${command_status} -eq 0 ]]; then
    print -u2 "${runtime_label} engine is already running (holder PID(s): ${(j:, :)${(f)output}})"
    exit 1
  fi
  if [[ ${command_status} -ne 1 || -n "${output}" ]]; then
    print -u2 "Cannot prove ${runtime_label} engine ownership with lsof: ${output:-exit ${command_status}}"
    exit 1
  fi
}

assert_peer_idle() {
  [[ -e "${peer_engine}" ]] || return 0
  local output=""
  local command_status=0
  output="$(lsof -t -- "${peer_engine}" 2>&1)" || command_status=$?
  if [[ ${command_status} -eq 0 ]]; then
    print -u2 "Peer Corsairs runtime is already running (holder PID(s): ${(j:, :)${(f)output}})"
    exit 1
  fi
  if [[ ${command_status} -ne 1 || -n "${output}" ]]; then
    print -u2 "Cannot prove peer runtime ownership with lsof: ${output:-exit ${command_status}}"
    exit 1
  fi
}

if [[ ! -x "${wine}" ]]; then
  print -u2 "CrossOver Wine not found: ${wine}"
  exit 1
fi

if [[ ! -f "${engine}" ]]; then
  print -u2 "${runtime_label} engine not found: ${engine}"
  exit 1
fi

[[ ! -L "${game_dir}" && ! -L "${engine}" ]] || {
  print -u2 "Symlinked ${runtime_label} runtime is refused"
  exit 1
}
[[ "$(/usr/bin/python3 -c 'import os, sys; print(os.path.realpath(sys.argv[1]))' "${game_dir}")" == "${game_dir}" ]] || {
  print -u2 "${runtime_label} game path resolves outside the fixed target"
  exit 1
}
[[ "${game_dir:t}" == "${expected_basename}" ]] || {
  print -u2 "Unexpected ${runtime_label} game basename: ${game_dir:t}"
  exit 1
}
if [[ -e "${peer_engine}" && "${engine}" -ef "${peer_engine}" ]]; then
  print -u2 "${runtime_label} engine aliases its peer runtime executable"
  exit 1
fi

if [[ -e "${pending_install}" ]]; then
  print -u2 "An interrupted engine install must be recovered before launch: ${pending_install}"
  exit 1
fi

assert_engine_idle
assert_peer_idle

lock_candidate="${lifecycle_lock}.$$"
print -r -- "$$" > "${lock_candidate}"
lock_acquired="no"
if ln "${lock_candidate}" "${lifecycle_lock}" 2>/dev/null; then
  lock_acquired="yes"
elif reclaim_stale_lock && ln "${lock_candidate}" "${lifecycle_lock}" 2>/dev/null; then
  lock_acquired="yes"
fi
unlink "${lock_candidate}"
if [[ "${lock_acquired}" != "yes" ]]; then
  lock_owner="unknown"
  [[ -f "${lifecycle_lock}" && ! -L "${lifecycle_lock}" ]] && lock_owner="$(<"${lifecycle_lock}")"
  print -u2 "Engine lifecycle is busy (lock owner PID ${lock_owner})"
  exit 1
fi
trap release_lock EXIT INT TERM HUP
assert_engine_idle
assert_peer_idle

"${wine}" \
  --bottle "${bottle}" \
  --no-update \
  --workdir "${game_dir}" \
  "${engine}"
