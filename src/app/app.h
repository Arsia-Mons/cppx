#pragma once

#include "../game/ui/game_ui_pipeline.h"
#include "../platform/control_mailbox.h"
#include "../platform/sdl/window.h"
#include "../renderer/font_registry.h"
#include "../renderer/sdl_clay_renderer.h"
#include "../shooter/shooter_game.h"

#include <clay.h>

namespace app {

struct AppOptions {
    const char *title       = "clay + react hello-world";
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
    static void on_clay_error(Clay_ErrorData err);

    AppOptions                 options_        = {};
    platform::sdl::Window      window_         = {};
    renderer::FontRegistry     fonts_          = {};
    renderer::SdlClayRenderer  clay_render_    = {};
    void                      *clay_arena_mem_ = nullptr;
    Clay_Context              *clay_ctx_       = nullptr;
    game::ui::GameUiPipeline   ui_pipeline_    = {};
    shooter::ShooterGame       shooter_game_   = {};
    platform::ControlMailbox   control_        = {};
    bool                       initialized_    = false;
    bool                       running_        = true;
};

} // namespace app
