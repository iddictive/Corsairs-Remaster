"""Replay frozen CPU contact witnesses; never builds/stages or controls the game."""
from pathlib import Path
import hashlib
import json
import subprocess

out = Path(__file__).resolve().parent
native = Path.cwd() / 'experiments/native-metal'
outputs = []
commands = []
for label in ('before', 'after'):
    source = (out / (label + '.hpp')).read_text()
    needle = 'namespace frame_detail\n{'
    assert source.count(needle) == 1
    source = source.replace(needle, needle + '\ninline std::size_t exactCalls = 0;')
    needle = '    double lo, hi;'
    assert source.count(needle) == 1
    source = source.replace(needle, '    ++exactCalls;\n' + needle)
    (out / 'query.hpp').write_text(source)
    command = ['clang++', '-std=c++20', '-O2', '-I' + str(out / 'inputs'),
               str(out / 'probe.cpp'), '-o', str(out / label)]
    commands.append(command)
    result = subprocess.run(command, text=True, capture_output=True)
    (out / (label + '-build.txt')).write_text(result.stdout + result.stderr)
    result.check_returncode()
    result = subprocess.run([str(out / label), '100'], text=True, capture_output=True)
    (out / (label + '-result.txt')).write_text(result.stdout)
    (out / (label + '-cost.txt')).write_text(result.stderr)
    result.check_returncode()
    outputs.append(result.stdout)
    print(label, result.stderr, end='')
assert outputs[0] == outputs[1], 'physical witness, impulse, energy or motion changed'
command = ['clang++', '-std=c++20', '-O1', '-fsanitize=address,undefined',
           '-I' + str(out / 'inputs'), str(out / 'probe.cpp'), '-o', str(out / 'sanitized')]
commands.append(command)
subprocess.run(command, check=True)
result = subprocess.run([str(out / 'sanitized')], text=True, capture_output=True)
(out / 'sanitized-result.txt').write_text(result.stdout + result.stderr)
result.check_returncode()
receipt = {
    'meaning': 'Frozen CPU source checks only; no engine build, delivery or player FPS acceptance',
    'exact_outputs_equal': True,
    'commands': commands,
    'run_arguments': ['before 100', 'after 100', 'sanitized'],
    'compiler': subprocess.check_output(['clang++', '--version'], text=True),
    'inputs_sha256': {str(p.relative_to(out)): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in [out / 'before.hpp', out / 'after.hpp', out / 'probe.cpp',
                                out / 'run_probe.py', *sorted((out / 'inputs').rglob('*.hpp'))]},
}
(out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print('PASS exact before/after witnesses, response and integrated motion; ASan/UBSan clean')
