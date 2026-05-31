#!/usr/bin/env python3
# ui_style_invariants_guard: pins the style AUTHORING optionality model.
#
# The from-first-principles styling design (docs/retained-ui/styling-render-system-design.md
# §3) bans value-space sentinels in the authoring overlay: presence is carried by
# Opt<T>{set, value} and read via Opt::set ONLY — never by comparing a value against
# a magic number. (The resolved-output VisualStyle is allowed to read presence via
# value cues like background.a==0, because those are renderer-read outputs, not
# authoring inputs — so this guard scopes to style_patch.h, the one authoring overlay.)
#
# Invariants enforced here (a regression in any of these breaks the design):
#   1. Opt<T> is the {bool set; T value;} shape.
#   2. Every StylePatch member is an Opt<...> (no bare-value fields used as sentinels).
#   3. apply() gates every field on `.set` ONLY — no value-equality
#      presence test (no `==`/`!=` against a sentinel) decides whether to write a field.
#
# Structural, comment-stripped scan; no compiler needed.

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
STYLE_PATCH = ROOT / "src/ui/style/style_patch.h"


def strip_comments(text: str) -> str:
    out = []
    i, n = 0, len(text)
    while i < n:
        two = text[i:i + 2]
        if two == "//":
            j = text.find("\n", i)
            i = n if j == -1 else j
        elif two == "/*":
            j = text.find("*/", i + 2)
            i = n if j == -1 else j + 2
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def body_of(code: str, decl_regex: str) -> str:
    """Return the brace-balanced body following the first match of decl_regex."""
    m = re.search(decl_regex, code)
    if not m:
        return ""
    i = code.find("{", m.end())
    if i == -1:
        return ""
    depth = 0
    for j in range(i, len(code)):
        if code[j] == "{":
            depth += 1
        elif code[j] == "}":
            depth -= 1
            if depth == 0:
                return code[i + 1:j]
    return ""


def main() -> int:
    failures = []

    if not STYLE_PATCH.exists():
        print(f"ui_style_invariants_guard: missing {STYLE_PATCH}", file=sys.stderr)
        return 1

    code = strip_comments(STYLE_PATCH.read_text(encoding="utf-8"))

    # 1. Opt<T> shape: must be `struct Opt { bool set ...; T value ...; }`.
    opt_body = body_of(code, r"struct\s+Opt\b")
    if not opt_body:
        failures.append("Opt<T> struct not found in style_patch.h")
    else:
        if not re.search(r"\bbool\s+set\b", opt_body):
            failures.append("Opt<T> lost its `bool set` presence flag")
        if not re.search(r"\bT\s+value\b", opt_body):
            failures.append("Opt<T> lost its `T value` payload")

    # 2. Every StylePatch member is an Opt<...>.
    patch_body = body_of(code, r"struct\s+StylePatch\b")
    if not patch_body:
        failures.append("struct StylePatch not found in style_patch.h")
    else:
        # type name [= default] ;  — capture members with or without an initializer,
        # because a value sentinel typically hides in a default (e.g. `float r = -1.f;`).
        member_re = re.compile(
            r"^\s*([A-Za-z_][\w:<>\s]*?)\s+([A-Za-z_]\w*)\s*(?:=[^;]*)?;", re.M
        )
        members = list(member_re.finditer(patch_body))
        if not members:
            failures.append("StylePatch appears to have no fields (parse failure?)")
        for m in members:
            field_type = m.group(1).strip()
            field_name = m.group(2)
            if not field_type.startswith("Opt<"):
                failures.append(
                    f"StylePatch.{field_name} has non-Opt type {field_type!r} "
                    "— authoring presence must be Opt<T>::set, never a value sentinel"
                )

    # 3. apply() gates fields on `.set` only — no value-equality presence test.
    for fn_regex, fn_name in (
        (r"\bapply\s*\(\s*VisualStyle", "apply"),
    ):
        fn_body = body_of(code, fn_regex)
        if not fn_body:
            failures.append(f"{fn_name}() not found in style_patch.h")
            continue
        # Every `if (...)` condition in these functions must test `.set`.
        for cond in re.finditer(r"\bif\s*\(([^)]*)\)", fn_body):
            text = cond.group(1)
            if ".set" not in text:
                failures.append(
                    f"{fn_name}(): condition `if ({text.strip()})` does not gate on "
                    "`.set` — value-space sentinel presence test is banned"
                )
        # No equality/inequality comparison anywhere in the presence logic.
        if re.search(r"[^=!<>]==|!=", fn_body):
            failures.append(
                f"{fn_name}(): contains an ==/!= comparison — presence must be "
                "decided by Opt::set, not by comparing a value to a sentinel"
            )

    if failures:
        print("ui_style_invariants_guard failed:", file=sys.stderr)
        for f in failures:
            print(f"  {f}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
