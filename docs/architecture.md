# Architecture

The implementation follows a game-style split:

- `App` owns SDL, long-lived game state, navigation, and input focus.
- `ClayService` owns Clay initialization and per-frame lifecycle calls.
- `SdlClayRenderer` converts Clay render commands into SDL draw calls from the concern-oriented `rendering` layer.
- `ui::primitive` functions declare reusable UI atoms and small molecules.
- Screen renderers compose primitives and queue intents.

## Frame Order

1. SDL events update platform input, text fields, and high-level hotkeys.
2. `ClayService::beginFrame` sets layout dimensions, pointer state, and scroll state.
3. The active screen declares a complete Clay tree.
4. `Clay_EndLayout(deltaTime)` computes render commands.
5. Queued UI actions are drained into application state.
6. `SdlClayRenderer` draws the command array.

This keeps Clay callbacks free of destructive state changes during layout.

## Extending

- Add new reusable widgets under `src/ui/components` when a screen-local pattern becomes shared.
- Add new screens under `src/ui/screens` and expose them through `Screens.h`.
- Keep SDL-specific runtime setup in `src/app` and command rendering in `src/rendering`.
- Prefer stable `CLAY_ID` and `CLAY_IDI` values for interactive, repeated, scrollable, or debugged elements.
