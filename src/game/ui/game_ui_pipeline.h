#pragma once

#include <functional>

#include <clay.h>

#include "../../client/ui/client_ui.h"
#include "../../input.h"
#include "../../ui/focus/ui_focus.h"

namespace game::ui {

struct GameUiFrame {
    ::ui::UiInputFrame input = {};
    Clay_Dimensions layout = {};
    Clay_Vector2 pointer = {};
    const InputState *demo_input = nullptr;
};

using RenderClayCommands = std::function<void(Clay_RenderCommandArray &)>;

class GameUiPipeline {
public:
    client::ui::ClientUi &client_ui() { return client_ui_; }
    const client::ui::ClientUi &client_ui() const { return client_ui_; }

    void render_client_ui_frame(const GameUiFrame &frame,
                                const RenderClayCommands &render_commands);

private:
    client::ui::ClientUi client_ui_;
};

const GameUiFrame *use_game_ui_frame();
const InputState *use_demo_app_input();

} // namespace game::ui
