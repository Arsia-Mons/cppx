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

// Per-frame wrapper installed by the App. Receives the `build` callback that
// invokes the visible screens; the wrapper pushes any cross-cutting providers
// (game/app-shell contexts, etc.) and then calls `build()`. Keeps the pipeline
// game-agnostic — the framework just calls a callback without knowing what
// contexts it installs.
using FrameProvider = std::function<void(const std::function<void()> &build)>;

class UiPipeline {
public:
    ClientUi       &client_ui()       { return client_ui_; }
    const ClientUi &client_ui() const { return client_ui_; }

    void set_frame_provider(FrameProvider provider) { frame_provider_ = std::move(provider); }

    void render_client_ui_frame(const UiPipelineFrame    &frame,
                                const RenderClayCommands &render_commands);

private:
    ClientUi      client_ui_;
    FrameProvider frame_provider_;
};

const UiPipelineFrame *use_ui_pipeline_frame();

} // namespace client::ui
