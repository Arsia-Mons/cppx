# SDL3 Clay Reference

Gold-standard reference implementation for using [Clay](https://github.com/nicbarker/clay) as a composable UI layer inside a C++ SDL3 game shell.

The app demonstrates:

- Main menu navigation.
- Options sections for controls, audio, and video.
- Login/connect flow with username and password focus handling.
- Lobby with chat, server select, character select, game creation, and play entry.
- Simple game screen with a return path back to the menu.
- Responsive flex layouts, scroll containers, stable Clay IDs, component variants, and a renderer boundary.

## Build

On Windows, use the wrapper:

```powershell
./build.ps1
./build.ps1 -Run
./build.ps1 -Tests
```

The wrapper finds Visual Studio, uses `VCPKG_ROOT` when available, configures
`cmake-build-debug`, and builds the `hello` target by default. SDL3 and
SDL3_ttf are fetched when needed.

## Structure

- `src/app`: application state, navigation, event loop, and SDL-owned runtime setup.
- `src/rendering`: renderer adapters for Clay render commands.
- `src/ui/core`: Clay lifecycle, design tokens, actions, and primitives.
- `src/ui/components`: reserved for larger reusable composed widgets.
- `src/ui/screens`: screen-level compositions.
- `docs/references.md`: source references and integration decisions.

See `docs/architecture.md` for the implementation model and extension points.
