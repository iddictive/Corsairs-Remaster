#!/usr/bin/env python3
"""Extract real volley target dispatch; prove type-safe dispatch and old UB."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--repo', type=Path, required=True)
args = parser.parse_args()
source = args.repo / 'experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.cpp'
text = source.read_text()
pattern = r'auto \*target = (?:dynamic|static)_cast<SHIP \*>\(core\.GetEntityPointer\(enemyState\.testVolleyTarget\)\);\s*auto \*captain = target \? target->GetACharacter\(\) : nullptr;'
matches = re.findall(pattern, text)
assert len(matches) == 1, f'Expected one actual volley dispatch, got {len(matches)}'
actual = matches[0]
assert 'dynamic_cast' in actual, 'Actual source is not fixed'
program = r'''
#include <cassert>
#include <string_view>
struct ATTRIBUTES {};
struct Entity { virtual ~Entity() = default; };
struct SHIP : Entity {
    int calls = 0;
    ATTRIBUTES character;
    virtual ATTRIBUTES *GetACharacter() { ++calls; return &character; }
};
struct MODEL : Entity {};
struct Core {
    Entity *entity = nullptr;
    Entity *GetEntityPointer(int) { return entity; }
};
struct State { int testVolleyTarget = 42; };
void dispatch(Core &core, State &enemyState, SHIP *expected) {
    // Two statements extracted verbatim from actual production source:
    EXTRACTED
    assert(target == expected);
    assert(captain == (expected ? &expected->character : nullptr));
}
int main(int argc, char **argv) {
    Core core;
    State enemyState;
    SHIP ship;
    MODEL fortModel;
    if (argc == 2 && std::string_view(argv[1]) == "model") {
        core.entity = &fortModel;
        dispatch(core, enemyState, nullptr);
        return 0;
    }
    core.entity = &ship;
    dispatch(core, enemyState, &ship);
    assert(ship.calls == 1);
    core.entity = &fortModel;
    dispatch(core, enemyState, nullptr);
    assert(ship.calls == 1);
    core.entity = nullptr; // Missing/stale ID resolves to null, not dangling pointer.
    dispatch(core, enemyState, nullptr);
    assert(ship.calls == 1);
}
'''
cxx = shlex.split(os.environ.get('CXX', 'g++'))
flags = ['-std=c++20', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined,vptr', '-fno-sanitize-recover=all']
with tempfile.TemporaryDirectory(prefix='corsairs-volley-type-') as tmp:
    root = Path(tmp)
    for label, statements in [('fixed', actual), ('old-static-cast', actual.replace('dynamic_cast<SHIP *>', 'static_cast<SHIP *>'))]:
        cpp = root / (label + '.cpp')
        exe = root / label
        cpp.write_text(program.replace('EXTRACTED', statements))
        subprocess.run(cxx + flags + [str(cpp), '-o', str(exe)], check=True)
        result = subprocess.run([str(exe)] + (['model'] if label != 'fixed' else []), text=True, capture_output=True)
        if label == 'fixed':
            assert result.returncode == 0, result.stderr
            assert not result.stderr, result.stderr
            print('PASS actual source: valid SHIP dispatch once, fort MODEL rejected, null stale-ID rejected')
        else:
            assert result.returncode != 0, 'Old mutation unexpectedly succeeded'
            assert 'runtime error:' in result.stderr and ('downcast' in result.stderr or 'member call' in result.stderr), result.stderr
            print('PASS old static_cast mutation rejected by UBSan:')
            print(result.stderr.strip())
