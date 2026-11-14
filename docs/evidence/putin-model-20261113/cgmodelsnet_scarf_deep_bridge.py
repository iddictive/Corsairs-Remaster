"""Render the wider body transition with matching bridge material."""
from pathlib import Path

here = Path(__file__).resolve().parent
path = here / 'cgmodelsnet_scarf_bridge_trial.py'
source = path.read_text()
for old, new in [
    ('cgmodelsnet-body-scarf-fullring', 'cgmodelsnet-body-scarf-deep'),
    ('cgmodelsnet-scarf-bridge-', 'cgmodelsnet-scarf-deep-bridge-'),
]:
    assert source.count(old) >= 1
    source = source.replace(old, new)
exec(compile(source, str(path), 'exec'), {'__file__': str(path), '__name__': '__main__'})
