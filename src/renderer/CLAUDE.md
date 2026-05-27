# src/renderer/

Drawing backend: SDL3-based retained UI command rendering + font ownership. This is *not* the place for game-world rendering yet (we don't have one); when it appears, `world_renderer.{h,cpp}` joins this directory.

## Files

- `font_registry.{h,cpp}` — opens a default font and owns `TTF_TextEngine` lifetime.
- `sdl_clay_renderer.{h,cpp}` — legacy compatibility artifact for Clay render commands; not used by the app target.
- `sdl_retained_renderer.{h,cpp}` — translates retained `DrawList` rect/text commands into SDL draw calls.

## Hard rules

- `renderer/` can depend on SDL3, SDL3_ttf, legacy Clay files while they exist, and generic `ui/retained` draw-command types. It must not depend on `client/`, `game/`, or `app/`.
- Fonts and render state are owned here. Other modules ask for measurements / rendered output; they don't reach into renderer internals.
