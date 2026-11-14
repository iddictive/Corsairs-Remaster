import pathlib
import re
HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE.parents[2]
SHIP=ROOT / "experiments/native-metal/engine/src/libs/ship/src"
MATH=ROOT / "experiments/native-metal/engine/src/libs/math/include"
ENGINE=SHIP / "phonk_ordnance.cpp"

def function(source, name):
    match = re.search(r'(?m)^(?:constexpr\s+)?(?:void|bool|float|CVECTOR|InterceptorProfile)\s+' + name + r'\s*\(', source)
    if not match:
        raise RuntimeError('runtime function missing: ' + name)
    start = match.start()
    brace = source.index('{', match.end())
    depth = 0
    for pos in range(brace, len(source)):
        if source[pos] == '{':
            depth += 1
        elif source[pos] == '}':
            depth -= 1
            if depth == 0:
                return source[start:pos + 1] + '\n'
    raise RuntimeError('unclosed function: ' + name)


