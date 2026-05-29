#pragma once

#include "../client/ui/ui_pipeline.h"
#include "../platform/control_mailbox.h"
#include "../platform/sdl/window.h"
#include "../renderer/font_registry.h"
#include "../renderer/ui_surface.h"
#include "../game/shooter_game.h"

namespace app {

struct AppOptions {
    const char *title       = "retained ui hello-world";
    int         width       = 800;
    int         height      = 500;
    const char *control_dir = nullptr;
};

class App {
public:
    App();
    ~App();

    App(const App &) = delete;
    App &operator=(const App &) = delete;

    bool initialize(const AppOptions &options);
    int  run();
    void shutdown();

private:
    AppOptions                 options_        = {};
    platform::sdl::Window      window_         = {};
    renderer::FontRegistry     fonts_          = {};
    renderer::UiSurface        surface_        = {};
    client::ui::UiPipeline     ui_pipeline_    = {};
    shooter::ShooterGame       shooter_game_   = {};
    platform::ControlMailbox   control_        = {};
    bool                       initialized_    = false;
    bool                       running_        = true;
};

} // namespace app
