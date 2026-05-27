# src/

Source tree. Boundaries here are deliberate — see `../architecture.md` for the full mental model and ownership rules.

## Layering

```text
app/      process lifecycle and the per-frame loop
ui/       generic UI toolkit/runtime (no game vocabulary)
client/   client UI shell (screen stack, mutation queue, focus glue) + the game's screens
game/     game rules and state (player, weapons, economy, inventory)
platform/ OS/library adapters (SDL window/input, control mailbox)
renderer/ font + UI→SDL render glue
react.{h,cpp}  React-style hook runtime over Clay/retained component trees
main.cpp  ~15-line entrypoint: builds AppOptions, hands off to app::App
```

Allowed dependency direction: `client/ui/screens → client/ui → game → ui → react → clay`. `app/` composes everything; `platform/` and `renderer/` are siblings consumed by `app/`. `game/` must not depend on Clay/SDL/UI.

## Where new code goes

- **Game rules / state** → `game/`.
- **A new screen** → `client/ui/screens/<screen_name>/<screen_name>_screen.{h,cpp}` (with `components/` subdir if the screen has its own components).
- **Cross-screen UI hook** → `client/ui/hooks/`.
- **Cross-screen UI component** → `client/ui/components/`.
- **Cross-screen UI provider** → `client/ui/providers/`.
- **Generic widget** (no game vocabulary) → `ui/primitives/`.
- **OS/SDL-specific glue** → `platform/sdl/`.
- **Rendering backend code** → `renderer/`.
- **Per-frame loop / app lifecycle** → `app/`.

## Hard rules (from architecture.md, not optional)

- `ui/` must not know about game concepts.
- `game/` must not include Clay or SDL headers.
- UI must queue mutations during a Clay pass via `client::ui::ClientUi::queue_deferred_mutation` (see `client/ui/client_ui.h`).
- Game-specific UI screens live under `client/ui/screens/<screen>/`, not in `game/`.

## Tests

Each subsystem has a focused test binary registered in `../CMakeLists.txt`: `react_runtime_tests`, `ui_focus_tests`, `ui_primitives_tests`, `client_ui_tests`, `ui_pipeline_tests`, `shooter_ui_tests`, plus three Python CLI smoke tests (`ui_cli_smoke`, `ui_cli_dm`, `ui_cli_commands`). Add a matching test target when introducing a new module — keep its dependency list minimal so the test compiles in isolation.
