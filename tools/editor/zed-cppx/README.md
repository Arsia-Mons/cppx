# CPPX for Zed

Zed extension that provides syntax highlighting for `.cppx` and `.hx` files —
C++ with JSX-like tags, as used in this repo's retained UI authoring.

The extension wraps a small tree-sitter grammar (`grammars/cppx/`) that
recognises JSX tags, attributes, and brace expressions. Everything else in the
file is opaque `cpp_text` that Zed injects `tree-sitter-cpp` into, so you get
full C++ highlighting outside the JSX bits.

## Install (dev extension)

This extension is loaded directly from this directory; nothing is published to
the Zed extension registry.

Zed's extension manifest only accepts git URLs for grammars — there is no
`path = "..."` option. To avoid publishing the grammar to a remote (and to
avoid a nested `.git` inside this repo, which would turn `grammars/cppx/`
into a submodule pointer), we use a *sidecar* git repo on the local
filesystem. `install.sh` syncs the grammar source into that sidecar,
commits, and rewrites the `rev` in `extension.toml`.

1. Run the install script. This creates / refreshes the sidecar repo.
   - macOS / Linux: `./tools/editor/zed-cppx/install.sh`
     → `~/.cache/zed-cppx-grammar` (override with `CPPX_SIDECAR_DIR`)
   - Windows (PowerShell): `tools\editor\zed-cppx\install.ps1`
     → `%LOCALAPPDATA%\zed-cppx-grammar` (override with `$env:CPPX_SIDECAR_DIR`)
2. In Zed, run the `zed: install dev extension` command (Command Palette
   → search for it).
3. Pick `tools/editor/zed-cppx/` in this repository.

The install script rewrites `[grammars.cppx]` in `extension.toml` with a
machine-local `file://` URL and the current sidecar commit SHA — those
edits are per-machine and aren't meant to be committed.

## Regenerating the parser

If you edit `grammar-source/grammar.js`, regenerate the parser and re-run
`install.sh` before reinstalling the extension in Zed:

```sh
cd tools/editor/zed-cppx/grammar-source
npx tree-sitter-cli@0.22 generate
cd ..
./install.sh        # or .\install.ps1 on Windows
```

This refreshes `src/parser.c`, `src/grammar.json`, and `src/node-types.json`,
then syncs the new files into the sidecar repo and bumps `rev` in
`extension.toml`.

## Layout

```text
extension.toml               # Zed extension manifest (rewritten by install.sh)
install.sh                   # sync grammar -> sidecar git repo, bump rev
languages/cppx/
  config.toml                # filetypes, comments, brackets
  highlights.scm             # JSX tag / attribute / string colouring
  injections.scm             # inject tree-sitter-cpp into cpp_text/expr_text
  brackets.scm               # bracket-matching pairs
  indents.scm                # auto-indent triggers
grammar-source/              # source of truth for the grammar; tracked here
  grammar.js
  src/parser.c               # generated; checked in for install convenience
grammars/cppx/               # populated by Zed on install (gitignored); do
                             # not edit, install.sh will wipe it
```

On install Zed clones the grammar repo named in `extension.toml` into
`grammars/cppx/` and compiles `src/parser.c` to a WASM parser. The
`grammar-source/` directory is the canonical copy in this repo; `install.sh`
mirrors it into the sidecar and clears Zed's clone target so the next
install starts from a clean slate.

## Grammar notes

CPPX is C++ with JSX-like tags inside, which makes `<` ambiguous (template
parameter vs. JSX tag). The grammar resolves this structurally where it can
and with targeted lexical carve-outs where it can't:

- `Ident<...>` (no space) — always consumed as cpp_text (template).
- `template <...>` — explicit keyword carve-out.
- `<<`, `< 5`, etc. — `<` followed by non-tag-start stays in cpp_text.
- Otherwise, `<Ident` starts a JSX tag, which only commits if the surrounding
  structure can parse as `open + children + close` or `self-closing`.

See `grammar.js` for the full rules.

## Relationship to the TextMate package

The sibling `tools/editor/cppx/` directory ships a TextMate grammar that
covers VS Code and JetBrains. Zed doesn't load TextMate grammars, so this
directory is the parallel tree-sitter implementation.
