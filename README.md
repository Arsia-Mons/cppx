# SDL3 Retained UI Reference

Reference implementation for a retained game UI inside a C++20 / SDL3 game
shell. The UI uses a small React-style hook runtime, a retained tree, Yoga for
flex layout, retained focus/event routing, and SDL render-command output.

The app demonstrates:

- Main menu, options, pause, in-game HUD, and loadout screens.
- Retained panels, text, buttons, toggles, selectables, generic focusable
  containers, and scroll containers.
- Keyboard/gamepad/pointer focus navigation over retained layout boxes.
- Deferred UI mutations so game and screen state are not changed during the
  declaration pass.
- Deterministic CLI control through the mailbox protocol used by smoke tests.
- JSX-like `.cppx` / `.hx` generated C++ fixtures for retained components.

## Build

On macOS/Linux:

```sh
./build.sh
./build.sh --run
./build.sh --tests
```

On Windows:

```powershell
./build.ps1
./build.ps1 -Run
./build.ps1 -Tests
```

The wrappers configure `cmake-build-debug` by default, build the `hello` target,
and can run the full CTest suite. SDL3 and SDL3_ttf are fetched when needed.

## Structure

- `src/app`: process lifecycle and the per-frame loop.
- `src/client/ui`: screen stack, providers, hooks, components, and screens.
- `src/ui`: generic retained UI runtime, components, focus, layout, and draw
  commands.
- `src/renderer`: SDL renderer and font ownership.
- `src/platform`: SDL adapters and CLI control mailbox.
- `src/game`: game rules and state.
- `src/react.{h,cpp}`: hook runtime.

See `architecture.md` for the implementation model and extension points.
