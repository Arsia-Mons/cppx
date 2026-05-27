# src/platform/

OS and library adapters. SDL window/renderer/input are isolated here so the rest of the tree doesn't include `<SDL3/SDL.h>`. The control mailbox is the headless test IPC (file-based request/reply) used by `tools/ui_cli.py`.

## Files

- `sdl/window.{h,cpp}` — `SDL_Window` + `SDL_Renderer` lifetime, resize, vsync.
- `sdl/input.{h,cpp}` — SDL keycodes + gamepad buttons → `::ui::UiInputFrame`.
- `control_mailbox.{h,cpp}` — JSON-over-files IPC for the headless CLI tests.
  Inspect/state replies expose retained focus and focusables so
  `tools/ui_cli.py` can target controls deterministically.

## Hard rules

- SDL types may appear in this directory's headers (they're SDL adapters). Other directories include these adapters but never include SDL headers directly outside `renderer/` and `app/`.
- New OS adapters (audio device, clipboard, etc.) join `platform/sdl/` (or a sibling `platform/<lib>/` if non-SDL).
- `control_mailbox` is test infrastructure, not gameplay infrastructure. Keep it at the parent level so it's discoverable as a sibling of `sdl/`.
