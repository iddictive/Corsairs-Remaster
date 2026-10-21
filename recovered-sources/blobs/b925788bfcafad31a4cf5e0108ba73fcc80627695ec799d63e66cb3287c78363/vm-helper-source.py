import re

def replace_body(root, path, name, new):
    p = root / "PROGRAM" / path
    source = p.read_text()
    match = re.search(r'\b(?:void|int|bool|string|float) ' + name + r'\([^)]*\)(?:\s|//[^\n]*\n)*\{', source, re.I)
    if not match:
        raise ValueError(f"missing fixture boundary {path}:{name}")
    start = match.end()
    end, depth = start, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    p.write_text(source[:start] + "\n" + new + "\n" + source[end-1:])
