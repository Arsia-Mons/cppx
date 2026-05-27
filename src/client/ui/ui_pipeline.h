#pragma once

#include <functional>

#include <clay.h>

#include "client_ui.h"
#include "../../ui/focus/ui_focus.h"

namespace client::ui {

struct UiPipelineFrame {
    ::ui::UiInputFrame input   = {};
    Clay_Dimensions    layout  = {};
    Clay_Vector2       pointer = {};
};

using RenderClayCommands = std::function<void(Clay_RenderCommandArray &)>;

class UiPipeline {
public:
    ClientUi       &client_ui()       { return client_ui_; }
    const ClientUi &client_ui() const { return client_ui_; }

    void render_client_ui_frame(const UiPipelineFrame    &frame,
                                const RenderClayCommands &render_commands);

private:
    ClientUi client_ui_;
};

const UiPipelineFrame *use_ui_pipeline_frame();

} // namespace client::ui
