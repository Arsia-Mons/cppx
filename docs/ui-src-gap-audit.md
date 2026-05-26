# UI source gap audit

This audit compares the current `src/` tree to the active UI architecture docs:

- `docs/client-ui-focus-navigation-architecture.md`
- `docs/ui-focus-interaction-plan.md`
- `docs/ui-focus-stress-test.md`
- `docs/engine-ui-boundary-plan.md`

It is a working implementation audit. It does not reduce the target architecture.

## Missing source layers

- `src/ui/focus/`: no focus scopes, focusable registration, layout harvest,
  spatial resolver, focus source tracking, modal trapping, initial focus, or
  bounded diagnostics exist in source.
- `src/ui/primitives/`: no `Focusable`, `Button`, `Toggle`, `Selectable`,
  richer tile primitive, control-state layering, pointer press/release handling,
  or primitive tests exist.
- `src/client/ui/`: no retained `UiScreen`, `ScreenStack`, visible-screen
  ordering, `ScreenNavigator` hook, `ClientUi`, UI write queue, or post-layout
  drain point exists.
- `src/game/ui/`: no `UiInputFrame`, `GameUiPipeline`, platform-to-UI input
  adaptation, presentation providers, or game/UI write adapter exists.
- `src/shooter/`: no shooter state, presentation builders, shooter hooks, HUD,
  pause/options, loadout/buy flow, modal dialogs, or end-to-end stress example
  exists.

## Current demo shortcuts that conflict with the target

- `src/main.cpp` owns the whole UI frame and directly calls `App(&input)`.
  The documented path requires platform input to flow through a game/UI pipeline
  into `ClientUi`, with writes drained after Clay declaration.
- `InputState` is a demo key-edge struct for counter/theme/image behavior. It
  does not represent normalized navigation, confirm/cancel, pointer, or source
  data for a focus runtime.
- `App` is the root UI and owns demo-local composition directly. There is no
  retained screen stack or visible-screen ordering.
- `Counter` mutates hook state while reading demo input directly from
  `InputContext`. The final control path needs focusable primitives and
  hook-returned functions that request writes through the UI boundary.
- `Image` reaches SDL renderer globals through `app_state.h`. That is acceptable
  as a temporary sample resource path, but it is not the documented client UI
  boundary and should not shape the shooter example.
- Theme and input providers are useful provider examples, but they are demo
  providers. The final stack needs named presentation and write hooks for the
  client and shooter domains.

## Runtime pieces to keep as foundations

- `src/react.h` and `src/react.cpp` already provide component identity, keyed
  identity, hook storage, providers, effects, refs, frame boundaries, hook drift
  diagnostics, unmount cleanup, and shutdown cleanup.
- `REACT_COMPONENT_BEGIN`, `REACT_COMPONENT_BEGIN_KEY`, and provider macros are
  the existing C++ component boundary. New UI layers should compose through
  them instead of creating a second component model.
- `use_state_int`, `use_ref`, `use_effect`, and `use_context` are the current
  hook primitives. Local UI state for screens and dialogs should stay on this
  path.
- `tests/react_runtime_tests.cpp` is the existing regression harness for hook
  identity, providers, lifecycle, and cleanup. New architecture tests should
  extend the test suite rather than bypass it.
- The existing SDL/Clay initialization in `src/main.cpp` is useful platform
  scaffolding, but the primary app call must move behind the documented
  pipeline and `ClientUi` ownership.

## First vertical slice

The first source slice should implement the generic focus runtime under
`src/ui/focus/` with tests that do not depend on the shooter example:

1. Bounded focus runtime storage and explicit overflow diagnostics.
2. Focus scopes with stable ids, modal scope selection, initial focus requests,
   and parent focus preservation.
3. Focusable registration during declaration with current-frame callbacks.
4. Layout harvest from `Clay_GetElementData()` after `Clay_EndLayout()`.
5. Frame-N directional navigation using frame-N-1 rectangles.
6. Disabled-control skipping for navigation and confirm.
7. Stable spatial resolver tie-breaks and local boundary rules.

That slice proves the most architecture-critical invariant: screen code can
declare focusable UI in Clay order while navigation is derived from harvested
layout rectangles, not screen-authored neighbor tables.
