#pragma once

#include "../client/ui/ui_pipeline.h"
#include "../platform/control_mailbox.h"
#include "../platform/sdl/window.h"
#include "../renderer/sdl_clay_renderer.h"
#include "../renderer/sdl_retained_renderer.h"

namespace app {

class GameLoop {
public:
    GameLoop(platform::sdl::Window      &window,
             renderer::SdlClayRenderer  &clay_render,
             renderer::SdlRetainedRenderer &retained_render,
             client::ui::UiPipeline   &ui_pipeline,
             platform::ControlMailbox   &control,
             bool                       &running);

    void tick();

private:
    platform::sdl::Window      &window_;
    renderer::SdlClayRenderer  &clay_render_;
    renderer::SdlRetainedRenderer &retained_render_;
    client::ui::UiPipeline   &ui_pipeline_;
    platform::ControlMailbox   &control_;
    bool                       &running_;
    bool                        previous_pointer_down_ = false;
};

} // namespace app
