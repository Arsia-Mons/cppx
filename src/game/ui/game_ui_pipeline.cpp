#include "game_ui_pipeline.h"

#include "../../react.h"

namespace game::ui {

static ReactContext GameUiFrameContext = {};

const GameUiFrame *use_game_ui_frame() {
    return static_cast<const GameUiFrame *>(use_context(&GameUiFrameContext));
}

void GameUiPipeline::render_client_ui_frame(const GameUiFrame &frame,
                                            const RenderClayCommands &render_commands) {
    Clay_SetLayoutDimensions(frame.layout);
    Clay_SetPointerState(frame.pointer, frame.input.pointer_down);

    client_ui_.begin_frame(frame.input);
    react_begin_frame();
    Clay_BeginLayout();

    REACT_PROVIDER_ENTER("GameUiFrameProvider");
    PROVIDE(&GameUiFrameContext, const_cast<GameUiFrame *>(&frame)) {
        CLAY({
            .id = Clay_GetElementId(CLAY_STRING("ClientUiRoot")),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            },
        }) {
            client_ui_.build_visible_screens();
        }
    }
    REACT_PROVIDER_EXIT();

    Clay_RenderCommandArray commands = Clay_EndLayout();
    client_ui_.end_layout(frame.input);
    react_end_frame();

    if (render_commands) {
        render_commands(commands);
    }

    client_ui_.drain_writes();
}

} // namespace game::ui
