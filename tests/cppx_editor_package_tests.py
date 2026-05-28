#!/usr/bin/env python3

from __future__ import annotations

import json
import pathlib
import sys


def fail(message: str) -> bool:
    print(message, file=sys.stderr)
    return False


def main() -> int:
    root = pathlib.Path("tools/editor/cppx")
    package_path = root / "package.json"
    grammar_path = root / "syntaxes/cppx.tmLanguage.json"
    config_path = root / "language-configuration.json"

    package = json.loads(package_path.read_text())
    grammar = json.loads(grammar_path.read_text())
    json.loads(config_path.read_text())

    languages = package.get("contributes", {}).get("languages", [])
    grammars = package.get("contributes", {}).get("grammars", [])
    ok = True

    if not languages:
        ok = fail("cppx package must contribute a language")
    else:
        language = languages[0]
        if language.get("id") != "cppx":
            ok = fail("cppx language id must be cppx")
        if language.get("extensions") != [".cppx", ".hx"]:
            ok = fail("cppx language extensions must be .cppx and .hx")
        if not (root / language.get("configuration", "")).exists():
            ok = fail("cppx language configuration path is missing")

    if not grammars:
        ok = fail("cppx package must contribute a grammar")
    else:
        contributed = grammars[0]
        if contributed.get("scopeName") != "source.cppx":
            ok = fail("cppx grammar scope must be source.cppx")
        if not (root / contributed.get("path", "")).exists():
            ok = fail("cppx grammar path is missing")
        if contributed.get("embeddedLanguages", {}).get(
            "meta.embedded.block.cppx"
        ) != "cpp":
            ok = fail("cppx embedded expression language must map to cpp")

    if grammar.get("scopeName") != "source.cppx":
        ok = fail("cppx tmLanguage scopeName must be source.cppx")
    if grammar.get("fileTypes") != ["cppx", "hx"]:
        ok = fail("cppx tmLanguage fileTypes must be cppx and hx")
    repository = grammar.get("repository", {})
    for rule in [
        "jsx-opening-tag",
        "jsx-closing-tag",
        "jsx-attribute",
        "jsx-expression",
    ]:
        if rule not in repository:
            ok = fail(f"cppx grammar missing {rule}")

    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
