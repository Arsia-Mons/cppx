#!/usr/bin/env python3
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
SCAN_FILES = [ROOT / "CMakeLists.txt"]
SCAN_DIRS = [ROOT / "src"]
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp"}
BLOCKED = ("clay", "ui/focus", "ui/primitives")
SOURCE_BLOCKED = (
    "ui/runtime/components.h",
    "runtime/components.h",
    "react_retained_component_begin",
    "retainednodescope",
    "begin_retained_frame",
    "begin_retained_tree_frame",
)
DELETED_RUNTIME_FILES = (
    ROOT / "src/ui/runtime/components.h",
    ROOT / "src/ui/runtime/components.cpp",
)


def iter_files():
    for path in SCAN_FILES:
        yield path
    for directory in SCAN_DIRS:
        for path in directory.rglob("*"):
            if path.is_file() and path.suffix in SOURCE_SUFFIXES:
                yield path


def is_under(path: Path, directory: Path) -> bool:
    try:
        path.relative_to(directory)
        return True
    except ValueError:
        return False


def main() -> int:
    failures = []
    for path in iter_files():
        rel = path.relative_to(ROOT).as_posix()
        haystack = (rel + "\n" + path.read_text(encoding="utf-8")).lower()
        for term in BLOCKED:
            if term in haystack:
                failures.append(f"{rel}: contains blocked UI dependency term {term!r}")
                break
        if is_under(path, ROOT / "src"):
            for term in SOURCE_BLOCKED:
                if term in haystack:
                    failures.append(
                        f"{rel}: contains blocked immediate retained API term {term!r}"
                    )
                    break

    cmake_text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8").lower()
    if "src/ui/runtime/components.cpp" in cmake_text:
        failures.append(
            "CMakeLists.txt: old immediate retained components.cpp is still linked"
        )

    for deleted_file in DELETED_RUNTIME_FILES:
        if deleted_file.exists():
            failures.append(
                f"{deleted_file.relative_to(ROOT)}: old immediate retained API file still exists"
            )

    if failures:
        print("runtime dependency guard failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
