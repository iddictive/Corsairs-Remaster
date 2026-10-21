#!/bin/zsh

set -euo pipefail

readonly game_dir="/REQUIRED_EXTERNAL_INPUT/Library/Application Support/CrossOver/Bottles/GAMES/drive_c/Games/KVL"
readonly stable_dir="/REQUIRED_EXTERNAL_INPUT/Library/Application Support/CrossOver/Bottles/GAMES/drive_c/Games/KVL-Codex-Test"
readonly candidates_root="/REQUIRED_EXTERNAL_INPUT/05_Repo/StormEngine-Corsairs/build/candidates"
readonly engine_source="/REQUIRED_EXTERNAL_INPUT/05_Repo/StormEngine-Corsairs"
readonly snapshots_root="${game_dir}/.codex-engine-snapshots"
readonly lifecycle_lock="${game_dir}/.codex-engine-lifecycle.lock"
readonly pending_install="${game_dir}/.codex-engine-install.pending"
readonly llvm_readobj="/opt/homebrew/opt/llvm/bin/llvm-readobj"
readonly llvm_pdbutil="/opt/homebrew/opt/llvm/bin/llvm-pdbutil"

typeset RESTORE_ON_EXIT=""
typeset STAGE_PREFIX=""
typeset LOCK_OWNED="no"

die() {
  print -u2 -- "$*"
  trap - EXIT INT TERM HUP
  if [[ "${LOCK_OWNED}" == "yes" ]]; then
    local restore_owner="${RESTORE_ON_EXIT}"
    RESTORE_ON_EXIT=""
    if [[ -n "${restore_owner}" ]] && ! (restore_triplet "${restore_owner}"); then
      print -u2 -- "Automatic restore failed; launch remains blocked by ${pending_install}"
    fi
    cleanup_temp_prefix "${STAGE_PREFIX}"
    [[ -e "${pending_install}.tmp.$$" ]] && unlink "${pending_install}.tmp.$$" || true
    release_lock
  fi
  exit 1
}

sha256() {
  shasum -a 256 -- "$1" | awk '{print $1}'
}

manifest_value() {
  local manifest="$1"
  local key="$2"
  awk -F '\t' -v wanted="${key}" '$1 == wanted {print $2; exit}' "${manifest}"
}

real_path() {
  /usr/bin/python3 -c 'import os, sys; print(os.path.realpath(sys.argv[1]))' "$1"
}

assert_regular_file() {
  local path="$1"
  [[ -f "${path}" ]] || die "Missing regular file: ${path}"
  [[ ! -L "${path}" ]] || die "Symlinked runtime component is refused: ${path}"
}

assert_fixed_target() {
  [[ -d "${game_dir}" ]] || die "Primary game directory is missing: ${game_dir}"
  [[ ! -L "${game_dir}" ]] || die "Symlinked primary game directory is refused: ${game_dir}"
  [[ "$(real_path "${game_dir}")" == "${game_dir}" ]] || die "Primary game path resolves outside the fixed target"
  [[ "${game_dir:t}" == "KVL" ]] || die "Unexpected primary game basename: ${game_dir:t}"
  [[ "${game_dir}" != "${stable_dir}" ]] || die "Frozen KVL-Codex-Test is never a staging target"

  local name
  for name in engine.exe engine.pdb mimalloc.dll; do
    assert_regular_file "${game_dir}/${name}"
    if [[ -e "${stable_dir}/${name}" && "${game_dir}/${name}" -ef "${stable_dir}/${name}" ]]; then
      die "Primary component aliases the frozen KVL-Codex-Test file: ${name}"
    fi
  done
}

