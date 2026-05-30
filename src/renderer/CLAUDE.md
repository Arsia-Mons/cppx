# src/renderer/

Drawing backend: SDL3-based retained UI command rendering + font ownership. This is *not* the place for game-world rendering yet (we don't have one); when it appears, `world_renderer.{h,cpp}` joins this directory.

## Files

- `font_registry.{h,cpp}` — opens a default font and owns `TTF_TextEngine` lifetime.
- `draw_executor.{h,cpp}` — executes the tagged-union `DrawCommandList` IR (`ui/runtime/draw_command.h`) into SDL draw calls. Takes a `RenderMode` that selects how rounded vector primitives are rasterized.
- `render_mode.h` — `RenderMode {Ssaa, FringeAa, Sdf}`, the single source of truth for the rounded-primitive AA strategy, plus the pure mapping functions both pipeline stages read. SDL-free pure data. See `docs/retained-ui/RENDER-MODES.md`.
- `sdf_raster.{h,cpp}` — signed-distance-field rasterizer for `RenderMode::Sdf`: analytic per-pixel coverage masks (fill / border+outline rings / gradient), cached by device-pixel geometry.
- `ui_surface.{h,cpp}` — the minimal SDL surface the per-frame loop draws into (clear/present + the SDL_Renderer and FontRegistry the executor needs). Owns the SSAA target and the `SdfMaskCache`.
- `texture_registry.{h,cpp}` — owns decoded textures for image draw commands.

## Hard rules

- `renderer/` can depend on SDL3, SDL3_ttf, and generic `ui/runtime` draw-command types. It must not depend on `client/`, `game/`, or `app/`.
- Fonts and render state are owned here. Other modules ask for measurements / rendered output; they don't reach into renderer internals.
