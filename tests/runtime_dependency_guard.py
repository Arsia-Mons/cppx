#!/usr/bin/env python3
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
SCAN_FILES = [ROOT / "CMakeLists.txt"]
SCAN_DIRS = [ROOT / "src"]
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp"}
BLOCKED = ("clay", "ui/focus", "ui/primitives")


def iter_files():
    for path in SCAN_FILES:
        yield path
    for directory in SCAN_DIRS:
        for path in directory.rglob("*"):
            if path.is_file() and path.suffix in SOURCE_SUFFIXES:
                yield path


def main() -> int:
    failures = []
    for path in iter_files():
        rel = path.relative_to(ROOT).as_posix()
        haystack = (rel + "\n" + path.read_text(encoding="utf-8")).lower()
        for term in BLOCKED:
            if term in haystack:
                failures.append(f"{rel}: contains blocked UI dependency term {term!r}")
                break

    if failures:
        print("runtime dependency guard failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
