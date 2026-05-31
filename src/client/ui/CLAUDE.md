# src/client/ui/

Client UI shell: the screen stack, retained focus/runtime glue, the deferred-mutation queue that lets UI emit mutations safely during a retained frame, plus the game's per-screen UI built on top.

## Authoritative UI conventions

Use this section as the default guidance for future `src/client/ui` work. If
older planning docs disagree, prefer this file and update or delete the stale
guidance as part of the change.

- **Screens are components.** Do not split a screen into `Screen` + `View`
  unless the view is genuinely reused or independently meaningful. The screen
  component should compose hooks and semantic components directly.
- **Public APIs are props, children, hooks, and providers.** Do not expose
  `UiElementFrame`, context bags, retained builders, dispatcher state, or other
  runtime plumbing through authored component contracts.
- **Organize by React role.** Use `components/` for renderable UI, `hooks/` for
  consumer APIs, `providers/` for provider components that own context, and
  `lib/` for private helpers. A component or screen folder may have its own
  `components/`, `hooks/`, `providers/`, or `lib/` only when the concept is local
  to that folder.
- **Context lives with its provider.** Keep each `ReactContext` private in the
  provider implementation. Export the provider from `providers/` and export the
  consumer hook from `hooks/`; the hook implementation may live in the provider
  `.cpp` when that is what keeps the context private.
- **No `*_actions` files.** Navigation to a destination is owned by the caller,
  not the destination. Destination screen modules export screens/components or
  factories; callers compose `use_navigation()` with those destination types.
- **Use capability hooks, not god hooks.** Prefer `use_app()` for app-shell
  capabilities, `use_navigation()` for screen-stack operations and screen
  metadata, and `use_server()` for shooter/domain state and mutations. Do not
  add broad flow hooks that know about specific button flows or child structure.
- **Co-locate first, bubble up only with a real second consumer.** A reusable
  component moves to `client/ui/components/`; a reusable hook moves to
  `client/ui/hooks/`; a reusable provider moves to `client/ui/providers/`.
- **Keep mutations deferred.** UI declaration stays pure. Hide
  `internal::DeferredUiMutationSink` inside domain/screen hooks or providers;
  ordinary components should consume named hooks/capabilities.

## Files

```text
app_shell/                       Game-agnostic framework shell (the "frame around the app"):
  client_ui.{h,cpp}              ClientUi: owns ScreenStack, retained tree/focus/draw runtime, queued mutations.
  ui_pipeline.{h,cpp}            UiPipeline: wraps ClientUi in the frame pass (retained layout/focus/draw phases).
  deferred_ui_mutation.h         internal::DeferredUiMutationSink — host-safe deferred mutation API.
  navigation/screen_stack.{h,cpp}  Retained screens, overlay flag, build order, entry IDs.
  navigation/ui_screen.h         UiScreen / OverlayScreen base classes — override build_ui(); inherit OverlayScreen for floating screens.
callback_deps.h                  Hook-dependency capture helper (framework glue, kept at the client/ui root).
hooks/                           Cross-screen React hooks (e.g. use_app, use_navigation, use_server).
providers/                       Context-owning providers (AppProvider, NavigationProvider, ServerProvider). Keep each ReactContext private in its provider implementation.
components/                      Shared semantic app components (the shadcn-style layer):
  tokens.h                       shooter::tokens design tokens (palette + fill/panel/text visual builders).
  actions/                       AppButton, AppCheckbox, AppInput, ActionRow (+ app_button_variant.h, actions.h umbrella).
  layout/                        ScreenLayout (variant: Menu/Game/Overlay/CenteredOverlay).
  surfaces/                      Panel (variant: Hero/Overlay/Sunken).
  text/                          ScreenTitle (variant), ScreenSubtitle, BodyText (variant + tone).
screens/<screen>/                Per-screen feature module: <screen>_screen.{hx,cppx|h,cpp},
                                 optional components/, hooks/, providers/, lib/.
```

Client UI is layered as primitives (`src/ui/*`) → shared semantic components (`components/*`) → screen feature modules (`screens/*`). Screen code composes semantic components and passes **variants**; only a component's own implementation touches host/runtime `Style`/`VisualStyle`/events. See `docs/client-ui-component-architecture-goal.md`.

`use_navigation()` is the public navigation hook screens use without mutating the screen stack mid-build. Screens compose navigation directly with destination screen types; destination modules export screens/components, not caller-specific hooks. `use_navigation().is_top` reads the current `NavigationProvider` top-screen flag; retained overlay screens use it to avoid rendering/trapping input when a higher overlay covers them.
Use `use_app()` for app-shell capabilities such as quit, and `use_server()` for shooter/domain state and mutations. Domain and screen-local setter hooks may use `internal::DeferredUiMutationSink`; ordinary screen components should expose named hooks instead of the sink itself.

The `app_shell/` files are framework-shaped — they don't know about specific games. Game vocabulary (the `ShooterGame`, `WeaponSpec`, etc.) shows up in `providers/`, `hooks/`, `components/`, and `screens/` because this project has exactly one game (the shooter). If a second game ever appears, promote shared pieces back into framework headers and namespace per-game code accordingly.

## Per-frame contract

`ClientUi` runs three phases each frame — keep them ordered and don't fold work into the wrong one:

1. `begin_frame(input)` — reset per-frame state and discard stale queued mutations.
2. `build_visible_screens()` — invoke each visible screen's `build_ui()` inside the retained tree frame.
3. `end_layout(input)` — handle frame-end navigation such as cancel-pop overlays, then **callers must `drain_deferred_mutations()`** before the next frame's `begin_frame`.

`UiPipeline` (`ui_pipeline.h`) is the standard wrapper that runs these phases plus retained tree begin/end, retained layout/focus/draw updates, renderer handoff, and deferred mutation draining. Prefer using it over calling `ClientUi` directly.

Retained confirm callbacks are dispatched during `ClientUi::update_retained_runtime()`, after retained focus has resolved the confirmed node and before render callbacks. Keep action handlers on retained controls as queued/deferred mutations when they change app or screen state.

## Hard rules

- **Never mutate from inside `build_ui()`**. Product UI should call named hooks/capabilities; lower-level hooks can submit a `DeferredUiMutation` via `internal::use_deferred_ui_mutations()` or call `ClientUi::queue_*`. The mutation fires after layout, before the next frame.
- `CLIENT_UI_MAX_QUEUED_MUTATIONS` (=128, `client_ui.h`) is a hard cap; if you hit it you have a per-frame mutation leak, not a sizing problem.
- The shell (`app_shell/`) must stay game-agnostic. Game-aware code lives in `providers/`, `hooks/`, `components/`, and `screens/`.
- Screens are owned by `ScreenStack` (`std::unique_ptr<UiScreen>`). Don't hold raw pointers across frames; resolve via `UiScreenEntryId`.

## Testing

`client_ui_tests` in `../../../tests/client_ui_tests.cpp` covers the screen stack + mutation queue; `ui_pipeline_tests` covers the frame wrapper, including retained runtime updates before render callbacks. Add cases there when extending the per-frame contract.
