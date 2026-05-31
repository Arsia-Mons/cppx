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

    // Device pixels per UI point (1.0 on a standard display, 2.0 on a 2x Retina
    // panel). UI layout stays in points; the renderer scales by this so geometry
    // and text fill the native-resolution backbuffer crisply. Falls back to 1.0.
    float pixel_density() const;

private:
    SDL_Window   *window_   = nullptr;
    SDL_Renderer *renderer_ = nullptr;
};

} // namespace platform::sdl
