#!/usr/bin/env bash
# Prepare the local sidecar git repo that Zed's dev extension installer
# clones the cppx grammar from.
#
# Zed's extension manifest requires `[grammars.X]` to point at a git
# repository (no `path` option). We don't want to publish the grammar to
# a remote, and we don't want a nested `.git` inside this repo (it would
# turn `grammars/cppx/` into a submodule pointer in the outer repo).
#
# Instead this script keeps the grammar source tracked normally in the
# outer repo and mirrors it into a sibling git repo at
# `${SIDECAR_DIR}` whose only purpose is to give Zed a `file://` URL
# to clone from. The current commit SHA is written back into
# `extension.toml`.
#
# Run this whenever you edit `grammars/cppx/grammar.js` and want Zed to
# pick up the change. After running, install or reinstall the dev
# extension in Zed via `zed: install dev extension`, pointing at this
# directory.

set -euo pipefail

EXT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
GRAMMAR_SRC="${EXT_DIR}/grammar-source"
SIDECAR_DIR="${CPPX_SIDECAR_DIR:-${HOME}/.cache/zed-cppx-grammar}"
EXT_TOML="${EXT_DIR}/extension.toml"
ZED_CLONE_DIR="${EXT_DIR}/grammars/cppx"

if [[ ! -f "${GRAMMAR_SRC}/grammar.js" ]]; then
  echo "error: ${GRAMMAR_SRC}/grammar.js not found" >&2
  exit 1
fi

# Zed clones the grammar into <extension>/grammars/<name>/ at install time
# and refuses if that directory exists with non-matching contents. Clear it
# whenever we sync so the next install starts from a known-empty target.
if [[ -e "${ZED_CLONE_DIR}" ]]; then
  rm -rf "${ZED_CLONE_DIR}"
fi

mkdir -p "${SIDECAR_DIR}"

# Mirror grammar source into the sidecar, dropping anything that isn't part
# of the tracked source (e.g. build artifacts) so the sidecar stays minimal.
rsync -a --delete \
  --exclude='.git/' \
  --exclude='node_modules/' \
  --exclude='build/' \
  --exclude='prebuilds/' \
  --exclude='*.wasm' \
  "${GRAMMAR_SRC}/" "${SIDECAR_DIR}/"

cd "${SIDECAR_DIR}"

if [[ ! -d .git ]]; then
  git init -q
  git config user.email "$(git -C "${EXT_DIR}" config user.email 2>/dev/null || echo zed-cppx@local)"
  git config user.name "$(git -C "${EXT_DIR}" config user.name 2>/dev/null || echo zed-cppx)"
fi

git add -A
if git diff --cached --quiet; then
  COMMIT_SHA="$(git rev-parse HEAD)"
  echo "sidecar already up to date at ${COMMIT_SHA}"
else
  git commit -q -m "Sync from $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  COMMIT_SHA="$(git rev-parse HEAD)"
  echo "synced grammar -> ${SIDECAR_DIR} @ ${COMMIT_SHA}"
fi

# Rewrite the [grammars.cppx] block in extension.toml. We do it with awk
# rather than a TOML library so this script has zero dependencies beyond
# git/rsync/awk.
NEW_REPO="file://${SIDECAR_DIR}"
awk -v repo="${NEW_REPO}" -v rev="${COMMIT_SHA}" '
  BEGIN { in_section = 0 }
  /^\[grammars\.cppx\]/ { in_section = 1; print; next }
  /^\[/ && in_section { in_section = 0 }
  in_section && /^repository[[:space:]]*=/ { printf "repository = \"%s\"\n", repo; next }
  in_section && /^rev[[:space:]]*=/        { printf "rev = \"%s\"\n", rev; next }
  { print }
' "${EXT_TOML}" > "${EXT_TOML}.tmp"
mv "${EXT_TOML}.tmp" "${EXT_TOML}"

echo "updated ${EXT_TOML}:"
grep -A2 '^\[grammars\.cppx\]' "${EXT_TOML}"
echo
echo "next: in Zed, run 'zed: install dev extension' and pick ${EXT_DIR}"
