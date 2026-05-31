# src/ui/

The generic UI toolkit. **This directory must not know which game is being built** — no weapons, no loadout, no shooter vocabulary, no replicated game state.

## Layout today

```text
input.h      UI-shaped input frame shared by app/client/runtime code
span.h       tiny non-owning span used by framework containers
components/  generic element-returning UI components
runtime/     UiTree, UiElement/reconciler, flex layout, focus, draw commands
```

The retained runtime is the app path. Add new subdirectories only when they
have multiple files, not preemptively.

## What belongs here

- Layout helpers (Box/Row/Column/Spacer/Divider) once we need them.
- Visual primitives generic enough to drop into any retained UI app.
- Focus and input routing in **UI coordinates**, never SDL coordinates.
- The theme **mechanism** — `style/` (`Theme`/`RoleStyle` types, `ThemeContext`, `use_theme()`, `resolve()`, `StylePatch`, the `patch()` builder) and a **neutral** `default_theme()` fallback. Primitives accept a per-instance `StylePatch style_override`, merged over the theme role by `resolve()` (never a dense full-`VisualStyle` override).

## What does NOT belong here

- Anything that names a shooter concept (weapon, loadout, HUD, round).
- An authored *product* theme (a concrete palette/values). The theme mechanism lives here, but the values are a **client** concern, installed via a `ThemeProvider` over `ThemeContext`.
- SDL types or `<SDL.h>` includes — `platform/` adapts those into UI-shaped input.
- References to `client::ui` or `game::ui` — dependency flows the other way.
- Screen-level layout. Screens live in `client/ui/` and game-specific dirs.

## Composition

The target authoring model is returned `UiElement` descriptions committed by
the reconciler in `runtime/element.*`. Generic UI components that return
element descriptions live in `components/`, one component per file, with the
umbrella include at `components/components.h`. New generic runtime work should
move toward `UiElement` factories, provider elements, and reconciler-owned hook
entry/exit. Reorderable lists must use stable keys.

## Testing

Retained runtime coverage lives in `retained_ui_*_tests`. If a new primitive needs SDL, it doesn't belong here.
