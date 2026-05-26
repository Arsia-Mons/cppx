# UI source architecture audit

This audit compares the current `src/` tree to the active UI architecture docs:

- `docs/client-ui-focus-navigation-architecture.md`
- `docs/ui-focus-interaction-plan.md`
- `docs/ui-focus-stress-test.md`
- `docs/engine-ui-boundary-plan.md`

It is a source/readiness audit, not a replacement architecture proposal.

## Implemented source layers

- `src/ui/focus/` owns generic focus scopes, focusable registration, modal
  trapping, focus source tracking, previous-frame layout harvest, spatial
  navigation, initial focus, disabled-control skipping, overflow diagnostics,
  pointer press/release confirmation, and bounded frame-local storage.
- `src/ui/primitives/` owns generic focusable primitives: `Focusable`,
  `Button`, `Toggle`, and `Selectable`, with visual state derived from focus
  and caller-owned control state.
- `src/client/ui/` owns retained `UiScreen` entries, `ScreenStack`,
  visible-screen ordering, overlay frames, `ScreenNavigator`, `ClientUi`, and a
  bounded post-layout write queue for stack and game writes.
- `src/game/ui/` owns the `GameUiPipeline` frame boundary around React, Clay,
  `ClientUi`, focus/layout dispatch, rendering, and post-render write draining.
- `src/platform/` owns SDL/key/gamepad/pointer adaptation plus the local control
  mailbox used by the CLI and E2E tests.
- `src/shooter/` is the pressure-test example with HUD, pause/options, loadout
  tabs, weapon/gear grid, details, equipment slots, compare toggle, modal
  confirmation, disabled item behavior, and hooks returning reads/functions.

## Removed demo shortcuts

- The old direct `App(&input)` root is no longer compiled into `hello`.
- Demo-only `Counter`, `Image`, theme provider, input provider, `InputState`,
  and demo app screen sources have been removed from the active runtime.
- The primary runtime path is now:

```text
SDL/platform input
  -> UiInputFrame
  -> GameUiPipeline
  -> ClientUi
  -> ScreenStack / UiScreen
  -> screen component roots
  -> hooks and primitives
  -> Clay layout/render
  -> post-layout/post-render write drain
```

## Current proof surface

- Unit tests cover React hook/provider/runtime behavior, focus navigation,
  primitive interaction, retained screen stack ownership, visible overlay
  ordering, and game/UI pipeline write ordering.
- Shooter UI tests cover pause/options stack flow, loadout modal focus
  restoration, loadout tab/equipment targets, deferred shooter writes, and real
  shooter buy/equip state transitions.
- CLI tests launch the runtime, drive keyboard/gamepad/pointer input through the
  platform adapter, inspect screens/focus/game state/focusable rectangles,
  target-click focusables, capture screenshots, capture frame sequences, and
  validate wrong-state/error reporting.
- The repo-local UI implementation skill under `.codex/skills/` is part of the
  working contract for future implementation and review passes.

## Remaining review focus

Before claiming the full plan complete, reviewers should inspect:

- that ordinary screen components continue to use hooks for game/UI reads and
  write functions instead of prop-drilled state bundles;
- that `ui/` stays shooter-agnostic and `platform/` stays free of shooter
  mutation shortcuts;
- that new visual flows are proven by CLI captures and opened screenshots, not
  only state assertions;
- that the repo-local UI skill remains current with any new source or reviewer
  findings.
