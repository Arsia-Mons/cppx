#include "game_loop.h"

#include "../platform/sdl/input.h"
#include "../renderer/draw_executor.h"

#include <SDL3/SDL.h>

namespace app {

GameLoop::GameLoop(platform::sdl::Window &window, renderer::UiSurface &surface,
                   client::ui::UiPipeline &ui_pipeline,
                   platform::ControlMailbox &control, bool &running)
    : window_(window), surface_(surface), ui_pipeline_(ui_pipeline),
      control_(control), running_(running) {}

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
    surface_.clear({12, 14, 22, 255});
    // The live path renders the tagged-union IR through execute_draw_commands.
    // Fonts come from the surface, which owns the FontRegistry handed to it at
    // initialize().
    renderer::execute_draw_commands(
        surface_.sdl_renderer(),
        ui_pipeline_.client_ui().retained_command_list(), surface_.fonts(),
        /*textures=*/nullptr, window_.pixel_density());
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