read_exe_identity() {
  local exe="$1"
  local output
  output="$("${llvm_readobj}" --coff-debug-directory "${exe}")" || die "Cannot inspect PE debug identity: ${exe}"
  EXE_FORMAT="$(print -r -- "${output}" | awk '/^Format:/ {print $2; exit}')"
  EXE_GUID="$(print -r -- "${output}" | awk '/PDBGUID:/ {gsub(/[{}]/, "", $2); print toupper($2); exit}')"
  EXE_AGE="$(print -r -- "${output}" | awk '/PDBAge:/ {print $2; exit}')"
  [[ "${EXE_FORMAT}" == "COFF-x86-64" ]] || die "Engine is not a PE32+ x86-64 executable: ${exe}"
  [[ "${EXE_GUID}" =~ '^[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}$' ]] || die "Engine has no readable CodeView GUID: ${exe}"
  [[ "${EXE_AGE}" =~ '^[0-9]+$' ]] || die "Engine has no readable PDB age: ${exe}"
}

read_pdb_identity() {
  local pdb="$1"
  local output
  output="$("${llvm_pdbutil}" dump -summary "${pdb}")" || die "Cannot inspect PDB identity: ${pdb}"
  PDB_GUID="$(print -r -- "${output}" | awk '/^[[:space:]]*GUID:/ {gsub(/[{}]/, "", $2); print toupper($2); exit}')"
  PDB_AGE="$(print -r -- "${output}" | awk '/^[[:space:]]*Age:/ {print $2; exit}')"
  [[ "${PDB_GUID}" =~ '^[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}$' ]] || die "PDB has no readable GUID: ${pdb}"
  [[ "${PDB_AGE}" =~ '^[0-9]+$' ]] || die "PDB has no readable age: ${pdb}"
}

assert_x64_pe() {
  local component="$1"
  local format
  format="$("${llvm_readobj}" --file-headers "${component}" | awk '/^Format:/ {print $2; exit}')" ||
    die "Cannot inspect PE architecture: ${component}"
  [[ "${format}" == "COFF-x86-64" ]] || die "Runtime component is not PE32+ x86-64: ${component}"
}

inspect_triplet() {
  local root="$1"
  local engine_name="$2"
  assert_regular_file "${root}/${engine_name}"
  assert_regular_file "${root}/engine.pdb"
  assert_regular_file "${root}/mimalloc.dll"

  ENGINE_SHA="$(sha256 "${root}/${engine_name}")"
  PDB_SHA="$(sha256 "${root}/engine.pdb")"
  MIMALLOC_SHA="$(sha256 "${root}/mimalloc.dll")"
  read_exe_identity "${root}/${engine_name}"
  read_pdb_identity "${root}/engine.pdb"
  assert_x64_pe "${root}/mimalloc.dll"
  if [[ "${EXE_GUID}" == "${PDB_GUID}" && "${EXE_AGE}" == "${PDB_AGE}" ]]; then
    DEBUG_MATCH="yes"
  else
    DEBUG_MATCH="no"
  fi
  BUNDLE_ID="$(print -rn -- "${ENGINE_SHA}${PDB_SHA}${MIMALLOC_SHA}" | shasum -a 256 | awk '{print $1}')"
}

verify_source_provenance() {
  local exe="$1"
  local source_commit="$2"
  local resolved_commit
  local watermark
  resolved_commit="$(git -C "${engine_source}" rev-parse --verify "${source_commit}^{commit}" 2>/dev/null)" ||
    die "Source commit does not resolve in the canonical engine worktree: ${source_commit}"
  [[ "${resolved_commit}" == "${source_commit}" ]] || die "Resolved source commit differs from the requested OID"
  watermark="$(strings -a "${exe}" | rg -m 1 'codex/corsairs-engine\([0-9a-f]{40}\)(-DIRTY\([0-9a-f]+\))?' || true)"
  [[ -n "${watermark}" ]] || die "Candidate engine has no canonical source watermark"
  [[ "${watermark}" == *"(${source_commit})"* ]] || die "Candidate watermark does not match source commit ${source_commit}: ${watermark}"
  [[ "${watermark}" != *"-DIRTY("* ]] || die "Refusing a dirty-watermark engine candidate: ${watermark}"
}

