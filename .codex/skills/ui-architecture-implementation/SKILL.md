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

platform/control harness
  launches or attaches to the runtime, drives the same input adapter, captures rendered frames

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
- Shooter state may be held by the game/sample owner and exposed to screens via
  a provider, but components should consume narrow hooks returning values and
  functions instead of receiving the whole game object as props.
- `GameUiPipeline` owns React/Clay frame lifecycle around `ClientUi`: begin,
  declare, end layout, dispatch focus/input, render Clay commands, then drain
  queued UI writes.
- CLI/control-harness input must enter through the same platform-to-`UiInputFrame`
  adapter as normal runtime input. Screenshots and frame captures must come from
  the actual rendered frame, not a synthetic component snapshot.
- Gamepad-style harness input should set `UiFocusSource::Gamepad` through the
  platform input adapter so focus visibility/source behavior is tested
  separately from keyboard shortcuts.
- CLI/control-harness inspection can expose read-only runtime and game debug
  state, but keep the platform mailbox generic. App/game code should register a
  narrow read-only provider instead of letting `platform/` learn shooter rules or
  mutate game/UI state behind the pipeline.
- Targeted CLI pointer commands should resolve against inspectable focusable
  rectangles from the previous rendered frame. The CLI may translate a target
  name/id to coordinates, but it must still send normal pointer input through
  the platform adapter instead of mutating focus or invoking UI callbacks.
- Visual E2E proof must include opening/inspecting representative captured
  frames. Passing component layout numbers, state assertions, or non-empty
  image files are not enough when the final rendered result looks wrong.
- Key retained-screen providers and screen component roots by `UiScreen` entry
  id so hook state survives rerenders and resets on unmount/new entries.
- Route `use_screen_navigator().pop_current()` by retained screen entry id,
  not by whichever screen happens to be top when writes drain.
- Route `use_screen_navigator().push(...)` through the same bounded
  post-layout write queue. Queued screen objects are owned by the queue until
  drain and must be cleared, not orphaned, if the queue is reset.
- Render retained overlay screens through `ClientUi`-owned Clay floating frames
  attached to the root. Emitting visible screens as ordinary siblings can make
  layout/state tests pass while overlays render beside, rather than over, the
  gameplay screen.
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
- When a conditional dialog should start from its default control each time it
  opens, key the dialog component and focus scope by a local open-instance
  serial. Reusing the same modal scope id can intentionally preserve focus, but
  it can also reopen on the old Cancel/destructive target after the dialog was
  hidden.
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
- When a Clay config field has a narrow type, compute dynamic values into a
  typed local such as `uint16_t border_width` before using Clay macros inside
  designated initializers.

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
- `src/platform/` owns platform input adaptation and local control harness
  plumbing. It may inspect runtime state for tests, but must not mutate focus,
  screen, or shooter state behind the pipeline.
- `src/shooter/` proves the architecture through HUD, pause/options, loadout,
  modals, disabled items, tab/grid reflow, and real write requests.

## Failure Modes To Correct

- Calling anything a view model or introducing screen-wide state bundles.
- Passing a broad state object through component trees instead of using hooks.
- Replacing retained `UiScreen` objects with enums, route tables, or switch
  renderers.
- Rendering overlay screens as normal Clay siblings instead of root-attached
  floating frames owned by `ClientUi`.
- Putting navigation graphs in screen code for ordinary lists/grids.
- Reading raw shooter/world state directly from ordinary UI components.
- Mutating game, stack, or shared state during Clay declaration.
- Storing old-frame callbacks in retained focus records.
- Letting a modal scope move parent focus while the modal is active.
- Reusing a conditional modal focus scope id when the UX requires fresh initial
  focus on every open.
- Treating passing render output as proof when focus/input/write ordering is
  unverified, or treating a captured screenshot as proof without inspecting the
  rendered pixels for unexpected layout, clipping, focus, or artifact issues.
- Building CLI tests that bypass SDL/platform input adaptation or capture
  non-rendered component snapshots.

## Verification

Use focused tests as slices land, plus the plan gates before completion claims:

```sh
ctest --test-dir build --output-on-failure
git diff --check
rg -n "ScreenRoute|GridStrategy|use_local_state|OptionsState|view model|MVC" docs src tests
```

For any slice that produces screenshots or frame captures, open representative
captures and inspect the rendered result before claiming visual correctness.
The harness can prove that the file was produced; it cannot by itself prove that
the UI looks correct.

Expected matches in active docs may be guardrail text. Do not treat guardrail
mentions as source drift, but investigate any source or example that trains the
wrong architecture.

Before claiming a slice is done, inspect this skill and update it if source,
docs, tests, or reviewer findings changed the operating rules.
