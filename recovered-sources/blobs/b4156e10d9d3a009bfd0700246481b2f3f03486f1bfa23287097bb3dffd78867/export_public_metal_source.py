#!/usr/bin/env python3
"""Export the current native build without private history or player state."""
import argparse
import ast
from pathlib import Path
import re
import shutil
import subprocess

PROJECT = Path(__file__).resolve().parents[1]


def export(target):
    if target.exists():
        raise RuntimeError('Output exists; refusing overwrite')
    tracked = set(subprocess.check_output(['git', '-C', str(PROJECT), 'ls-files'], text=True).splitlines())
    # Source consumers include native audio/patch adapters, never its old runtime.
    selected = {name for name in tracked if name.startswith(('experiments/native-metal/', 'experiments/native-storm/'))
                and not name.endswith('.md') and '/.cache/' not in name}
    selected.update(str(p.relative_to(PROJECT)) for p in (PROJECT / 'experiments/native-metal/branding').rglob('*') if p.is_file())
    selected.update({'experiments/native-metal/external-url.patch', 'experiments/native-metal/public_launcher.py'})
    tools = {'prepare_metal_inputs', 'restore_metal_runtime', 'sync_metal_gameplay',
             'metal_graphics_settings', 'metal_menu_branding', 'patch_metal_portobello_ground',
             'patch_metal_portobello_shore', 'export_metal_app'}
    scanned = set()
    while tools - scanned:
        module = sorted(tools - scanned)[0]
        path = PROJECT / 'tools' / (module + '.py')
        if not path.is_file():
            raise RuntimeError(f'Missing module: {module}')
        selected.add(path.relative_to(PROJECT).as_posix())
        source = path.read_text()
        for node in ast.walk(ast.parse(source)):
            names = [node.module] if isinstance(node, ast.ImportFrom) else ([n.name for n in node.names] if isinstance(node, ast.Import) else [])
            for name in names:
                if name and (PROJECT / 'tools' / (name.split('.')[0] + '.py')).is_file():
                    tools.add(name.split('.')[0])
        for name in re.findall(r'[\w-]+\.py', source):
            if (PROJECT / 'tools' / name).is_file():
                tools.add(name[:-3])
        scanned.add(module)
    target.mkdir(parents=True)
    for name in sorted(selected):
        source = PROJECT / name
        if not source.is_file() or source.name in {'AGENTS.md', 'TASKS.md'} or '__pycache__' in source.parts:
            continue
        destination = target / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
        if source.suffix in {'.py', '.sh', '.txt', '.json', '.patch', '.mm', '.cpp', '.h', '.hpp'} or source.name in {'CMakeLists.txt', '.gitignore'}:
            try:
                text = destination.read_text()
            except UnicodeDecodeError:
                continue
            # Public builds own their disposable patch transaction, no private Codex helper.
            text = text.replace("subprocess.run(['/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp', str(temporary)], check=True, stdout=subprocess.DEVNULL)", 'shutil.rmtree(temporary)')
            text = text.replace("subprocess.run(['/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp',str(root)],check=True,stdout=subprocess.DEVNULL)", 'shutil.rmtree(root)')
            text = text.replace('/REQUIRED_EXTERNAL_INPUT/Corsairs', '/REQUIRED_EXTERNAL_INPUT/Corsairs').replace('/REQUIRED_EXTERNAL_INPUT', '/REQUIRED_EXTERNAL_INPUT')
            destination.write_text(text)
    (target / '.gitignore').write_text('experiments/*/.cache/\nexperiments/native-metal/inputs/\n__pycache__/\n*.pyc\n.DS_Store\ndist/\n')
    (target / 'README.md').write_text('''# Corsairs — Iddictive Remaster for Mac

Native Apple Silicon / Metal build of City of Abandoned Ships, GPK 1.3.2 AT + ReConstruction 1.4.1, with rendering and gameplay changes.

[Download the app and read the case](https://iddictive.us/cases/corsairs-native-metal/)

**Experimental build:** a September 30 player launch hung during CoreAudio/TCC initialization alongside macOS WindowServer watchdog resets. Stability is unresolved; this is not a stable release.

## Play

Requires an Apple Silicon Mac and macOS 15 or newer. Unzip the app, move it to Applications and open it. Python and runtime libraries are bundled; Xcode and Homebrew are not needed to play. Saves and settings live separately in `~/Library/Application Support/Iddictive Corsairs` and survive replacement of the app. The app is ad-hoc signed, not notarized.

## Build

Requires Apple Silicon, macOS 15+, Xcode command-line tools, and Python 3.10+.

1. Install the downloadable app in `/Applications/Corsairs Iddictive Remaster.app`; it supplies the game data for development staging.
2. Download `Corsairs-Metal-build-inputs.tar.gz` from [the release](https://github.com/iddictive/Corsairs-Remaster/releases/latest) and extract it into `experiments/native-metal` (creating `inputs/`). The manifest pins engine source, headers, dependencies, tools and reviewed originals.
3. Run `experiments/native-metal/run.sh --stage-only`, then `experiments/native-metal/run.sh --launch-installed` to play your development candidate.

Development staging creates independent empty saves. It does not import or overwrite your installed application's saves. `experiments/native-storm` contains source adapters required by the Metal build; its old runtime is not distributed or supported.

Rendering and gameplay changes remain subject to real-scene playtesting. Build and packaging checks do not prove full gameplay stability. Original engine source and dependency notices are included in the pinned build inputs; this repository has no blanket license granting ownership of third-party game content.
''')
    for path in target.rglob('*'):
        if not path.is_file():
            continue
        if path.suffix == '.py':
            compile(path.read_bytes(), str(path), 'exec')
        if path.stat().st_size < 10 * 1024 * 1024 and b'/REQUIRED_EXTERNAL_INPUT' in path.read_bytes():
            raise RuntimeError(f'Private machine path: {path.relative_to(target)}')
    print(f'Exported {len(selected)} selected source files to {target}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    export(parser.parse_args().output.resolve())
