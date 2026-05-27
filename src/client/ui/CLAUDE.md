# src/client/ui/

Client UI shell: the screen stack, focus glue, the deferred-write queue that lets UI emit mutations safely during a Clay pass, plus the game's per-screen UI built on top.

## Files

```text
client_ui.{h,cpp}                ClientUi shell: owns ScreenStack + UiFocusRuntime,
                                 runs per-frame phases, queues deferred writes.
ui_pipeline.{h,cpp}              UiPipeline: wraps ClientUi in a Clay frame pass.
navigation/screen_stack.{h,cpp}  Retained screens, overlay flag, build order, entry IDs.
navigation/ui_screen.h           UiScreen interface — override build_ui() and (optionally) is_overlay().
providers/                       Cross-screen React context providers (e.g., shooter_provider).
hooks/                           Cross-screen React hooks (e.g., shooter_hud, shooter_weapons).
components/                      Cross-screen visual components (e.g., hud_band).
screens/<screen>/                Per-screen dir: <screen>_screen.{h,cpp} + optional components/.
```

`ScreenNavigator` (`use_screen_navigator`) and `QueueUiWrite` (`use_ui_write_queue`) are the hooks screens use to mutate navigation/state without touching the Clay tree mid-build.

The shell files (`client_ui`, `ui_pipeline`, `navigation/`) are framework-shaped — they don't know about specific games. Game vocabulary (the `ShooterGame`, `WeaponSpec`, etc.) shows up in `providers/`, `hooks/`, `components/`, and `screens/` because this project has exactly one game (the shooter). If a second game ever appears, promote shared pieces back into framework headers and namespace per-game code accordingly.

## Per-frame contract

`ClientUi` runs three phases each frame — keep them ordered and don't fold work into the wrong one:

1. `begin_frame(input)` — feed input to focus runtime; reset per-frame state.
2. `build_visible_screens()` — invoke each visible screen's `build_ui()` (a Clay layout pass).
3. `end_layout(input)` — finalize focus, then **callers must `drain_writes()`** before the next frame's `begin_frame`.

`UiPipeline` (`ui_pipeline.h`) is the standard wrapper that runs these phases plus Clay begin/end and render-command emission. Prefer using it over calling `ClientUi` directly.

## Hard rules

- **Never mutate from inside `build_ui()`**. Push a `UiDeferredWrite` via `use_ui_write_queue()` or call `ClientUi::queue_*`. The write fires after layout, before the next frame.
- `CLIENT_UI_MAX_WRITES` (=128, `client_ui.h`) is a hard cap; if you hit it you have a per-frame mutation leak, not a sizing problem.
- The shell (`client_ui`, `ui_pipeline`, `navigation/`) must stay game-agnostic. Game-aware code lives in `providers/`, `hooks/`, `components/`, and `screens/`.
- Screens are owned by `ScreenStack` (`std::unique_ptr<UiScreen>`). Don't hold raw pointers across frames; resolve via `UiScreenEntryId`.

## Testing

`client_ui_tests` in `../../../tests/client_ui_tests.cpp` covers the screen stack + write queue; `ui_pipeline_tests` covers the Clay frame wrapper. Add cases there when extending the per-frame contract.
