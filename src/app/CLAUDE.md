# src/app/

Owns process lifecycle and the per-frame loop. `app::App` constructs SDL, fonts, the retained renderer, the `UiPipeline`, the `ShooterGame`, and the `ControlMailbox`; `app::GameLoop::tick()` is the per-frame body (poll events → build input frame → run UI pipeline → render retained draw commands → present → drain deferred mutations).

## Files

- `app.{h,cpp}` — `App::initialize/run/shutdown`. Owns every subsystem; nothing else creates one.
- `game_loop.{h,cpp}` — `GameLoop::tick()`. Per-frame transient flags (`previous_pointer_down_`) plus the active rounded-primitive AA strategy (`renderer::RenderMode`: SSAA / FringeAa / SDF), set from `UI_RENDER_MODE`, cycled with F2, and switchable headlessly via the control mailbox `render_mode` op. Owns the per-frame supersample policy (`supersample_for`) and passes the mode + SDF cache to `execute_draw_commands`. See `renderer/render_mode.h` and `docs/retained-ui/RENDER-MODES.md`.

## Hard rules

- `app/` is the only directory allowed to know about every subsystem at once. Don't push subsystem-wiring into `client/`, `game/`, or `platform/`.
- No game rules here — only orchestration. Game rules live in `game/`.
- No layout-backend calls here — `UiPipeline` owns the retained frame body.
- New per-frame stages go in `GameLoop::tick()`, not bolted on elsewhere.
