# src/client/ui/

Game-agnostic client UI shell: the screen stack, focus glue, and the deferred-write queue that lets UI emit mutations safely during a Clay pass.

## Files

```text
client_ui.{h,cpp}    ClientUi: owns ScreenStack + UiFocusRuntime, runs per-frame phases,
                     queues deferred writes (push/pop/arbitrary fn) drained after layout.
screen_stack.{h,cpp} Retained screens, overlay flag, build order, entry IDs.
ui_screen.h          UiScreen interface — override build_ui() and (optionally) is_overlay().
```

`ScreenNavigator` (use_screen_navigator) and `QueueUiWrite` (use_ui_write_queue) are the hooks screens use to mutate navigation/state without touching the Clay tree mid-build.

## Per-frame contract

`ClientUi` runs three phases each frame — keep them ordered and don't fold work into the wrong one:

1. `begin_frame(input)` — feed input to focus runtime; reset per-frame state.
2. `build_visible_screens()` — invoke each visible screen's `build_ui()` (a Clay layout pass).
3. `end_layout(input)` — finalize focus, then **callers must `drain_writes()`** before the next frame's `begin_frame`.

`GameUiPipeline` (`../../game/ui/game_ui_pipeline.h`) is the standard wrapper that runs these phases plus Clay begin/end and render-command emission. Prefer using it over calling `ClientUi` directly.

## Hard rules

- **Never mutate from inside `build_ui()`**. Push a `UiDeferredWrite` via `use_ui_write_queue()` or call `ClientUi::queue_*`. The write fires after layout, before the next frame.
- `CLIENT_UI_MAX_WRITES` (=128, `client_ui.h:13`) is a hard cap; if you hit it you have a per-frame mutation leak, not a sizing problem.
- This directory must stay game-agnostic. Shooter screens live in `../../shooter/`, not here. New game-specific overlays go under their game's directory or — for a real game — a `client/ui/screens|overlays|hud/` tree per `../../../architecture.md`.
- Screens are owned by `ScreenStack` (`std::unique_ptr<UiScreen>`). Don't hold raw pointers across frames; resolve via `UiScreenEntryId`.

## Testing

`client_ui_tests` in `../../../tests/client_ui_tests.cpp` covers the screen stack + write queue. Add cases there when extending the per-frame contract.
