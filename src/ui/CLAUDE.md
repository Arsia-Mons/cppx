# src/ui/

The generic UI toolkit. **This directory must not know which game is being built** — no weapons, no loadout, no shooter vocabulary, no replicated game state.

## Layout today

```text
input.h      UI-shaped input frame shared by app/client/runtime code
span.h       tiny non-owning span used by framework containers
retained/    UiTree, UiElement/reconciler, retained components, flex layout, focus, draw commands
```

The retained runtime is the app path. Add new subdirectories only when they
have multiple files, not preemptively.

## What belongs here

- Layout helpers (Box/Row/Column/Spacer/Divider) once we need them.
- Visual primitives generic enough to drop into any retained UI app.
- Focus and input routing in **UI coordinates**, never SDL coordinates.
- Design tokens (colors, typography) — none yet; introduce a `design/` folder when needed.

## What does NOT belong here

- Anything that names a shooter concept (weapon, loadout, HUD, round).
- SDL types or `<SDL.h>` includes — `platform/` adapts those into UI-shaped input.
- References to `client::ui` or `game::ui` — dependency flows the other way.
- Screen-level layout. Screens live in `client/ui/` and game-specific dirs.

## Composition

The target authoring model is returned `UiElement` descriptions committed by
the reconciler in `retained/element.*`. The older retained component helpers
still exist while app/client code migrates, but new generic runtime work should
move toward `UiElement` factories, provider elements, and reconciler-owned hook
entry/exit. Reorderable lists must use stable keys.

## Testing

Retained runtime coverage lives in `retained_ui_*_tests`. If a new primitive needs SDL, it doesn't belong here.