write_manifest() {
  local path="$1"
  local source_commit="$2"
  local classification="$3"
  {
    print -r -- $'format\t1'
    print -r -- $'bundle_id\t'"${BUNDLE_ID}"
    print -r -- $'source_commit\t'"${source_commit}"
    print -r -- $'classification\t'"${classification}"
    print -r -- $'engine_sha256\t'"${ENGINE_SHA}"
    print -r -- $'pdb_sha256\t'"${PDB_SHA}"
    print -r -- $'mimalloc_sha256\t'"${MIMALLOC_SHA}"
    print -r -- $'exe_guid\t'"${EXE_GUID}"
    print -r -- $'exe_age\t'"${EXE_AGE}"
    print -r -- $'pdb_guid\t'"${PDB_GUID}"
    print -r -- $'pdb_age\t'"${PDB_AGE}"
    print -r -- $'debug_match\t'"${DEBUG_MATCH}"
  } > "${path}"
}

verify_manifest() {
  local root="$1"
  local require_debug_match="${2:-yes}"
  local manifest="${root}/manifest.tsv"
  assert_regular_file "${manifest}"
  [[ "$(manifest_value "${manifest}" format)" == "1" ]] || die "Unsupported candidate manifest format"
  [[ "$(manifest_value "${manifest}" bundle_id)" == "${BUNDLE_ID}" ]] || die "Bundle ID does not match its files"
  [[ "$(manifest_value "${manifest}" engine_sha256)" == "${ENGINE_SHA}" ]] || die "Engine hash mismatch"
  [[ "$(manifest_value "${manifest}" pdb_sha256)" == "${PDB_SHA}" ]] || die "PDB hash mismatch"
  [[ "$(manifest_value "${manifest}" mimalloc_sha256)" == "${MIMALLOC_SHA}" ]] || die "Mimalloc hash mismatch"
  [[ "$(manifest_value "${manifest}" exe_guid)" == "${EXE_GUID}" ]] || die "EXE GUID mismatch"
  [[ "$(manifest_value "${manifest}" exe_age)" == "${EXE_AGE}" ]] || die "EXE age mismatch"
  [[ "$(manifest_value "${manifest}" pdb_guid)" == "${PDB_GUID}" ]] || die "PDB GUID mismatch"
  [[ "$(manifest_value "${manifest}" pdb_age)" == "${PDB_AGE}" ]] || die "PDB age mismatch"
  [[ "$(manifest_value "${manifest}" debug_match)" == "${DEBUG_MATCH}" ]] || die "Manifest debug-match state is stale"
  [[ "${require_debug_match}" != "yes" || "${DEBUG_MATCH}" == "yes" ]] || die "Candidate EXE and PDB do not match"
}

assert_candidate_path() {
  local candidate="$1"
  [[ -d "${candidate}" ]] || die "Candidate directory is missing: ${candidate}"
  [[ ! -L "${candidate}" ]] || die "Symlinked candidate directory is refused"
  local resolved
  resolved="$(real_path "${candidate}")"
  [[ "${resolved:h}" == "${candidates_root}" ]] || die "Candidate must be a direct child of ${candidates_root}"
  [[ "${resolved:t}" =~ '^[0-9a-f]{64}$' ]] || die "Candidate directory must use its full bundle ID"
  print -r -- "${resolved}"
}

runtime_holders() {
  local output
  local command_status=0
  output="$(lsof -t -- "${game_dir}/engine.exe" "${game_dir}/engine.pdb" "${game_dir}/mimalloc.dll" 2>&1)" || command_status=$?
  if [[ ${command_status} -eq 0 ]]; then
    print -r -- "${output}" | sort -u
    return
  fi
  if [[ ${command_status} -eq 1 ]]; then
    if [[ -z "${output}" ]]; then
      return
    fi
    if print -r -- "${output}" | awk 'NF && $0 !~ /^[0-9]+$/ {invalid = 1} END {exit invalid}'; then
      print -r -- "${output}" | sort -u
      return
    fi
  fi
  die "Cannot prove runtime file ownership with lsof: ${output:-exit ${command_status}}"
}

