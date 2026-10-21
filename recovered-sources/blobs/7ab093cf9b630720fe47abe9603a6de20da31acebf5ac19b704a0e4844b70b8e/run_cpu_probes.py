from pathlib import Path
import shutil, subprocess, tempfile

source=Path(__file__).resolve().parent
scratch=Path(tempfile.mkdtemp(prefix='corsairs-contact-check-')).resolve()
try:
    for name in ('check.py','prepare_shared.py','frame_check.py','vm_check.py'):
        shutil.copyfile(source/name,scratch/name)
    for name in ('check.py','frame_check.py','vm_check.py'):
        subprocess.run(['python3',str(scratch/name)],check=True,timeout=60)
finally:
    subprocess.run(['/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp',str(scratch)],check=True)
