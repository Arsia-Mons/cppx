#include "game_loop.h"

#include "../platform/sdl/input.h"
#include "../renderer/draw_executor.h"

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace app {

GameLoop::GameLoop(platform::sdl::Window &window, renderer::UiSurface &surface,
                   client::ui::UiPipeline &ui_pipeline,
                   platform::ControlMailbox &control, bool &running)
    : window_(window), surface_(surface), ui_pipeline_(ui_pipeline),
      control_(control), running_(running) {
  // Initial render mode from the environment (UI_RENDER_MODE=ssaa|fringe|sdf),
  // so headless captures can pick a mode without a keyboard. Unset/unknown keeps
  // the built-in default.
  if (const char *env = std::getenv("UI_RENDER_MODE")) {
    renderer::RenderMode m = render_mode_;
    if (renderer::render_mode_from_slug(env, &m))
      render_mode_ = m;
  }
  // Expose the active mode in control-mailbox state replies (readback for
  // headless tests). GameLoop owns the mode, so it registers the slug closure —
  // platform/ never learns the RenderMode enum.
  control_.set_render_mode_provider(
      [this] { return renderer::render_mode_slug(render_mode_); });
  apply_window_title();
}

void GameLoop::set_render_mode(renderer::RenderMode mode) {
  if (mode == render_mode_)
    return;
  render_mode_ = mode;
  apply_window_title();
}

void GameLoop::apply_window_title() {
  if (!window_.handle())
    return;
  char title[128];
  std::snprintf(title, sizeof(title),
                "retained ui  —  Render: %s   [F2 to cycle]",
                renderer::render_mode_name(render_mode_));
  SDL_SetWindowTitle(window_.handle(), title);
}

void GameLoop::tick() {
  ::ui::UiInputFrame ui_input = {};

  SDL_Event ev;
  while (SDL_PollEvent(&ev)) {
    switch (ev.type) {
    case SDL_EVENT_QUIT:
      running_ = false;
      break;
    case SDL_EVENT_KEY_DOWN:
      if (ev.key.repeat)
        break;
      // Global dev toggle: F2 cycles the rounded-primitive AA strategy
      // (SSAA -> Fringe AA -> SDF). Intercepted here so it never reaches the UI
      // focus/navigation layer.
      if (ev.key.key == SDLK_F2) {
        set_render_mode(renderer::next_render_mode(render_mode_));
        break;
      }
      platform::apply_key_down(ev.key.key, ui_input, &running_);
      break;
    case SDL_EVENT_KEY_UP:
      platform::apply_key_up(ev.key.key, ui_input);
      break;
    case SDL_EVENT_TEXT_INPUT:
      platform::apply_text_input(ev.text.text, ui_input);
      break;
    case SDL_EVENT_TEXT_EDITING:
      platform::apply_text_editing(ev.edit.text, ev.edit.start, ev.edit.length,
                                   ui_input);
      break;
    default:
      break;
    }
  }

  const bool *keys = SDL_GetKeyboardState(nullptr);
  ui_input.confirm_down = ui_input.confirm_down || keys[SDL_SCANCODE_RETURN] ||
                          keys[SDL_SCANCODE_SPACE];
  ui_input.cancel_down = ui_input.cancel_down || keys[SDL_SCANCODE_ESCAPE];

  float mx, my;
  Uint32 mb = SDL_GetMouseState(&mx, &my);
  bool pointer_down = (mb & SDL_BUTTON_LMASK) != 0;
  ui_input.pointer_down = pointer_down;
  ui_input.pointer_pressed = pointer_down && !previous_pointer_down_;
  ui_input.pointer_released = !pointer_down && previous_pointer_down_;
  if (ui_input.pointer_pressed || ui_input.pointer_released) {
    ui_input.source = ::ui::UiFocusSource::Mouse;
  }
  previous_pointer_down_ = pointer_down;

  control_.poll(ui_input, running_, window_.handle(), ui_pipeline_);
  // Headless render-mode switching (control mailbox "render_mode" op). The
  // mailbox forwards a raw slug; we map it here (app/ owns the renderer enum).
  std::string mode_slug;
  if (control_.take_pending_render_mode(&mode_slug)) {
    renderer::RenderMode m = render_mode_;
    if (renderer::render_mode_from_slug(mode_slug.c_str(), &m))
      set_render_mode(m);
  }
  if (control_.apply_pointer_override(mx, my, pointer_down)) {
    ui_input.pointer_down = pointer_down;
  }

  int frame_w = 800, frame_h = 500;
  window_.size(&frame_w, &frame_h);
  client::ui::UiPipelineFrame frame = {
      .input = ui_input,
      .layout = {(float)frame_w, (float)frame_h},
      .pointer = {mx, my},
  };

  ui_pipeline_.render_client_ui_frame(frame, [&] {
    const float density = window_.pixel_density();
    // Supersample factor is a pure function of the active render mode: only SSAA
    // mode supersamples (and only on a standard-density panel). Fringe/SDF are
    // analytic per-primitive and render at native resolution.
    const int supersample = renderer::supersample_for(render_mode_, density);
    // begin_frame binds the (supersample) target + clears, returning the
    // effective device scale to render at. The live path renders the tagged-
    // union IR through execute_draw_commands; resolve_frame downsamples to the
    // window. Fonts come from the surface (owns the FontRegistry from init).
    const float scale =
        surface_.begin_frame({12, 14, 22, 255}, density, supersample);
    renderer::execute_draw_commands(
        surface_.sdl_renderer(),
        ui_pipeline_.client_ui().retained_command_list(), surface_.fonts(),
        /*textures=*/nullptr, scale, {render_mode_, surface_.sdf_cache()});
    surface_.resolve_frame();
    control_.capture_after_render(surface_.sdl_renderer(), ui_pipeline_);
    surface_.present();
  });
  control_.finish_frame(ui_pipeline_);

  bool wants_text_input = ui_pipeline_.client_ui().wants_text_input();
  if (wants_text_input != text_input_active_) {
    if (wants_text_input) {
      SDL_StartTextInput(window_.handle());
    } else {
      SDL_StopTextInput(window_.handle());
    }
    text_input_active_ = wants_text_input;
  }
}

} // namespace app
