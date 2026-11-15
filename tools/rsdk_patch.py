import argparse
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import rsdk_scripts as R

ROOT = pathlib.Path(__file__).resolve().parent.parent

CHANGES = [
    ("GlobalCode.bin", 3775, [-425984], [-850000]),
]


def fnv(words):
    h = 2166136261
    for w in words:
        w &= 0xFFFFFFFF
        for b in range(4):
            h ^= (w >> (8 * b)) & 0xFF
            h = (h * 16777619) & 0xFFFFFFFF
    return h


def build(folder):
    lines = ["webport-patch 1"]
    for name, pos, old, new in CHANGES:
        info = R.load_bytecode(pathlib.Path(folder) / name)
        actual = info["code"][pos:pos + len(old)]
        if actual != old:
            sys.exit("%s at %d holds %s, expected %s: this is not the file the patch was written for" % (name, pos, actual, old))
        lines.append("set %s %d %08x %d %s" % (name, pos, fnv(old), len(new), " ".join(str(w) for w in new)))
    return "\n".join(lines) + "\n"


def apply_to_words(code, patch_text, name):
    applied = skipped = 0
    for line in patch_text.splitlines():
        parts = line.split()
        if len(parts) < 6 or parts[0] != "set" or parts[1] != name:
            continue
        pos, h, count = int(parts[2]), int(parts[3], 16), int(parts[4])
        words = [int(x) for x in parts[5:5 + count]]
        if pos < 0 or pos + count > len(code) or fnv(code[pos:pos + count]) != h:
            skipped += 1
            continue
        code[pos:pos + count] = words
        applied += 1
    return applied, skipped


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("bytecode_folder")
    ap.add_argument("--out", default=str(ROOT / "web" / "webport.patch"))
    args = ap.parse_args()
    text = build(args.bytecode_folder)
    pathlib.Path(args.out).write_text(text, encoding="utf-8")
    print("wrote %s (%d changes)" % (args.out, len(CHANGES)))


if __name__ == "__main__":
    main()
