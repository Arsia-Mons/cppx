# CPPX

TL;DR:
- Write UI using JSX and React-style hooks (useState, effects, providers) to **cleanly** cross the ui/game data barrier.
- Game-dev style screen stack (pop/push, i.e. accept & cancel/go back)
- Automatic interaction/focusable state management of components - allowing for state-specific visual styling (think css psuedoclasses like `:hover` )
- **Implicit (and explicit) gamepad/keyboard traversal of focusable components!**
- Flexbox layout with Yoga
- Retained mode
- Emit render commands to your renderer of choice, similar to Flutter/Clay

The app demonstrates:

- Main menu, options, pause, in-game HUD, and loadout screens.
- Retained panels, text, buttons, toggles, selectables, generic focusable
  containers, and scroll containers.
- Keyboard/gamepad/pointer focus navigation over retained layout boxes.
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
- `src/ui/runtime`: hook runtime (`react.{h,cpp}`), retained tree, focus, layout, and draw commands.
- `src/ui/components`: generic element-returning UI components.
- `src/renderer`: SDL renderer and font ownership.
- `src/platform`: SDL adapters and CLI control mailbox.
- `src/game`: game rules and state.

See `architecture.md` for the implementation model and extension points.
