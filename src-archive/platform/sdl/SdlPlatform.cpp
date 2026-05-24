#include "platform/sdl/SdlPlatform.h"

namespace platform::sdl {

SdlPlatform::~SdlPlatform() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
    }
    if (window_) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
}

bool SdlPlatform::initialize(const char *title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_CreateWindowAndRenderer(title, width, height, SDL_WINDOW_RESIZABLE, &window_, &renderer_)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window/renderer: %s", SDL_GetError());
        return false;
    }

    SDL_SetRenderVSync(renderer_, 1);
    SDL_StartTextInput(window_);
    return true;
}

bool SdlPlatform::pollEvent(PlatformEvent &out) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                out = {.type = PlatformEvent::Type::Quit};
                return true;
            case SDL_EVENT_MOUSE_WHEEL:
                pendingScroll_.x += event.wheel.x * 32.0f;
                pendingScroll_.y += event.wheel.y * 32.0f;
                break;
            case SDL_EVENT_TEXT_INPUT:
                out = {.type = PlatformEvent::Type::TextInput, .text = event.text.text};
                return true;
            case SDL_EVENT_KEY_DOWN:
                out = {.type = PlatformEvent::Type::KeyDown, .key = mapKey(event.key.key)};
                return true;
            default:
                break;
        }
    }
    return false;
}

ui::UiInputState SdlPlatform::captureUiInput(float deltaSeconds) {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window_, &width, &height);

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    const SDL_MouseButtonFlags mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
    const Clay_Vector2 scrollDelta = pendingScroll_;
    pendingScroll_ = {0.0f, 0.0f};

    return {
        .width = static_cast<float>(width),
        .height = static_cast<float>(height),
        .mouseX = mouseX,
        .mouseY = mouseY,
        .pointerDown = (mouseButtons & SDL_BUTTON_LMASK) != 0,
        .scrollDelta = scrollDelta,
        .deltaSeconds = deltaSeconds
    };
}

void SdlPlatform::clear(Clay_Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer_);
}

void SdlPlatform::present() {
    SDL_RenderPresent(renderer_);
}

void SdlPlatform::setFullscreen(bool enabled) {
    SDL_SetWindowFullscreen(window_, enabled);
}

void SdlPlatform::setVsync(bool enabled) {
    SDL_SetRenderVSync(renderer_, enabled ? 1 : 0);
}

PlatformKey SdlPlatform::mapKey(SDL_Keycode key) {
    switch (key) {
        case SDLK_ESCAPE:
            return PlatformKey::Escape;
        case SDLK_BACKSPACE:
            return PlatformKey::Backspace;
        case SDLK_RETURN:
            return PlatformKey::Return;
        case SDLK_TAB:
            return PlatformKey::Tab;
        default:
            return PlatformKey::Other;
    }
}

}
