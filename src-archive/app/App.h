#pragma once

#include "client/ClientState.h"
#include "client/commands/ClientCommandQueue.h"
#include "client/commands/ClientCommandRouter.h"
#include "platform/sdl/SdlPlatform.h"
#include "renderer/FontRegistry.h"
#include "renderer/SdlClayRenderer.h"
#include "ui/runtime/CallbackStore.h"
#include "ui/runtime/ClayService.h"
#include "ui/runtime/TextStorage.h"

#include <cstdint>
#include <memory>

class App {
public:
    App() = default;
    ~App() = default;

    App(const App &) = delete;
    App &operator=(const App &) = delete;

    bool initialize();
    int run();

private:
    platform::sdl::SdlPlatform platform_;
    renderer::FontRegistry fonts_;
    std::unique_ptr<renderer::SdlClayRenderer> clayRenderer_;
    std::unique_ptr<ui::ClayService> clay_;
    std::unique_ptr<client::ClientCommandRouter> commandRouter_;
    client::ClientState state_;
    client::ClientCommandQueue commands_;
    ui::CallbackStore callbacks_;
    ui::TextStorage textStorage_;
    bool running_ = true;
    uint64_t previousTicks_ = 0;

    void handleEvent(const platform::sdl::PlatformEvent &event);
    void renderFrame(float deltaSeconds);
};
