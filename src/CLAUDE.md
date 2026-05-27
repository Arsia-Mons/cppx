# src/

Source tree. Boundaries here are deliberate — see `../architecture.md` for the full mental model and ownership rules.

## Layering

```text
ui/       generic Clay toolkit (no game vocabulary)
client/   local-player app shell (screen stack, write queue, focus glue)
game/     game-agnostic UI pipeline + (eventually) shared rules/state
platform/ OS/library adapters (SDL input normalization, control mailbox)
shooter/  the example game and its screens — stand-in for a real game tree
react.{h,cpp}  React-style hook runtime layered on Clay
main.cpp  SDL/Clay shell; currently also holds what should eventually be `app/`
```

Allowed dependency direction: `shooter → game → client → ui → react → clay`. `platform/` is a sibling that everyone above `ui/` may use. Don't reach upward.

## Where new code goes

- **Generic widget or layout helper** (no game words) → `ui/primitives/` or `ui/focus/`.
- **Game-agnostic UI infrastructure** (routing, modal stack, focus wiring) → `client/ui/`.
- **Per-frame pipeline glue** (Clay layout → render → drain writes) → `game/ui/`.
- **A new screen, HUD piece, or shooter overlay** → `shooter/` next to `shooter_ui.*`. Promote upward only when a second consumer appears (`architecture.md` "Co-Location Rule").
- **SDL or OS-specific code** → `platform/`. Never let SDL types leak into `game/` or `ui/`.

## Hard rules (from architecture.md, not optional)

- `ui/` must not know about shooter concepts (weapons, loadouts, HUDs).
- UI must not mutate game state during a Clay pass — queue via `client::ui::ClientUi::queue_deferred_write` (see `client/ui/client_ui.h:43`).
- Game truth lives in `game/` (or `shooter/` today); UI consumes read-only view data and emits typed intent.
- No Clay/SDL types in `game/` rule code.

## Tests

Each subsystem has a focused test binary registered in `../CMakeLists.txt:102+`: `react_runtime_tests`, `ui_focus_tests`, `ui_primitives_tests`, `client_ui_tests`, `game_ui_pipeline_tests`, `shooter_ui_tests`. Add a matching test target when introducing a new module — keep its dependency list minimal so the test compiles in isolation.
