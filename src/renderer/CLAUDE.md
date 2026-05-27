# src/renderer/

Drawing backend: SDL3-based Clay command rendering + font measurement. This is *not* the place for game-world rendering yet (we don't have one); when it appears, `world_renderer.{h,cpp}` joins this directory.

## Files

- `font_registry.{h,cpp}` — opens a default font, exposes `measure(...)` for Clay's text-measurement callback. Owns `TTF_TextEngine` lifetime.
- `sdl_clay_renderer.{h,cpp}` — translates `Clay_RenderCommandArray` into SDL draw calls.

## Hard rules

- `renderer/` can depend on SDL3, SDL3_ttf, and Clay. It must not depend on `client/`, `game/`, or `app/`.
- Fonts and render state are owned here. Other modules ask for measurements / rendered output; they don't reach into renderer internals.
