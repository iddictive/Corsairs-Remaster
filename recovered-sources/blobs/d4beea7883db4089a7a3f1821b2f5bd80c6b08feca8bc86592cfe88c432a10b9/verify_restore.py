"""One-off normal restore/upgrade replay; no native build or player state."""
import hashlib
import io
import json
import pathlib
import shutil
import subprocess
import sys
import tarfile
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT = pathlib.Path(__file__).parent
PAYLOAD = ROOT / 'dist/Corsairs-Metal-build-inputs.tar.gz'
APP = pathlib.Path('/Applications/Corsairs Iddictive Remaster.app')
sys.path.insert(0, str(ROOT / 'tools'))
from metal_storage import ensure_space

# Initial combined 160GiB admission passed before the first restore. Packaging
# keeps its own reservation; this replay replaces the deleted <=32GiB copy.
resumed = '--resume' in sys.argv
ensure_space(1024 ** 3 if resumed else 32 * 1024 ** 3, OUT)
resume = json.loads((OUT / 'resume-state.json').read_text()) if resumed else None
scratch = pathlib.Path(resume['scratch']) if resumed else pathlib.Path(tempfile.mkdtemp(prefix='corsairs-rebuild-')).resolve()
result = {'scope': 'Normal CLI restore from actual installed app and exported inputs; no CPP/GPU/player SAVE',
          'source_commit': subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip(),
          'payload_sha256': hashlib.sha256(PAYLOAD.read_bytes()).hexdigest(),
          'payload_bytes': PAYLOAD.stat().st_size}
try:
    if resumed:
        clone = pathlib.Path(resume['clone']).resolve()
        prior = json.loads((OUT / 'invalid-path-alias.json').read_text())
        assert prior['restore_exit'] == 0
        result.update(command=prior['command'], restore_exit=0, resumed_without_restore_rerun=True,
                      diagnostic_path_correction=resume)
    else:
        clone = scratch / 'source copy'
        clone.mkdir()
        source = subprocess.check_output(['git', '-C', str(ROOT), 'archive', 'HEAD'])
        with tarfile.open(fileobj=io.BytesIO(source)) as packed:
            packed.extractall(clone, filter='data')
        for name in ('prepare_metal_inputs.py', 'restore_metal_runtime.py', 'capture_metal_build.py'):
            current = ROOT / 'tools' / name
            shutil.copy2(current, clone / 'tools' / name)
            (OUT / name).write_bytes(current.read_bytes())
        with tarfile.open(PAYLOAD) as packed:
            packed.extractall(clone / 'experiments/native-metal', filter='data')
        command = [sys.executable, str(clone / 'tools/restore_metal_runtime.py'), '--app', str(APP)]
        ran = subprocess.run(command, cwd=clone, capture_output=True, text=True, timeout=120)
        (OUT / 'restore.stdout').write_text(ran.stdout)
        (OUT / 'restore.stderr').write_text(ran.stderr)
        result.update(command=command, restore_exit=ran.returncode)
        assert ran.returncode == 0, ran.stderr[-2000:]
    sys.path.insert(0, str(clone / 'tools'))
    import delivery_state
    import gameplay_sources
    import prepare_native_movies
    import prepare_phonk_music
    runtime = clone / 'experiments/native-metal/.cache/runtime'
    state = delivery_state.DeliveryState(runtime)
    assert '@engine' not in state.files
    for name, row in state.files.items():
        assert delivery_state.digest(state.path(name).read_bytes()) == row['sha256'], name
    result['resource_receipts_verified'] = len(state.files)
    result['empty_development_save'] = list((runtime / 'SAVE').iterdir()) == []
    assert result['empty_development_save']
    movies = prepare_native_movies.prepare(runtime, ffmpeg='/deliberately-unavailable-ffmpeg')
    result['prepared_movies_without_ffmpeg'] = len(movies)
    def reject_download(*args, **kwargs):
        raise AssertionError('Unexpected music download')
    prepare_phonk_music.urllib.request.urlopen = reject_download
    prepare_phonk_music.prepare()
    result['music_without_network'] = True
    name = 'PROGRAM/sea_ai/sea.c'
    sources, _ = gameplay_sources.read([name])
    before, legacy, encoding = sources[name]
    incoming = before + b'\r\n// isolated reconstruction upgrade\r\n'
    changes = gameplay_sources.prepare_delivery(runtime, ({name: (incoming, legacy, encoding)}, {}))
    delivery_state.transact(changes)
    after = delivery_state.DeliveryState(runtime)
    assert after.path(name).read_bytes() == incoming
    assert after.files[name]['sha256'] == delivery_state.digest(incoming)
    result['first_source_upgrade'] = 'accepted and committed through canonical transaction'
    after.path(name).write_bytes(incoming + b'// unknown local edit\r\n')
    try:
        gameplay_sources.prepare_delivery(runtime, ({name: (incoming, legacy, encoding)}, {}))
    except RuntimeError as error:
        assert 'Unknown delivered-file edit' in str(error)
        result['unknown_edit_negative'] = str(error)
    else:
        raise AssertionError('Unknown restored-file edit was admitted')
    missing = clone / 'experiments/native-metal/inputs/native/puerto-rico-city-originals/RESOURCE/MODELS/Islands/PuertoRico/PuertoRico.gm'
    missing.rename(missing.with_name(missing.name + '.missing-probe'))
    negative = subprocess.run([sys.executable, str(clone / 'tools/prepare_metal_inputs.py'), '--check'],
                              cwd=clone, capture_output=True, text=True, timeout=30)
    (OUT / 'missing-original.stderr').write_text(negative.stderr)
    assert negative.returncode != 0 and 'Input changed: native/puerto-rico-city-originals/' in negative.stderr
    result['missing_original_negative'] = 'rejected by input check before any build/stage'
    result['status'] = 'passed'
except BaseException as error:
    result.update(status='failed', error=repr(error))
    raise
finally:
    (OUT / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    subprocess.run(['/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp', str(scratch)], check=True)
print(json.dumps(result, indent=2))
