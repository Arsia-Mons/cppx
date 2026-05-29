#!/usr/bin/env python3
from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]
SCAN_FILES = [ROOT / "CMakeLists.txt"]
SCAN_DIRS = [ROOT / "src"]
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp"}
BLOCKED = ("clay", "ui/focus", "ui/primitives")

# src/ui/ is the SDL-free generic toolkit (architecture.md §3, §6). The ONLY
# seams across the SDL boundary are the geometry mesh contract and the single
# MeasureTextFn function pointer — neither names an SDL/SDL_ttf type. So no file
# under src/ui/ may include SDL/SDL_ttf or reference an SDL_/TTF_ symbol. We scan
# code only (comments are stripped) so prose mentioning "SDL_ttf" stays legal.
UI_SDL_FREE_DIR = ROOT / "src/ui"
SDL_INCLUDE_RE = re.compile(r'#\s*include\s*[<"]\s*(SDL3?/|SDL[._]|SDL_ttf)', re.IGNORECASE)
SDL_SYMBOL_RE = re.compile(r'\b(?:SDL|TTF)_[A-Za-z][A-Za-z0-9_]*')


def strip_comments(text: str) -> str:
    """Remove // line comments and /* */ block comments and string/char literals
    so identifier scans only see real code. Good enough for a structural guard."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        two = text[i:i + 2]
        if two == "//":
            j = text.find("\n", i)
            i = n if j == -1 else j
        elif two == "/*":
            j = text.find("*/", i + 2)
            i = n if j == -1 else j + 2
        elif c in ('"', "'"):
            quote = c
            i += 1
            while i < n:
                if text[i] == "\\":
                    i += 2
                    continue
                if text[i] == quote:
                    i += 1
                    break
                i += 1
        else:
            out.append(c)
            i += 1
    return "".join(out)
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

        if is_under(path, UI_SDL_FREE_DIR) and path.suffix in SOURCE_SUFFIXES:
            code = strip_comments(path.read_text(encoding="utf-8"))
            include_hit = SDL_INCLUDE_RE.search(code)
            if include_hit:
                failures.append(
                    f"{rel}: src/ui/ is SDL-free — illegal SDL/SDL_ttf include "
                    f"{include_hit.group(0)!r}"
                )
            symbol_hit = SDL_SYMBOL_RE.search(code)
            if symbol_hit:
                failures.append(
                    f"{rel}: src/ui/ is SDL-free — illegal SDL_/TTF_ symbol "
                    f"{symbol_hit.group(0)!r} (only the MeasureTextFn pointer + "
                    "geometry mesh seam may cross the SDL boundary)"
                )

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
