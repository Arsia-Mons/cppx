#pragma once

#include <SDL3/SDL.h>

namespace platform::sdl {

class Window {
public:
    Window() = default;
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    bool initialize(const char *title, int width, int height, bool vsync);
    void shutdown();

    SDL_Window   *handle()   const { return window_; }
    SDL_Renderer *renderer() const { return renderer_; }

    void size(int *w, int *h) const;
    bool set_size(int w, int h);
    void set_vsync(bool enabled);

private:
    SDL_Window   *window_   = nullptr;
    SDL_Renderer *renderer_ = nullptr;
};

} // namespace platform::sdl
