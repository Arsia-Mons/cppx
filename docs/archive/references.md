# References

This project is grounded in official Clay and SDL3 references first, with third-party indexes used only for navigation.

## Clay

- Clay README: https://github.com/nicbarker/clay
  - Used for Clay's lifecycle, renderer-agnostic model, debug tools, C++20 compatibility, multiple contexts, and render command model.
- Clay `clay.h`: https://github.com/nicbarker/clay/blob/main/clay.h
  - Final authority for public structs, command types, pointer states, scrolling, `Clay_OnHover`, and `Clay_EndLayout(float deltaTime)`.
- SDL3 simple demo: https://github.com/nicbarker/clay/tree/main/examples/SDL3-simple-demo
  - Used for SDL3 + SDL_ttf initialization, text measurement, pointer state, resize, and frame layout ordering.
- SDL3 Clay renderer: https://github.com/nicbarker/clay/blob/main/renderers/SDL3/clay_renderer_SDL3.c
  - Vendored as `third_party/clay/clay_renderer_SDL3.c` and used as the command dispatcher for rectangles, borders, text, images, and scissor clipping. Local changes are limited to include paths, exporting the render function, removing the unused SDL_image include, and matching render font size to the measured font size.
- Raylib sidebar scrolling example: https://github.com/nicbarker/clay/tree/main/examples/raylib-sidebar-scrolling-container
  - Used to cross-check scroll container handling and callback patterns.

## SDL3

- `SDL_CreateRenderer`: https://wiki.libsdl.org/SDL3/SDL_CreateRenderer
  - Documents renderer creation and default renderer selection.
- `SDL_CreateWindowAndRenderer`: https://wiki.libsdl.org/SDL3/SDL_CreateWindowAndRenderer
  - Documents main-thread window/renderer creation and resizable window setup.
- SDL3 render API index: https://wiki.libsdl.org/SDL3/CategoryRender
  - Used for SDL renderer calls and clipping behavior.

## Design Choices

- Clay is treated as the layout and interaction declaration layer only.
- SDL owns the window, event loop, renderer, font resources, and text input.
- UI callbacks enqueue typed actions. App state changes are drained after `Clay_EndLayout`.
- Pointer position, layout dimensions, hit testing, and rendering use window pixel coordinates.
- Scroll deltas are accumulated during SDL events and passed before layout each frame.
- Screen files compose reusable primitives instead of calling SDL directly.
