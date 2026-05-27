# Retained UI Migration Plan

This plan implements `retained-ui-migration-prompt.md` without redefining the
goal around the current Clay implementation. It is a living checklist for the
branch and PR.

## Current Findings

- `architecture.md` was missing in the current worktree even though the root
  contract says it is canonical. This slice restores it with the retained-mode
  target described explicitly.
- `src/react.{h,cpp}` is still used by Clay-backed screens, but hook fiber
  identity now has an app-owned 64-bit path. It already proves useful
  semantics: stable hook storage, keyed siblings, providers/context, effects,
  refs, callbacks, text storage, and unmount cleanup.
- `ClientUi`, `UiPipeline`, and `ScreenStack` already own the correct
  client-shell concerns: retained screens, retained frame sequencing, focus
  runtime lifetime, and deferred mutation draining. `ClientUi` owns retained
  tree/focus/draw runtime state for upcoming screen ports, and `UiPipeline`
  updates those outputs before renderer handoff.
- `src/ui/focus` and `src/ui/primitives` still use Clay IDs and Clay layout
  queries. Those modules will need retained node IDs and retained layout boxes
  before Clay can be removed.
- `renderer/` still emits legacy screens from `Clay_RenderCommandArray`, but
  the retained runtime now also has an app-owned draw command boundary consumed
  by the SDL retained renderer.
- `tools/ui_cli.py` drives the app through the control mailbox. That path must
  remain deterministic through the migration. Retained focusables now appear in
  the same inspect/state `focusables` array used by CLI pointer targeting.

## Flexbox Evaluation

The prompt requires a maintained C/C++-friendly flexbox option before
hand-rolling layout.

Selected path: Yoga.

- Yoga's official repository describes it as an embeddable flexbox layout
  engine and states that its main implementation targets C++20 with CMake build
  logic: https://github.com/facebook/yoga
- Taffy supports flexbox and grid and is strong for custom renderers, but its
  official docs describe it as Rust/WASM-based. That makes it a less direct fit
  for this C++20 repo unless a Rust bridge becomes acceptable:
  https://taffylayout.com/
- Stretch is Rust-first, with bindings for several platforms. It is a weaker
  fit than Yoga for this C++20/CMake codebase:
  https://github.com/vislyhq/stretch

Implementation rule: keep Yoga behind `FlexLayoutAdapter`. Do not build app
code against Yoga directly.

Current status: Yoga is wired behind `make_yoga_flex_layout_adapter()`, with
tests for column gap/grow, row percent/grow, padding, and measured nodes.

## Transpiler Design

Authored extensions:

- `.hx` for headers that may contain component declarations and JSX-like
  fragments.
- `.cppx` for component implementations.

Generated extensions:

- `.h` and `.cpp` in a deterministic generated directory under the CMake build
  tree.

Tool placement:

- `tools/cppx_transpile.py`.
- `cmake/cppx_transpile.cmake` for generated build-tree outputs.
- Golden tests in `tests/` that feed `.cppx`/`.hx` fixtures and compare exact
  generated output and diagnostics.

Grammar principles:

- JSX tags lower to normal C++ component calls.
- `{...}` escapes stay C++ expressions.
- `key=...` lowers to retained keyed child identity.
- Children lower to slot callbacks; they are invoked synchronously and never
  stored past the component call.
- Diagnostics include source filename and line mapping.

Current status: the transpiler handles JSX statement lines, compound component
names (`Panel.Header` -> `Panel::Header`), string/expression/bool props, text
children, expression children, `#line` source mappings, deterministic output,
and mismatch/unclosed-tag diagnostics.

## Migration Slices

1. Retained runtime skeleton: done.
   - `src/ui/retained/UiTree` with stable IDs, keyed children, style storage,
     layout storage, unmount cleanup, and frame reconciliation.
   - `FlexLayoutAdapter` boundary.
   - focused runtime tests.

2. Yoga integration: foundation done.
   - CMake `FetchContent` or vendored dependency decision.
   - adapter maps retained style to Yoga nodes.
   - tests for row/column, fixed, percent, grow, gap, padding, and measured
     nodes.
   - remaining before primitive port: broader alignment coverage as needed by
     the first retained primitives.

3. Transpiler: foundation done.
   - lexer/parser/generator.
   - golden fixtures.
   - CMake generated-source integration.
   - remaining before screen ports: expand the grammar only where real retained
     primitives need it, and start authoring migrated UI files as `.cppx` /
     `.hx`.

4. Retained hook runtime:
   - replace Clay-derived component IDs with retained parent/key identity:
     foundation done through `ReactFiberId`, `react_init_runtime()`, and
     `REACT_RETAINED_COMPONENT_*`.
   - preserve hooks, providers, effects, refs, callbacks, and unmount cleanup.
   - bind retained component entry to authored `.cppx` output and `UiTree` node
     creation: done through `src/ui/retained/components.*` and
     `retained_cppx_component_tests`.
   - remaining before primitive ports: expand the generic component surface only
     where real primitives need it.

5. Primitive port:
   - text, panel, button, toggle, and selectable retained nodes: foundation
     done through copied `UiTree` metadata, explicit retained primitive props,
     semantic control roles, labels, and interaction state.
   - retained button confirm callbacks: foundation done through copied node
     callbacks and `ClientUi` dispatch from retained confirmed node IDs.
   - remaining: focusable containers, scroll containers, visual draw styles,
     toggle/selectable callbacks, and broader event wiring.

6. Event and focus port:
   - hit testing from retained layout boxes: foundation done through
     `src/ui/retained/focus.*`.
   - focus scopes and navigation over retained node IDs: foundation done for
     root/modal retained scopes, spatial navigation, disabled controls, pointer
     confirm, and keyboard/gamepad/mouse source tracking.
   - button confirm dispatch now preserves deferred mutation ordering at the
     same frame boundary.

7. Screen ports:
   - shell-owned retained runtime state in `ClientUi`: foundation done.
   - pipeline-owned retained frame lifecycle: foundation done; screens can now
     emit retained nodes through `ScreenStack`, and the runtime computes layout,
     focus, and draw commands before render callbacks.
   - control-mailbox retained focusable readback: foundation done.
   - main menu.
   - options.
   - pause.
   - in-game HUD.
   - loadout and dialogs.

8. Renderer replacement:
   - retained draw command list: foundation done through
     `src/ui/retained/draw_list.*`, which emits rect/text commands from retained
     primitive metadata and computed layout boxes.
   - SDL renderer consumes retained commands: foundation done through
     `renderer/sdl_retained_renderer.*`, wired into the app frame after the
     legacy Clay pass.
   - font measurement moved behind retained layout text measurement.

9. Clay removal:
   - remove `third_party/clay*`.
   - remove `renderer/sdl_clay_renderer.*`.
   - delete Clay-specific tests or migrate assertions to retained snapshots.
   - add grep/guard tests for Clay re-entry.

## Verification Gates

Per meaningful slice:

- run the narrowest relevant target first;
- run `./build.sh --tests` before major handoffs;
- keep CLI smoke tests green once screen rendering is touched;
- capture BMP/screenshot evidence for visual screen ports;
- commit and push focused changes;
- keep the draft PR body current.

## Risks

- Hook identity currently depends on Clay hashes. This must move before screens
  can be Clay-free.
- Focus currently queries Clay element bounds. Event/focus migration must land
  after retained layout boxes are authoritative.
- The transpiler can easily become a separate language. Keep it as syntax sugar
  over ordinary C++ component calls.
- Partial Clay removal is dangerous if it leaves compatibility facades that
  still encode Clay semantics. Guard tests are required at the end.
