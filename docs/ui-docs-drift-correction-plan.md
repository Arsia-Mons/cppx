# UI docs drift correction execution record

## Scope

The active architecture docs are:

- `docs/client-ui-focus-navigation-architecture.md`
- `docs/ui-focus-interaction-plan.md`
- `docs/ui-focus-stress-test.md`
- `docs/engine-ui-boundary-plan.md`

The correction aligns those docs on one final architecture:

- `ui/` owns generic focus and primitive UI mechanics.
- `client/ui/` owns screen components, retained `UiScreen` entries,
  `ScreenStack`, `ClientUi`, and UI write draining.
- Screen components use hooks for values and functions.
- Local UI state stays in hook state.
- Clay supplies layout and render commands.
- Directional navigation is derived from harvested Clay rectangles.
- Game/engine code owns the outer tick, world simulation, raw platform input,
  and world rendering.

## Execution

The active docs were audited against
`docs/client-ui-focus-navigation-architecture.md` as the source of truth.

The correction pass:

- rewrote the UI interaction plan in final-state language;
- rewrote the Loadout stress test as an architecture exercise instead of a
  critique of discarded ideas;
- rewrote the engine/UI boundary plan so it treats `ClientUi` as the owner of
  UI stack/focus concerns;
- removed discarded traversal API names from the canonical doc;
- kept examples ordered from `Focusable` and primitive components up through
  screen views, retained screens, `ScreenStack`, `ClientUi`,
  `GameUiPipeline`, and the game tick.

## Verification

Required local checks:

```sh
rg -n "<drift marker regex>" docs
git diff --check
```

The repo-wide marker search may still report historical files under
`docs/archive/` and legacy focus-version directories. Those are not active
architecture docs. The active-doc search must be clean.

The correction is complete only after a read-only thermonuclear review finds no
blocking drift in the four active docs and the final correction commit exists.
