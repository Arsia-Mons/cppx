#pragma once

#include "../client/ui/ui_pipeline.h"
#include "../platform/control_mailbox.h"
#include "../platform/sdl/window.h"
#include "../renderer/render_mode.h"
#include "../renderer/ui_surface.h"

namespace app {

class GameLoop {
public:
  GameLoop(platform::sdl::Window &window, renderer::UiSurface &surface,
           client::ui::UiPipeline &ui_pipeline,
           platform::ControlMailbox &control, bool &running);

  void tick();

  // The active rounded-primitive AA strategy (see renderer/render_mode.h). The
  // initial value is read from the UI_RENDER_MODE env var at construction; the
  // F2 key cycles it and the control mailbox can set it for headless tests.
  renderer::RenderMode render_mode() const { return render_mode_; }
  void set_render_mode(renderer::RenderMode mode);

private:
  void apply_window_title(); // reflects the active mode in the window title

  platform::sdl::Window &window_;
  renderer::UiSurface &surface_;
  client::ui::UiPipeline &ui_pipeline_;
  platform::ControlMailbox &control_;
  bool &running_;
  bool previous_pointer_down_ = false;
  bool text_input_active_ = false;
  renderer::RenderMode render_mode_ = renderer::RenderMode::Ssaa;
};

} // namespace app
