#include "ui_pipeline.h"

#include "../../react.h"
#include "../../ui/retained/components.h"
#include "../../ui/retained/yoga_flex_layout.h"

namespace client::ui {

static ReactContext UiPipelineFrameContext = {};

namespace {

::ui::retained::FocusSource
to_retained_focus_source(::ui::UiFocusSource source) {
    switch (source) {
    case ::ui::UiFocusSource::None:
        return ::ui::retained::FocusSource::None;
    case ::ui::UiFocusSource::Keyboard:
        return ::ui::retained::FocusSource::Keyboard;
    case ::ui::UiFocusSource::Gamepad:
        return ::ui::retained::FocusSource::Gamepad;
    case ::ui::UiFocusSource::Mouse:
        return ::ui::retained::FocusSource::Mouse;
    case ::ui::UiFocusSource::Touch:
        return ::ui::retained::FocusSource::Touch;
    case ::ui::UiFocusSource::Programmatic:
        return ::ui::retained::FocusSource::Programmatic;
    }
    return ::ui::retained::FocusSource::Keyboard;
}

::ui::retained::InputFrame retained_input_frame(const UiPipelineFrame &frame) {
    return {
        .nav_up = frame.input.nav_up,
        .nav_down = frame.input.nav_down,
        .nav_left = frame.input.nav_left,
        .nav_right = frame.input.nav_right,
        .confirm_pressed = frame.input.confirm_pressed,
        .pointer_pressed = frame.input.pointer_pressed,
        .pointer_down = frame.input.pointer_down,
        .pointer_released = frame.input.pointer_released,
        .pointer_valid = true,
        .pointer_x = frame.pointer.x,
        .pointer_y = frame.pointer.y,
        .source = to_retained_focus_source(frame.input.source),
    };
}

bool retained_tree_has_modal(const ::ui::retained::UiTree &tree,
                             ::ui::retained::NodeId id) {
    ::ui::retained::NodeSnapshot node = {};
    if (!tree.snapshot(id, &node))
        return false;
    if (node.interaction.modal)
        return true;
    for (int i = 0; i < tree.child_count(id); ++i) {
        if (retained_tree_has_modal(tree, tree.child_at(id, i)))
            return true;
    }
    return false;
}

::ui::UiInputFrame legacy_input_frame(const UiPipelineFrame &frame,
                                      bool retained_modal_active) {
    if (!retained_modal_active)
        return frame.input;

    ::ui::UiInputFrame input = {};
    input.cancel_pressed = frame.input.cancel_pressed;
    input.cancel_down = frame.input.cancel_down;
    input.cancel_released = frame.input.cancel_released;
    input.source = frame.input.source;
    return input;
}

} // namespace

const UiPipelineFrame *use_ui_pipeline_frame() {
    return static_cast<const UiPipelineFrame *>(use_context(&UiPipelineFrameContext));
}

UiPipeline::UiPipeline()
    : retained_layout_(::ui::retained::make_yoga_flex_layout_adapter()) {}

void UiPipeline::render_client_ui_frame(const UiPipelineFrame    &frame,
                                        const RenderClayCommands &render_commands) {
    Clay_SetLayoutDimensions(frame.layout);
    Clay_SetPointerState(frame.pointer, frame.input.pointer_down);

    bool retained_modal_active = retained_tree_has_modal(
        client_ui_.retained_tree(), client_ui_.retained_tree().root_id());
    ::ui::UiInputFrame legacy_input =
        legacy_input_frame(frame, retained_modal_active);

    client_ui_.begin_frame(legacy_input);
    react_begin_frame();
    bool retained_frame_started = ::ui::retained::begin_retained_tree_frame(
        client_ui_.retained_tree(), frame.layout.width, frame.layout.height);
    if (!retained_frame_started) {
        react_report_error(
            "client/ui: failed to begin retained tree frame\n");
    }
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
    bool retained_frame_ended = false;
    if (retained_frame_started) {
        retained_frame_ended = ::ui::retained::end_retained_tree_frame();
        if (!retained_frame_ended) {
            react_report_error(
                "client/ui: failed to end retained tree frame\n");
        }
    }
    client_ui_.end_layout(legacy_input);

    if (retained_frame_started && retained_frame_ended) {
        bool retained_updated = client_ui_.update_retained_runtime(
            retained_layout_, { frame.layout.width, frame.layout.height },
            retained_input_frame(frame));
        if (!retained_updated) {
            react_report_error(
                "client/ui: failed to update retained runtime\n");
        }
    }
    react_end_frame();

    if (render_commands) {
        render_commands(commands);
    }

    client_ui_.drain_deferred_mutations();
}

} // namespace client::ui
