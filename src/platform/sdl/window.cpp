#include "window.h"

#include <stdio.h>

namespace platform::sdl {

Window::~Window() {
    shutdown();
}

bool Window::initialize(const char *title, int width, int height, bool vsync) {
    // HIGH_PIXEL_DENSITY: render at the display's native pixel resolution. On a
    // 2x Retina panel the window stays `width`x`height` points but the renderer
    // backbuffer is 2x in each axis, so the UI is drawn crisply at native res
    // instead of being upscaled by the OS (which blurs and re-aliases it).
    window_ = SDL_CreateWindow(title, width, height,
                               SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window_) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return false;
    }
    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(renderer_, vsync ? 1 : 0);
    return true;
}

void Window::shutdown() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
}

void Window::size(int *w, int *h) const {
    if (window_) SDL_GetWindowSize(window_, w, h);
}

bool Window::set_size(int w, int h) {
    return window_ && SDL_SetWindowSize(window_, w, h);
}

void Window::set_vsync(bool enabled) {
    if (renderer_) SDL_SetRenderVSync(renderer_, enabled ? 1 : 0);
}

float Window::pixel_density() const {
    if (!window_) return 1.0f;
    const float d = SDL_GetWindowPixelDensity(window_);
    return d > 0.0f ? d : 1.0f;
}

} // namespace platform::sdl
