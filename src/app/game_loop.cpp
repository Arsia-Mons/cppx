#include "game_loop.h"

#include "../platform/sdl/input.h"
#include "../ui/focus/ui_focus.h"

#include <SDL3/SDL.h>

namespace app {

GameLoop::GameLoop(platform::sdl::Window      &window,
                   renderer::SdlClayRenderer  &clay_render,
                   client::ui::UiPipeline     &ui_pipeline,
                   platform::ControlMailbox   &control,
                   bool                       &running)
    : window_(window),
      clay_render_(clay_render),
      ui_pipeline_(ui_pipeline),
      control_(control),
      running_(running) {}

void GameLoop::tick() {
    ::ui::UiInputFrame ui_input = {};

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_EVENT_QUIT:
                running_ = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (ev.key.repeat) break;
                platform::apply_key_down(ev.key.key, ui_input, &running_);
                break;
            case SDL_EVENT_KEY_UP:
                platform::apply_key_up(ev.key.key, ui_input);
                break;
            default:
                break;
        }
    }

    const bool *keys = SDL_GetKeyboardState(nullptr);
    ui_input.confirm_down = ui_input.confirm_down ||
        keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_SPACE];
    ui_input.cancel_down = ui_input.cancel_down || keys[SDL_SCANCODE_ESCAPE];

    float mx, my;
    Uint32 mb = SDL_GetMouseState(&mx, &my);
    bool pointer_down = (mb & SDL_BUTTON_LMASK) != 0;
    ui_input.pointer_down     = pointer_down;
    ui_input.pointer_pressed  = pointer_down && !previous_pointer_down_;
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
        .input   = ui_input,
        .layout  = { (float)frame_w, (float)frame_h },
        .pointer = { mx, my },
    };

    ui_pipeline_.render_client_ui_frame(frame, [&](Clay_RenderCommandArray &cmds) {
        clay_render_.clear({ 12, 14, 22, 255 });
        clay_render_.render(cmds);
        control_.capture_after_render(clay_render_.sdl_renderer(), ui_pipeline_);
        clay_render_.present();
    });
    control_.finish_frame(ui_pipeline_);
}

} // namespace app
