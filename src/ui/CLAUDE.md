# src/ui/

The generic Clay-based UI toolkit. **This directory must not know which game is being built** — no weapons, no loadout, no shooter vocabulary, no replicated game state.

## Layout today

```text
focus/       UiFocusRuntime: scopes, navigation rules, focus intents
primitives/  Button, Toggle, Focusable, Selectable, Clay text helper, visual state
```

The `architecture.md` shows a richer aspirational shape (`runtime/`, `design/`, `layout/`). Today only `focus/` and `primitives/` exist — add new subdirectories when they have multiple files, not preemptively.

## What belongs here

- Layout helpers (Box/Row/Column/Spacer/Divider) once we need them.
- Visual primitives generic enough to drop into any Clay app.
- Focus and input routing in **UI coordinates**, never SDL coordinates.
- Design tokens (colors, typography) — none yet; introduce a `design/` folder when needed.

## What does NOT belong here

- Anything that names a shooter concept (weapon, loadout, HUD, round).
- SDL types or `<SDL.h>` includes — `platform/` adapts those into UI-shaped input.
- References to `client::ui` or `game::ui` — dependency flows the other way.
- Screen-level layout. Screens live in `client/ui/` and game-specific dirs.

## Hooks

Primitives are React-style hook components — see `../../react.h`. Each primitive returns a small handle (focusable token, button result) and uses `REACT_COMPONENT_BEGIN` for stable IDs. Reorderable lists must use `REACT_COMPONENT_BEGIN_KEY` with a stable key.

## Testing

`ui_focus_tests` and `ui_primitives_tests` cover this directory (see `../../CMakeLists.txt:104,122`). They compile against the hook runtime and Clay only — keep it that way. If a new primitive needs SDL, it doesn't belong here.
