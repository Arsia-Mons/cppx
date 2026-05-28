---
name: cppx-authoring
description: Author idiomatic .cppx/.hx retained UI code in this C++20 SDL repo. Use when editing or reviewing .cppx, .hx, src/ui/components, retained UiElement component code, cppx transpiler fixtures, or JSX-like children composition.
---

# CPPX Authoring

## First Read

Before structural edits, inspect the current local files:

- `src/ui/CLAUDE.md` for generic UI boundaries.
- `src/react.h` for the hook/runtime contract.
- `src/ui/runtime/element.h` for `UiElement`, `UiChild`, `UiChildren`, and descriptor helpers.
- `tools/cppx_transpile.py` only when syntax or lowering behavior is unclear.

`architecture.md` may be absent in this checkout. If it is absent, do not invent broader boundaries from memory; use the files above plus nearby code.

## Authored Surface

Treat `.cppx` and `.hx` as the source files. Do not edit generated files under `cmake-build-*` or `generated/cppx`.

Use JSX-like authored components in `.cppx`:

```cppx
return <detail.Host kind={::ui::HostKind::Button}>
  {props.children}
  {props.children.count > 0 ? nullptr : props.label}
</detail.Host>
```

Avoid generated-code-shaped source in `.cppx`:

```cpp
::ui::component("Text", TextProps{.value = label}, Text)
::ui::host(::ui::HostKind::Box, {...})
```

Allowed exceptions:

- `components.h` descriptor factories.
- `.expected.*` transpiler fixtures.
- Ordinary non-`.cppx` C++ code that intentionally builds descriptors.

## Children

Prefer visible child composition at the JSX callsite.

- Use `{props.children}` for child slots.
- Use raw text children or string expressions for labels: `Resume`, `{props.label}`.
- Use `nullptr` for an absent string child. In this repo, `UiChild(const char*)` turns non-null strings into text nodes and null into an empty child.
- Do not hide simple child lists behind helpers like `checkbox_children(props)`.
- Keep helpers for data, styles, or callback assembly, not for disguising the JSX tree.

Good:

```cppx
return <detail.Host kind={::ui::HostKind::Checkbox}>
  <Box key="mark" style={checkbox_mark_style(props.checked)} />
  {props.label}
</detail.Host>
```

Acceptable fallback pattern when preserving a legacy `label` prop:

```cppx
return <detail.Host kind={::ui::HostKind::Button}>
  {props.children}
  {props.children.count > 0 ? nullptr : props.label}
</detail.Host>
```

Avoid:

```cppx
return <detail.Host kind={::ui::HostKind::Checkbox}>
  {checkbox_children(props)}
</detail.Host>
```

## Components

Generic reusable UI lives under `src/ui/components` and must remain game-agnostic.

- One component per `.hx`/`.cppx` pair.
- Use generated-header-safe includes in `.hx`, such as `#include "ui/components/common.h"`.
- Include component headers directly in `.cppx` when composing components, for example `#include "box.h"` before `<Box ... />`.
- Use `<detail.Host>` inside generic components when emitting a host node.
- Use component tags like `<Box>` and `<Text>` for component children rather than manually calling descriptor factories.

## Transpiler Limits

The current transpiler is intentionally small and line-oriented.

- JSX tags are handled as standalone tag lines or within an active JSX child list.
- JSX inside arbitrary C++ expressions, such as a ternary branch, is not supported.
- If that limitation forces generated-code-shaped authored source, prefer changing the authored structure or adding runtime/transpiler support instead of leaking `::ui::component(...)` into `.cppx`.

## Validation

After editing `.cppx` or `.hx`:

```sh
python3 tools/cppx_format.py --in-place <files>
python3 tools/cppx_transpile.py <file.cppx>
./build.sh --tests
git diff --check
```

For CMake wiring, add new `.cppx` and `.hx` inputs to the relevant `cppx_transpile(...)` call, then consume the generated outputs through the existing generated source variables.