assert_runtime_idle() {
  local holders
  holders="$(runtime_holders)"
  [[ -z "${holders}" ]] || die "Runtime files are still held by PID(s): ${(j:, :)${(f)holders}}"
  local engine_pids
  local pgrep_status
  if engine_pids="$(pgrep -if '[\\/]KVL[\\/].*engine\.exe|engine\.exe.*[\\/]KVL[\\/]' 2>&1)"; then
    :
  else
    pgrep_status=$?
    [[ ${pgrep_status} -eq 1 && -z "${engine_pids}" ]] || die "Cannot prove engine process state with pgrep: ${engine_pids:-exit ${pgrep_status}}"
  fi
  [[ -z "${engine_pids}" ]] || die "Primary KVL engine process is still present: ${(j:, :)${(f)engine_pids}}"
}

release_lock() {
  [[ "${LOCK_OWNED}" == "yes" ]] || return 0
  [[ -f "${lifecycle_lock}" && ! -L "${lifecycle_lock}" && "$(<"${lifecycle_lock}")" == "$$" ]] || return 0
  unlink "${lifecycle_lock}"
  LOCK_OWNED="no"
}

cleanup_temp_prefix() {
  local prefix="$1"
  local suffix
  [[ -n "${prefix}" ]] || return 0
  for suffix in .exe .pdb .dll; do
    [[ -e "${prefix}${suffix}" ]] && unlink "${prefix}${suffix}" || true
  done
}

