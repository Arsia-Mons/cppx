# Retained UI Migration Plan

This plan implements `retained-ui-migration-prompt.md` without redefining the
goal around the previous immediate-mode implementation. It is a living
checklist for the branch and PR.

## Current Findings

- `architecture.md` was missing in the current worktree even though the root
  contract says it is canonical. This slice restores it with the retained-mode
  target described explicitly.
- `src/react.{h,cpp}` is backend-free. Hook fiber identity has an app-owned
  64-bit path and preserves stable hook storage, keyed siblings,
  providers/context, effects, refs, callbacks, text storage, and unmount
  cleanup.
- `ClientUi`, `UiPipeline`, and `ScreenStack` own the retained app path:
  retained screens, retained frame sequencing, focus runtime lifetime, retained
  draw outputs, and deferred mutation draining. `UiPipeline` updates retained
  layout/focus/draw state before renderer handoff.
- `src/ui/focus`, `src/ui/primitives`, and their tests have been removed after
  their retained replacements reached parity.
- `renderer/` now renders the app from retained draw commands through
  `SdlRetainedRenderer`.
- `tools/ui_cli.py` drives the app through the control mailbox. That path must
  remain deterministic through the migration. Retained focusables now appear in
  the same inspect/state `focusables` array used by CLI pointer targeting.
- `goal/real-retained-reconciliation.md` tightens the final authoring model:
  app/client components must return `UiElement` descriptions, the reconciler
  must own component invocation and hook entry/exit, and old immediate
  retained declarations must not remain the production authoring API.

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

Current status: the transpiler handles JSX statement and `return <...>` lines,
compound component names (`Panel.Header` -> `Panel::Header`),
string/expression/bool props, text children, expression children, camelCase to
snake_case prop names, `#line` source mappings, deterministic output, and
mismatch/unclosed-tag diagnostics. It now emits returned `UiElement`
construction over `UiElementFrame` and `children` props instead of nested
immediate component calls.

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
   - returned-element output done for generated fixtures.
   - remaining before screen ports: expand the grammar only where migrated
     app/client UI needs it, and start authoring migrated UI files as `.cppx` /
     `.hx`.

4. Retained hook runtime:
   - retained parent/key identity: done through `ReactFiberId`,
     `react_init_runtime()`, and component macros.
   - preserve hooks, providers, effects, refs, callbacks, and unmount cleanup.
   - bind retained component entry to authored `.cppx` output and `UiTree` node
     creation: done through `src/ui/retained/components.*` and
     `retained_cppx_component_tests`.
   - backend-free runtime cleanup: done.

5. Primitive port:
   - text, panel, button, toggle, and selectable retained nodes: foundation
     done through copied `UiTree` metadata, explicit retained primitive props,
     semantic control roles, labels, and interaction state.
   - visual metadata for panel backgrounds, borders, text color, and text size:
     foundation done.
   - retained button confirm callbacks: foundation done through copied node
     callbacks and `ClientUi` dispatch from retained confirmed node IDs.
   - retained selectable focus/confirm callbacks: foundation done, including
     custom retained selectable content for complex screen controls.
   - generic focusable containers and scroll containers: foundation done.
   - remaining: richer draw styles and broader event wiring.

6. Event and focus port:
   - hit testing from retained layout boxes: foundation done through
     `src/ui/retained/focus.*`.
   - focus scopes and navigation over retained node IDs: foundation done for
     root/modal retained scopes, spatial navigation, disabled controls, pointer
     confirm, and keyboard/gamepad/mouse source tracking.
   - button confirm dispatch now preserves deferred mutation ordering at the
     same frame boundary.
   - retained focus changes now dispatch copied focus callbacks at the client
     frame boundary and restore parent focus after modal retained scopes close.

