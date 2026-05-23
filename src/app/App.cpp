#include "app/App.h"

#include "client/ui/ClientUi.h"
#include "ui/design/Theme.h"

#include <algorithm>
#include <SDL3/SDL.h>

namespace {

Clay_Dimensions measureTextThunk(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
    return static_cast<renderer::FontRegistry *>(userData)->measureText(text, config);
}

}

bool App::initialize() {
    if (!platform_.initialize("SDL3 Clay Reference", 1280, 720)) {
        return false;
    }

    if (!fonts_.initialize(platform_.renderer())) {
        return false;
    }

    clayRenderer_ = std::make_unique<renderer::SdlClayRenderer>(platform_.renderer(), fonts_);
    clay_ = std::make_unique<ui::ClayService>(1280.0f, 720.0f);
    Clay_SetMeasureTextFunction(measureTextThunk, &fonts_);
    commandRouter_ = std::make_unique<client::ClientCommandRouter>(state_, platform_, running_);

    previousTicks_ = SDL_GetTicks();
    return true;
}

int App::run() {
    while (running_) {
        const uint64_t ticks = SDL_GetTicks();
        const float deltaSeconds = std::max(0.001f, static_cast<float>(ticks - previousTicks_) / 1000.0f);
        previousTicks_ = ticks;

        platform::sdl::PlatformEvent event;
        while (platform_.pollEvent(event)) {
            handleEvent(event);
        }

        renderFrame(deltaSeconds);
        commandRouter_->drain(commands_);
    }
    return 0;
}

void App::handleEvent(const platform::sdl::PlatformEvent &event) {
    using platform::sdl::PlatformEvent;
    using platform::sdl::PlatformKey;

    switch (event.type) {
        case PlatformEvent::Type::Quit:
            running_ = false;
            break;
        case PlatformEvent::Type::TextInput:
            commandRouter_->handle({.type = client::ClientCommandType::AppendText, .text = event.text});
            break;
        case PlatformEvent::Type::KeyDown:
            if (event.key == PlatformKey::Escape) {
                commandRouter_->handle({.type = client::ClientCommandType::BackToMenu});
            } else if (event.key == PlatformKey::Backspace) {
                commandRouter_->handle({.type = client::ClientCommandType::BackspaceFocusedField});
            } else if (event.key == PlatformKey::Return) {
                commandRouter_->handle({.type = client::ClientCommandType::Submit});
            } else if (event.key == PlatformKey::Tab) {
                commandRouter_->handle({.type = client::ClientCommandType::AdvanceLoginFocus});
            }
            break;
    }
}

void App::renderFrame(float deltaSeconds) {
    ui::UiFrameContext frame{callbacks_, textStorage_};
    clay_->beginFrame(platform_.captureUiInput(deltaSeconds), frame);
    client::ui::render(frame, state_, commands_);

    Clay_RenderCommandArray commands = clay_->endFrame();
    platform_.clear(ui::color::Background);
    clayRenderer_->render(commands);
    platform_.present();
}
