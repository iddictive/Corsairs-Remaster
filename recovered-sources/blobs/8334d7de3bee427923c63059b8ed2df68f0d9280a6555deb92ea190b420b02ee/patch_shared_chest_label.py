#!/usr/bin/env python3
"""Label only the officer-supply chest in the player's current cabin."""
import argparse
import subprocess
from pathlib import Path

from runtime_script_patch import atomic_write, sha256

PROJECT = Path(__file__).resolve().parents[1]
RELATIVE = "PROGRAM/interface/itemsbox.c"
BASE_SHA256 = "5b35060ae02e96089d9e20991897c2309c838eb2926ef4f67be0a533fabfc029"
OLD = '\tSetFormatedText("MAIN_CAPTION", XI_ConvertString(sGetInterfaceTypeStr("titleExchangeItems", "titleItemsBox", "titleDeadItems","titleBarrel")));'
NEW = OLD + '''
	if (sGetInterfaceType() == INTERFACETYPE_CHEST && pchar.location == Get_My_Cabin())
	{
		if (CheckAttribute(pchar, "boxname"))
		{
			if (pchar.boxname == "box1") SetFormatedText("MAIN_CAPTION", "Общий сундук");
		}
	}'''


def transform(source):
    old = OLD.replace("\n", "\r\n").encode()
    new = NEW.replace("\n", "\r\n").encode()
    if sha256(source) != BASE_SHA256 or source.count(old) != 1:
        raise RuntimeError("unknown itemsbox revision; review before applying")
    return source.replace(old, new, 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("check", "apply"))
    parser.add_argument("--runtime", choices=("native-storm", "native-metal"), default="native-storm")
    args = parser.parse_args()
    root = PROJECT / "experiments" / args.runtime / ".cache"
    path = root / "runtime" / RELATIVE
    current = path.read_bytes()
    if NEW.replace("\n", "\r\n").encode() in current:
        original = current.replace(NEW.replace("\n", "\r\n").encode(), OLD.encode(), 1)
        if sha256(original) != BASE_SHA256:
            raise RuntimeError("unknown labeled itemsbox revision")
        print("Shared chest label: already applied")
        return
    candidate = transform(current)
    if args.action == "apply":
        processes = subprocess.run(["ps", "-axo", "comm="], capture_output=True, text=True, check=True)
        if any(line.endswith(("/native-engine", "/metal-engine")) for line in processes.stdout.splitlines()):
            raise RuntimeError("close the game before applying the chest label")
        if path.read_bytes() != current:
            raise RuntimeError("itemsbox changed concurrently")
        atomic_write(path, candidate)
        assert path.read_bytes() == candidate
    print("Shared chest label: " + ("applied" if args.action == "apply" else "prepared"))


if __name__ == "__main__":
    main()
