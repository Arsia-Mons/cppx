#pragma once

#include "ui/runtime/UiInputState.h"

#include <SDL3/SDL.h>
#include <clay.h>

#include <string>

namespace platform::sdl {

enum class PlatformKey {
    Other,
    Escape,
    Backspace,
    Return,
    Tab,
};

struct PlatformEvent {
    enum class Type {
        Quit,
        TextInput,
        KeyDown,
    };

    Type type = Type::Quit;
    PlatformKey key = PlatformKey::Other;
    std::string text;
};

class SdlPlatform {
public:
    SdlPlatform() = default;
    ~SdlPlatform();

    SdlPlatform(const SdlPlatform &) = delete;
    SdlPlatform &operator=(const SdlPlatform &) = delete;

    bool initialize(const char *title, int width, int height);
    bool pollEvent(PlatformEvent &event);
    ui::UiInputState captureUiInput(float deltaSeconds);

    void clear(Clay_Color color);
    void present();
    void setFullscreen(bool enabled);
    void setVsync(bool enabled);

    SDL_Renderer *renderer() const { return renderer_; }

private:
    SDL_Window *window_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    Clay_Vector2 pendingScroll_{0.0f, 0.0f};

    static PlatformKey mapKey(SDL_Keycode key);
};

}
