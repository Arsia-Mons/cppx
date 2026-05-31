# src/client/ui/

Client UI shell: the screen stack, retained focus/runtime glue, the deferred-mutation queue that lets UI emit mutations safely during a retained frame, plus the game's per-screen UI built on top.

## Files

```text
app_shell/                       Game-agnostic framework shell (the "frame around the app"):
  client_ui.{h,cpp}              ClientUi: owns ScreenStack, retained tree/focus/draw runtime, queued mutations.
  ui_pipeline.{h,cpp}            UiPipeline: wraps ClientUi in the frame pass (retained layout/focus/draw phases).
  deferred_ui_mutation.h         internal::DeferredUiMutationSink — host-safe deferred mutation API.
  app_shell_provider.{h,cpp}     AppShellProvider + use_request_quit() (app-level shell context).
  navigation/screen_stack.{h,cpp}  Retained screens, overlay flag, build order, entry IDs.
  navigation/ui_screen.h         UiScreen / OverlayScreen base classes — override build_ui(); inherit OverlayScreen for floating screens.
callback_deps.h                  Hook-dependency capture helper (framework glue, kept at the client/ui root).
providers/shooter_provider.{h,cpp}  Game context provider (use_shooter_game) — game vocabulary stays out of app_shell.
hooks/                           Cross-screen React hooks (e.g., shooter_hud, shooter_weapons).
components/                      Shared semantic app components (the shadcn-style layer):
  tokens.h                       shooter::tokens design tokens (palette + fill/panel/text visual builders).
  actions/                       AppButton, AppCheckbox, AppInput, ActionRow (+ app_button_variant.h, actions.h umbrella).
  layout/                        ScreenLayout (variant: Menu/Game/Overlay/CenteredOverlay).
  surfaces/                      Panel (variant: Hero/Overlay/Sunken).
  text/                          ScreenTitle (variant), ScreenSubtitle, BodyText (variant + tone).
  hud_band.{h,cpp}               In-game HUD band.
screens/<screen>/                Per-screen feature module: <screen>_screen.{hx,cppx|h,cpp},
                                 <screen>_actions.{h,cpp} (public nav/action hooks), optional components/.
```

Client UI is layered as primitives (`src/ui/*`) → shared semantic components (`components/*`) → screen feature modules (`screens/*`). Screen code composes semantic components and passes **variants**; only a component's own implementation touches host/runtime `Style`/`VisualStyle`/events. See `docs/client-ui-component-architecture-goal.md`.

`ScreenNavigator` (`use_screen_navigator`) is the public navigation action hook screens use without mutating the screen stack mid-build. Public navigation/action hooks live in `screens/<screen>/<screen>_actions.{h,cpp}` (e.g. `use_push_options_screen`, `use_start_match`). Domain and screen-local setter hooks may use `internal::DeferredUiMutationSink`; ordinary screen components should expose named actions instead of the sink itself.
`use_screen_is_top()` reads the current `ScreenProvider` top-screen flag; retained overlay screens use it to avoid rendering/trapping input when a higher overlay covers them.

The `app_shell/` files are framework-shaped — they don't know about specific games. Game vocabulary (the `ShooterGame`, `WeaponSpec`, etc.) shows up in `providers/`, `hooks/`, `components/`, and `screens/` because this project has exactly one game (the shooter). If a second game ever appears, promote shared pieces back into framework headers and namespace per-game code accordingly.

## Per-frame contract

`ClientUi` runs three phases each frame — keep them ordered and don't fold work into the wrong one:

1. `begin_frame(input)` — reset per-frame state and discard stale queued mutations.
2. `build_visible_screens()` — invoke each visible screen's `build_ui()` inside the retained tree frame.
3. `end_layout(input)` — handle frame-end navigation such as cancel-pop overlays, then **callers must `drain_deferred_mutations()`** before the next frame's `begin_frame`.

`UiPipeline` (`ui_pipeline.h`) is the standard wrapper that runs these phases plus retained tree begin/end, retained layout/focus/draw updates, renderer handoff, and deferred mutation draining. Prefer using it over calling `ClientUi` directly.

Retained confirm callbacks are dispatched during `ClientUi::update_retained_runtime()`, after retained focus has resolved the confirmed node and before render callbacks. Keep action handlers on retained controls as queued/deferred mutations when they change app or screen state.

## Hard rules

- **Never mutate from inside `build_ui()`**. Product UI should call named hooks/actions; lower-level hooks can submit a `DeferredUiMutation` via `internal::use_deferred_ui_mutations()` or call `ClientUi::queue_*`. The mutation fires after layout, before the next frame.
- `CLIENT_UI_MAX_QUEUED_MUTATIONS` (=128, `client_ui.h`) is a hard cap; if you hit it you have a per-frame mutation leak, not a sizing problem.
- The shell (`app_shell/`) must stay game-agnostic. Game-aware code lives in `providers/`, `hooks/`, `components/`, and `screens/`.
- Screens are owned by `ScreenStack` (`std::unique_ptr<UiScreen>`). Don't hold raw pointers across frames; resolve via `UiScreenEntryId`.

## Testing

`client_ui_tests` in `../../../tests/client_ui_tests.cpp` covers the screen stack + mutation queue; `ui_pipeline_tests` covers the frame wrapper, including retained runtime updates before render callbacks. Add cases there when extending the per-frame contract.
