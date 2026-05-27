#include "ui_pipeline.h"

#include "../../react.h"

namespace client::ui {

static ReactContext UiPipelineFrameContext = {};

const UiPipelineFrame *use_ui_pipeline_frame() {
    return static_cast<const UiPipelineFrame *>(use_context(&UiPipelineFrameContext));
}

void UiPipeline::render_client_ui_frame(const UiPipelineFrame    &frame,
                                        const RenderClayCommands &render_commands) {
    Clay_SetLayoutDimensions(frame.layout);
    Clay_SetPointerState(frame.pointer, frame.input.pointer_down);

    client_ui_.begin_frame(frame.input);
    react_begin_frame();
    Clay_BeginLayout();

    REACT_PROVIDER_ENTER("UiPipelineFrameProvider");
    PROVIDE(&UiPipelineFrameContext, const_cast<UiPipelineFrame *>(&frame)) {
        CLAY({
            .id = Clay_GetElementId(CLAY_STRING("ClientUiRoot")),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            },
        }) {
            auto build = [this] { client_ui_.build_visible_screens(); };
            if (frame_provider_) {
                frame_provider_(build);
            } else {
                build();
            }
        }
    }
    REACT_PROVIDER_EXIT();

    Clay_RenderCommandArray commands = Clay_EndLayout();
    client_ui_.end_layout(frame.input);
    react_end_frame();

    if (render_commands) {
        render_commands(commands);
    }

    client_ui_.drain_deferred_mutations();
}

} // namespace client::ui
