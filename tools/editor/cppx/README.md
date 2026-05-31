# CPPX Syntax

TextMate grammar package for `.cppx` and `.hx` authored UI files.

The grammar treats the files as C++ with JSX-like tags layered on top. It is
shaped as a VS Code language extension so editors that understand TextMate
bundles or VS Code extension folders can reuse the same package.

## VS Code

Package this directory as a VSIX from `tools/editor/cppx` and install the
resulting extension:

```sh
npx @vscode/vsce package
code --install-extension cppx-syntax-0.0.1.vsix
```

## JetBrains

Open Settings, go to Editor > TextMate Bundles, and add this directory.

## Terminal Diffs

The repository-level `.gitattributes` maps `.cppx` and `.hx` to Git's C++
diff driver. Syntax-aware diff viewers can additionally map these extensions
to C++ or this grammar depending on their extension support.
