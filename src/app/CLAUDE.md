# src/app/

Owns process lifecycle and the per-frame loop. `app::App` constructs SDL, fonts, Clay, the `UiPipeline`, the `ShooterGame`, and the `ControlMailbox`; `app::GameLoop::tick()` is the per-frame body (poll events → build input frame → run UI pipeline → render → present → drain deferred writes).

## Files

- `app.{h,cpp}` — `App::initialize/run/shutdown`. Owns every subsystem; nothing else creates one.
- `game_loop.{h,cpp}` — `GameLoop::tick()`. Stateless except for per-frame transient flags (`previous_pointer_down_`).

## Hard rules

- `app/` is the only directory allowed to know about every subsystem at once. Don't push subsystem-wiring into `client/`, `game/`, or `platform/`.
- No game rules here — only orchestration. Game rules live in `game/`.
- No Clay layout calls here — `UiPipeline` owns the frame body.
- New per-frame stages go in `GameLoop::tick()`, not bolted on elsewhere.
