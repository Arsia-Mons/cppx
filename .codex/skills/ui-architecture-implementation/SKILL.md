---
name: ui-architecture-implementation
description: Implement, extend, review, or correct this repo's React-style Clay UI architecture from the game loop through GameUiPipeline, ClientUi, ScreenStack, UiScreen, screen component roots, hooks/providers, focus runtime, and primitives. Use when working in /Users/hv/repos/sdl3-clay on src/ui/focus, src/ui/primitives, src/client/ui, src/game/ui, src/shooter, docs/ui-src-architecture-implementation-plan.md, or any task that risks drifting into MVC, broad state bundles, route-table renderers, prop drilling, manual focus graphs, or demo-only UI shortcuts.
---

# UI Architecture Implementation

## Start Here

Use this skill to implement the active UI architecture in source. The goal is a
React-style UI stack on Clay, not MVC, not route tables, not view models, and not
a demo component tree with nicer names.

Read these files before changing architecture:

- `docs/ui-src-architecture-implementation-plan.md`
- `docs/client-ui-focus-navigation-architecture.md`
- `docs/ui-focus-interaction-plan.md`
- `docs/ui-focus-stress-test.md`
- `docs/engine-ui-boundary-plan.md`
- `AGENTS.md`

Inspect current source before relying on memory. The source is the authority for
what has already landed.

## Canonical Stack

Keep this ownership chain intact:

```text
Game tick
  polls platform input, ticks gameplay, draws world, invokes GameUiPipeline

GameUiPipeline
  adapts platform/game state into UiInputFrame and presentation providers

ClientUi
  owns focus runtime, input routing, retained ScreenStack, write queues, drain point

ScreenStack / UiScreen
  owns retained screen lifetimes and visible ordering

{Name}ScreenView
  component root that uses hooks and local hook state

hooks/providers
  return values and functions at the point components need them

ui/primitives
  Focusable, Button, Toggle, Selectable, tile/stepper controls

ui/focus
  focus scopes, registration, previous-frame geometry navigation

Clay
  layout and render commands only
```

Dependency direction goes downward. Generic `ui/` must not know shooter rules.
Shooter code must not own focus, primitive state, or screen-stack mechanics.

## Patterns To Preserve

- Use hooks as the UI boundary. Hooks normally return values and functions.
- Keep local screen/dialog scratch in hook state inside `{Name}ScreenView`.
- Expose shared reads through named hooks, not broad world-shaped context.
- Request stack/game writes through hook-returned functions and drain after Clay
  declaration.
- `GameUiPipeline` owns React/Clay frame lifecycle around `ClientUi`: begin,
  declare, end layout, dispatch focus/input, render Clay commands, then drain
  queued UI writes.
- Key retained-screen providers and screen component roots by `UiScreen` entry
  id so hook state survives rerenders and resets on unmount/new entries.
- Route `use_screen_navigator().pop_current()` by retained screen entry id,
  not by whichever screen happens to be top when writes drain.
- Let screen authors declare components. Do not make them author sibling
  navigation edges.
- Derive directional navigation from harvested Clay rectangles from the previous
  completed layout frame.
- Use explicit nav rules only for rare boundaries: stop, wrap, or explicit
  target.
- Keep callbacks frame-local. Persist ids, layout, and state, not stale function
  objects from older frames.
- Capture confirm targets before current-layout fallback repair. If a focused
  control becomes disabled or disappears during the current declaration, do not
  retarget the same-frame confirm press to the newly selected fallback element.
- Suppress duplicate pointer/key confirms only after one path actually
  dispatched. A pointer release on the already-focused target must still confirm
  when no keyboard/gamepad confirm edge fired.
- In platform-to-`UiInputFrame` adapters, use current key/button state for
  `*_down` fields and edge events for `*_pressed`/`*_released`. A held pointer
  alone must not overwrite the source for a same-frame keyboard/gamepad
  navigation edge.
- When clearing bounded frame-local registration arrays, reset the entries, not
  only the count. Callback objects must release captures at the end of the UI
  frame.
- Keep bounded runtime storage honest. Overflow is a diagnostic and dropped
  registration/write, not hidden allocation in the middle of a UI frame.

## Implementation Workflow

1. Re-read the active plan and the relevant architecture doc section.
2. Inspect current `src/`, `tests/`, and existing uncommitted changes.
3. Implement one coherent slice.
4. Add focused tests that prove the slice's real invariant.
5. Run local verification.
6. Commit the slice with a message that names the doc requirement and checks.
7. Run a read-only review pass and fix every high-confidence finding.
8. Update this skill when the slice exposes a new rule, trap, or correction.

Do not use `docs/ui-src-architecture-implementation-plan.md` as a progress
tracker. Edit the plan only when the contract itself needs correction.

## Source-Specific Guidance

- `src/react.*` is the foundation. Extend the existing component, keyed identity,
  hooks, providers, effects, refs, and cleanup runtime.
- `src/main.cpp` is platform scaffolding until the pipeline replaces direct
  `App(&input)` rendering.
- Demo providers/components are examples, not architecture. Do not let
  `Counter`, `Image`, or demo `InputState` define the final UI boundary.
- `src/ui/focus/` stays generic: stable focus scopes, focusable registration,
  layout harvest, spatial navigation, modal trapping, and source tracking.
- `src/ui/primitives/` stays generic: controls derive visual state from focus
  plus caller-owned control state.
- `src/client/ui/` owns retained screens, `ClientUi`, screen navigation hooks,
  visible ordering, and post-layout write draining.
- `src/game/ui/` adapts platform/game state into `UiInputFrame`, presentation
  hooks/providers, and write application.
- `src/shooter/` proves the architecture through HUD, pause/options, loadout,
  modals, disabled items, tab/grid reflow, and real write requests.

## Failure Modes To Correct

- Calling anything a view model or introducing screen-wide state bundles.
- Passing a broad state object through component trees instead of using hooks.
- Replacing retained `UiScreen` objects with enums, route tables, or switch
  renderers.
- Putting navigation graphs in screen code for ordinary lists/grids.
- Reading raw shooter/world state directly from ordinary UI components.
- Mutating game, stack, or shared state during Clay declaration.
- Storing old-frame callbacks in retained focus records.
- Letting a modal scope move parent focus while the modal is active.
- Treating passing render output as proof when focus/input/write ordering is
  unverified.

## Verification

Use focused tests as slices land, plus the plan gates before completion claims:

```sh
ctest --test-dir build --output-on-failure
git diff --check
rg -n "ScreenRoute|GridStrategy|use_local_state|OptionsState|view model|MVC" docs src tests
```

Expected matches in active docs may be guardrail text. Do not treat guardrail
mentions as source drift, but investigate any source or example that trains the
wrong architecture.

Before claiming a slice is done, inspect this skill and update it if source,
docs, tests, or reviewer findings changed the operating rules.