7. Screen ports:
   - shell-owned retained runtime state in `ClientUi`: foundation done.
   - pipeline-owned retained frame lifecycle: foundation done; screens can now
     emit retained nodes through `ScreenStack`, and the runtime computes layout,
     focus, and draw commands before render callbacks.
   - control-mailbox retained focusable readback: foundation done.
   - main menu: ported to retained panels, text, buttons, focus, confirm
     callbacks, and CLI targeting.
   - options: ported to retained modal panel, text, toggles, back button,
     focus, confirm callbacks, and CLI targeting.
   - retained overlay top-screen gating: foundation done through
     `ScreenProvider`.
   - pause: ported to retained modal panel, text, buttons, focus, confirm
     callbacks, and CLI targeting.
   - in-game HUD/action buttons: ported to retained panels, text, buttons,
     focus, confirm callbacks, and top-screen gating.
   - loadout and confirm dialog: ported to retained panels, text, selectables,
     toggles, buttons, focus callbacks, confirm callbacks, indexed CLI
     targeting, and modal focus restoration.

8. Renderer replacement:
   - retained draw command list: foundation done through
     `src/ui/retained/draw_list.*`, which emits rect/text commands from retained
     primitive metadata and computed layout boxes.
   - SDL renderer consumes retained commands: foundation done through
     `renderer/sdl_retained_renderer.*`, wired as the app frame's only UI
     renderer.
   - app frame uses retained draw commands only.
   - retained text rendering now uses `FontRegistry` for SDL_ttf font/text
     engine ownership.

9. Runtime dependency removal:
   - removed obsolete vendored layout-runtime files.
   - removed obsolete renderer glue.
   - deleted `src/ui/focus`, `src/ui/primitives`, and their tests after
     retained assertions covered the behavior.
   - added `runtime_dependency_guard` to prevent obsolete UI runtime
     dependencies from re-entering source and CMake paths.

10. Returned-element reconciler:
   - foundation started through `src/ui/retained/element.*`.
   - `UiElement` now covers empty, fragment, host, component, and provider
     descriptions.
   - `UiElementFrame` owns bounded frame storage for element descriptors,
     child lists, copied component props, copied strings, and destructors.
   - `reconcile_retained_tree()` walks returned elements, invokes component
     render thunks, pushes provider context, owns hook fiber entry/exit, and
     commits `HostKind::Box` / `HostKind::Text` nodes into `UiTree`.
   - `retained_ui_reconciler_tests` verifies host commit metadata, component
     hook identity across frames, and provider context scoping.
   - retained control factory foundation started through
     `src/ui/retained/element_components.*`: button, toggle, selectable,
     focusable, and scroll-container components now return `UiElement`
     descriptors over `HostKind::Box` / `HostKind::Text` instead of mutating
     `UiTree` directly.
   - `retained_ui_element_components_tests` verifies element button metadata
     and callbacks, toggle change dispatch, and selectable owned children.
   - `.cppx` / `.hx` output now emits returned element construction with
     `children` props; `retained_cppx_component_tests` compiles that output
     against `BoxElement`, `TextElement`, `ButtonElement`, and a stateful
     component descriptor.
   - `UiScreen` now has an optional returned-element entrypoint, and `ClientUi`
     can commit returned screen roots inside the existing screen/provider frame
     boundary.
   - main menu, options, pause, in-game HUD/action UI, loadout, and loadout
     confirm dialog are migrated to returned `UiElement` roots/components
     through `BoxElement`, `TextElement`, `ButtonElement`, `ToggleElement`,
     `SelectableElement`, and frame-owned provider values.
   - remaining: delete or make private the old immediate authoring API and add
     guards that keep it out of app/client code.

## Verification Gates

Per meaningful slice:

- run the narrowest relevant target first;
- run `./build.sh --tests` before major handoffs;
- keep CLI smoke tests green once screen rendering is touched;
- capture BMP/screenshot evidence for visual screen ports;
- commit and push focused changes;
- keep the draft PR body current.

## Risks

- The transpiler can easily become a separate language. Keep it as syntax sugar
  over ordinary C++ component calls.
- Runtime dependency drift is now guarded by `runtime_dependency_guard`; update
  that test if new source roots are added to the UI runtime path.
