# Retained UI Migration Plan

This plan implements `retained-ui-migration-prompt.md` without redefining the
goal around the current Clay implementation. It is a living checklist for the
branch and PR.

## Current Findings

- `architecture.md` was missing in the current worktree even though the root
  contract says it is canonical. This slice restores it with the retained-mode
  target described explicitly.
- `src/react.{h,cpp}` is still Clay-backed. It already proves useful semantics:
  stable hook storage, keyed siblings, providers/context, effects, refs,
  callbacks, text storage, and unmount cleanup.
- `ClientUi` and `ScreenStack` already own the correct client-shell concerns:
  retained screens, focus runtime lifetime, per-frame sequencing, and deferred
  mutation draining.
- `src/ui/focus` and `src/ui/primitives` still use Clay IDs and Clay layout
  queries. Those modules will need retained node IDs and retained layout boxes
  before Clay can be removed.
- `renderer/` still emits from `Clay_RenderCommandArray`. The retained runtime
  needs an app-owned draw command boundary before the renderer can stop
  depending on Clay.
- `tools/ui_cli.py` drives the app through the control mailbox. That path must
  remain deterministic through the migration.

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
tests for column gap/grow and row percent/grow behavior.

## Transpiler Design

Authored extensions:

- `.hx` for headers that may contain component declarations and JSX-like
  fragments.
- `.cppx` for component implementations.

Generated extensions:

- `.h` and `.cpp` in a deterministic generated directory under the CMake build
  tree.

Tool placement:

- `tools/cppx_transpile.py` or a small C++ tool under `tools/`, selected after
  the grammar is pinned.
- Golden tests in `tests/` that feed `.cppx`/`.hx` fixtures and compare exact
  generated output.

Grammar principles:

- JSX tags lower to normal C++ component calls.
- `{...}` escapes stay C++ expressions.
- `key=...` lowers to retained keyed child identity.
- Children lower to slot callbacks; they are invoked synchronously and never
  stored past the component call.
- Diagnostics include source filename and line mapping.

## Migration Slices

1. Retained runtime skeleton: done.
   - `src/ui/retained/UiTree` with stable IDs, keyed children, style storage,
     layout storage, unmount cleanup, and frame reconciliation.
   - `FlexLayoutAdapter` boundary.
   - focused runtime tests.

2. Yoga integration: adapter foundation done; text measurement expansion remains.
   - CMake `FetchContent` or vendored dependency decision.
   - adapter maps retained style to Yoga nodes.
   - tests for row/column, fixed, percent, grow, gap, and padding.
   - remaining before primitive port: measured text nodes and broader alignment
     coverage.

3. Transpiler:
   - lexer/parser/generator.
   - golden fixtures.
   - CMake generated-source integration.

4. Retained hook runtime:
   - replace Clay-derived component IDs with retained parent/key identity.
   - preserve hooks, providers, effects, refs, callbacks, and unmount cleanup.

5. Primitive port:
   - text, button, toggle, selectable, focusable containers, panels, scroll
     containers, and common visual state.

6. Event and focus port:
   - hit testing from retained layout boxes.
   - focus scopes and navigation over retained node IDs.
   - deferred mutation ordering kept at the same frame boundary.

7. Screen ports:
   - main menu.
   - options.
   - pause.
   - in-game HUD.
   - loadout and dialogs.

8. Renderer replacement:
   - retained draw command list.
   - SDL renderer consumes retained commands.
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
