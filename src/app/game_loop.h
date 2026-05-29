#pragma once

#include "../client/ui/ui_pipeline.h"
#include "../platform/control_mailbox.h"
#include "../platform/sdl/window.h"
#include "../renderer/ui_surface.h"

namespace app {

class GameLoop {
public:
  GameLoop(platform::sdl::Window &window, renderer::UiSurface &surface,
           client::ui::UiPipeline &ui_pipeline,
           platform::ControlMailbox &control, bool &running);

  void tick();

private:
  platform::sdl::Window &window_;
  renderer::UiSurface &surface_;
  client::ui::UiPipeline &ui_pipeline_;
  platform::ControlMailbox &control_;
  bool &running_;
  bool previous_pointer_down_ = false;
  bool text_input_active_ = false;
};

} // namespace app