finish_lifecycle() {
  local command_status=$?
  trap - EXIT INT TERM HUP
  if [[ -n "${RESTORE_ON_EXIT}" ]]; then
    print -u2 -- "Interrupted install; restoring ${RESTORE_ON_EXIT}"
    if ! restore_triplet "${RESTORE_ON_EXIT}"; then
      print -u2 -- "Automatic restore failed; launch remains blocked by ${pending_install}"
      command_status=1
    fi
  fi
  cleanup_temp_prefix "${STAGE_PREFIX}"
  [[ -e "${pending_install}.tmp.$$" ]] && unlink "${pending_install}.tmp.$$" || true
  release_lock
  exit ${command_status}
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

acquire_lock() {
  assert_fixed_target
  local lock_candidate="${lifecycle_lock}.$$"
  local acquired="no"
  print -r -- "$$" > "${lock_candidate}" || die "Cannot create engine lifecycle lock candidate"
  if ln "${lock_candidate}" "${lifecycle_lock}" 2>/dev/null; then
    acquired="yes"
  elif reclaim_stale_lock && ln "${lock_candidate}" "${lifecycle_lock}" 2>/dev/null; then
    acquired="yes"
  fi
  unlink "${lock_candidate}"
  if [[ "${acquired}" != "yes" ]]; then
    local owner="unknown"
    [[ -f "${lifecycle_lock}" && ! -L "${lifecycle_lock}" ]] && owner="$(<"${lifecycle_lock}")"
    die "Engine lifecycle is busy (lock owner PID ${owner})"
  fi
  LOCK_OWNED="yes"
  recover_pending_install
}

snapshot_installed() {
  inspect_triplet "${game_dir}" engine.exe
  local expected_id="${BUNDLE_ID}"
  local destination="${snapshots_root}/${BUNDLE_ID}"
  if [[ -d "${destination}" ]]; then
    inspect_triplet "${destination}" engine.exe
    verify_manifest "${destination}" any
    [[ "${destination:t}" == "${BUNDLE_ID}" ]] || die "Snapshot directory name does not match its content ID"
    SNAPSHOT_PATH="${destination}"
    print -r -- "Existing immutable snapshot: ${destination}"
    return
  fi
  mkdir -p "${snapshots_root}"
  local incoming="${snapshots_root}/.${BUNDLE_ID}.incoming.$$"
  mkdir "${incoming}"
  cp -p "${game_dir}/engine.exe" "${incoming}/engine.exe"
  cp -p "${game_dir}/engine.pdb" "${incoming}/engine.pdb" || die "Cannot copy installed PDB into the rollback snapshot"
  cp -p "${game_dir}/mimalloc.dll" "${incoming}/mimalloc.dll" || die "Cannot copy installed mimalloc into the rollback snapshot"
  inspect_triplet "${incoming}" engine.exe
  [[ "${BUNDLE_ID}" == "${expected_id}" ]] || die "Snapshot copy failed content verification"
  write_manifest "${incoming}/manifest.tsv" "installed-unknown" "legacy-${DEBUG_MATCH}"
  chmod a-w "${incoming}/engine.exe" "${incoming}/engine.pdb" "${incoming}/mimalloc.dll" "${incoming}/manifest.tsv"
  mv "${incoming}" "${destination}" || die "Cannot publish rollback snapshot"
  chmod a-w "${destination}"
  SNAPSHOT_PATH="${destination}"
  print -r -- "Created immutable snapshot: ${destination}"
}

restore_triplet() {
  local snapshot="$1"
  inspect_triplet "${snapshot}" engine.exe
  verify_manifest "${snapshot}" any
  local expected_id="${BUNDLE_ID}"
  local prefix="${game_dir}/.codex-engine-restore.$$"
  STAGE_PREFIX="${prefix}"
  cp -p "${snapshot}/engine.exe" "${prefix}.exe" || return 1
  cp -p "${snapshot}/engine.pdb" "${prefix}.pdb" || return 1
  cp -p "${snapshot}/mimalloc.dll" "${prefix}.dll" || return 1
  [[ "$(sha256 "${prefix}.exe")" == "${ENGINE_SHA}" ]] || return 1
  [[ "$(sha256 "${prefix}.pdb")" == "${PDB_SHA}" ]] || return 1
  [[ "$(sha256 "${prefix}.dll")" == "${MIMALLOC_SHA}" ]] || return 1
  mv -f "${prefix}.exe" "${game_dir}/engine.exe" || return 1
  mv -f "${prefix}.pdb" "${game_dir}/engine.pdb" || return 1
  mv -f "${prefix}.dll" "${game_dir}/mimalloc.dll" || return 1
  STAGE_PREFIX=""
  inspect_triplet "${game_dir}" engine.exe
  [[ "${BUNDLE_ID}" == "${expected_id}" ]] || return 1
  [[ -e "${pending_install}" ]] && unlink "${pending_install}" || true
}

recover_pending_install() {
  [[ -e "${pending_install}" ]] || return 0
  [[ -f "${pending_install}" && ! -L "${pending_install}" ]] || die "Invalid pending-install marker: ${pending_install}"
  local snapshot
  snapshot="$(<"${pending_install}")"
  [[ -d "${snapshot}" && ! -L "${snapshot}" ]] || die "Pending-install rollback snapshot is missing: ${snapshot}"
  snapshot="$(real_path "${snapshot}")"
  [[ "${snapshot:h}" == "${snapshots_root}" ]] || die "Pending-install marker points outside the snapshot root"
  assert_runtime_idle
  RESTORE_ON_EXIT="${snapshot}"
  restore_triplet "${snapshot}" || die "Cannot recover the interrupted engine install"
  RESTORE_ON_EXIT=""
  print -r -- "Recovered interrupted install from ${snapshot}"
}

install_bundle() {
  local source="$1"
  local prefix="${game_dir}/.codex-engine-stage.$$"
  local expected_id="$(manifest_value "${source}/manifest.tsv" bundle_id)"
  STAGE_PREFIX="${prefix}"
  cp -p "${source}/engine.exe" "${prefix}.exe" || die "Cannot stage candidate engine"
  cp -p "${source}/engine.pdb" "${prefix}.pdb" || die "Cannot stage candidate PDB"
  cp -p "${source}/mimalloc.dll" "${prefix}.dll" || die "Cannot stage candidate mimalloc"
  [[ "$(sha256 "${prefix}.exe")" == "$(manifest_value "${source}/manifest.tsv" engine_sha256)" ]] || die "Staged engine copy failed verification"
  [[ "$(sha256 "${prefix}.pdb")" == "$(manifest_value "${source}/manifest.tsv" pdb_sha256)" ]] || die "Staged PDB copy failed verification"
  [[ "$(sha256 "${prefix}.dll")" == "$(manifest_value "${source}/manifest.tsv" mimalloc_sha256)" ]] || die "Staged mimalloc copy failed verification"
  assert_runtime_idle

  print -r -- "${SNAPSHOT_PATH}" > "${pending_install}.tmp.$$" || die "Cannot write pending-install marker"
  mv -f "${pending_install}.tmp.$$" "${pending_install}" || die "Cannot publish pending-install marker"
  RESTORE_ON_EXIT="${SNAPSHOT_PATH}"
  mv -f "${prefix}.exe" "${game_dir}/engine.exe" || die "Cannot install candidate engine"
  mv -f "${prefix}.pdb" "${game_dir}/engine.pdb" || die "Cannot install candidate PDB"
  mv -f "${prefix}.dll" "${game_dir}/mimalloc.dll" || die "Cannot install candidate mimalloc"
  STAGE_PREFIX=""

  inspect_triplet "${game_dir}" engine.exe
  [[ "${BUNDLE_ID}" == "${expected_id}" ]] || die "Installed bundle failed final verification"
  RESTORE_ON_EXIT=""
  unlink "${pending_install}" || die "Cannot clear verified pending-install marker"
  print -r -- "Installed engine bundle ${BUNDLE_ID}"
}

prepare_candidate() {
  local source="${1:-}"
  local source_commit="${2:-}"
  [[ -n "${source}" && -n "${source_commit}" ]] || die "Usage: $0 prepare <build-bin-dir> <40-hex-source-commit>"
  [[ "${source_commit}" =~ '^[0-9a-f]{40}$' ]] || die "Source commit must be a full lowercase Git OID"
  [[ -d "${source}" && ! -L "${source}" ]] || die "Build output directory is missing or symlinked"
  assert_regular_file "${source}/engine.exe-1"
  assert_regular_file "${source}/engine.pdb"
  assert_regular_file "${source}/mimalloc.dll"
  verify_source_provenance "${source}/engine.exe-1" "${source_commit}"
  inspect_triplet "${source}" engine.exe-1
  [[ "${DEBUG_MATCH}" == "yes" ]] || die "Refusing candidate with a mismatched EXE/PDB pair"
  local expected_id="${BUNDLE_ID}"

  mkdir -p "${candidates_root}"
  local destination="${candidates_root}/${BUNDLE_ID}"
  if [[ -d "${destination}" ]]; then
    inspect_triplet "${destination}" engine.exe
    verify_manifest "${destination}"
    [[ "${destination:t}" == "${BUNDLE_ID}" ]] || die "Candidate directory name does not match its content ID"
    print -r -- "Candidate already prepared: ${destination}"
    return
  fi
  local incoming="${candidates_root}/.${BUNDLE_ID}.incoming.$$"
  mkdir "${incoming}"
  cp -p "${source}/engine.exe-1" "${incoming}/engine.exe"
  cp -p "${source}/engine.pdb" "${incoming}/engine.pdb"
  cp -p "${source}/mimalloc.dll" "${incoming}/mimalloc.dll"
  inspect_triplet "${incoming}" engine.exe
  [[ "${BUNDLE_ID}" == "${expected_id}" ]] || die "Candidate copy failed content verification"
  write_manifest "${incoming}/manifest.tsv" "${source_commit}" "candidate"
  chmod a-w "${incoming}/engine.exe" "${incoming}/engine.pdb" "${incoming}/mimalloc.dll" "${incoming}/manifest.tsv"
  mv "${incoming}" "${destination}"
  chmod a-w "${destination}"
  print -r -- "Prepared candidate ${BUNDLE_ID}: ${destination}"
}

show_status() {
  assert_fixed_target
  if [[ -e "${pending_install}" ]]; then
    print -r -- "target=${game_dir}"
    print -r -- "health=interrupted-install"
    print -r -- "pending_restore=$(<"${pending_install}")"
    return 3
  fi
  inspect_triplet "${game_dir}" engine.exe
  print -r -- "target=${game_dir}"
  print -r -- "bundle_id=${BUNDLE_ID}"
  print -r -- "engine_sha256=${ENGINE_SHA}"
  print -r -- "pdb_sha256=${PDB_SHA}"
  print -r -- "mimalloc_sha256=${MIMALLOC_SHA}"
  print -r -- "exe_pdb=${EXE_GUID}/${EXE_AGE}"
  print -r -- "pdb=${PDB_GUID}/${PDB_AGE}"
  print -r -- "debug_match=${DEBUG_MATCH}"
  print -r -- "health=$([[ "${DEBUG_MATCH}" == yes ]] && print healthy || print legacy-unmatched)"
  local holders
  holders="$(runtime_holders)"
  print -r -- "holders=${holders:-none}"
  [[ "${DEBUG_MATCH}" == "yes" ]] || return 2
}

stage_candidate() {
  local candidate
  candidate="$(assert_candidate_path "${1:-}")"
  inspect_triplet "${candidate}" engine.exe
  verify_manifest "${candidate}"
  [[ "${candidate:t}" == "${BUNDLE_ID}" ]] || die "Candidate directory name does not match its content ID"
  acquire_lock
  assert_runtime_idle
  snapshot_installed
  install_bundle "${candidate}"
}

rollback_snapshot() {
  local requested="${1:-}"
  local legacy_permission="${2:-}"
  [[ -d "${requested}" && ! -L "${requested}" ]] || die "Snapshot directory is missing or symlinked"
  local snapshot
  snapshot="$(real_path "${requested}")"
  [[ "${snapshot:h}" == "${snapshots_root}" ]] || die "Rollback accepts only a direct child of ${snapshots_root}"
  inspect_triplet "${snapshot}" engine.exe
  verify_manifest "${snapshot}" any
  [[ "${snapshot:t}" == "${BUNDLE_ID}" ]] || die "Snapshot directory name does not match its content ID"
  if [[ "${DEBUG_MATCH}" != "yes" && "${legacy_permission}" != "--allow-legacy-unmatched" ]]; then
    die "Snapshot has a mismatched EXE/PDB pair; repeat with --allow-legacy-unmatched for an explicit exact legacy rollback"
  fi
  acquire_lock
  assert_runtime_idle
  snapshot_installed
  install_bundle "${snapshot}"
  [[ "${DEBUG_MATCH}" == "yes" ]] || print -u2 -- "Restored exact legacy runtime with an unmatched PDB"
}

trap finish_lifecycle EXIT INT TERM HUP

case "${1:-status}" in
  status)
    show_status
    ;;
  prepare)
    shift
    prepare_candidate "$@"
    ;;
  stage)
    shift
    stage_candidate "$@"
    ;;
  rollback)
    shift
    rollback_snapshot "$@"
    ;;
  *)
    die "Usage: $0 {status|prepare <build-bin-dir> <source-commit>|stage <candidate-dir>|rollback <snapshot-dir> [--allow-legacy-unmatched]}"
    ;;
esac
