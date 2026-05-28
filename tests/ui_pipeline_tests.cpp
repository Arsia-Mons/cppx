#include "client/ui/ui_pipeline.h"

#include "client/ui/navigation/ui_screen.h"
#include "react.h"
#include "ui/retained/draw_list.h"
#include "ui/retained/element_components.h"
#include "ui/retained/focus.h"

#include <memory>
#include <stdio.h>
#include <string.h>

#define CHECK(expr)                                                              \
    do {                                                                         \
        if (!(expr)) {                                                           \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__,  \
                    #expr);                                                      \
            return false;                                                        \
        }                                                                        \
    } while (0)

using client::ui::ScreenNavigator;
using client::ui::UiScreen;
using client::ui::UiPipeline;
using client::ui::UiPipelineFrame;

static UiPipelineFrame test_frame(::ui::UiInputFrame input = {}) {
    return {
        .input = input,
        .layout = { 640, 480 },
        .pointer = { -1000.0f, -1000.0f },
    };
}

class FrameProviderProbeScreen final : public UiScreen {
public:
    explicit FrameProviderProbeScreen(bool *observed) : observed_(observed) {}

    const char *debug_name() const override { return "FrameProviderProbe"; }

    void build_ui() override {
        const UiPipelineFrame *frame = client::ui::use_ui_pipeline_frame();
        *observed_ = frame && frame->input.nav_right;
    }

private:
    bool *observed_;
};

struct RetainedProbeScreenProps {
    uint32_t unused = 0;
};

struct PopOnRetainedConfirmScreenProps {
    uint32_t unused = 0;
};

static const char *screen_entry_key(::ui::retained::UiElementFrame &frame,
                                    const char *prefix,
                                    client::ui::UiScreenEntryId entry_id) {
    char key[64] = {};
    snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
    return frame.copy_string(key);
}

static ::ui::retained::UiElement
render_retained_probe_screen(const RetainedProbeScreenProps &props,
                             ::ui::retained::UiElementFrame &frame) {
    (void)props;
    return ::ui::retained::ButtonElement(frame,
                                         {
                                             .key = "confirm",
                                             .id = "PipelineRetainedButton",
                                             .label = "Retained",
                                         });
}

static ::ui::retained::UiElement render_pop_on_retained_confirm_screen(
    const PopOnRetainedConfirmScreenProps &props,
    ::ui::retained::UiElementFrame &frame) {
    (void)props;
    ScreenNavigator nav = client::ui::use_screen_navigator();
    return ::ui::retained::ButtonElement(frame,
                                         {
                                             .key = "pop",
                                             .id = "PipelineRetainedPopButton",
                                             .label = "Pop",
                                             .on_confirm = nav.pop_current,
                                         });
}

class RetainedProbeScreen final : public UiScreen {
public:
    const char *debug_name() const override { return "RetainedProbe"; }

    bool build_element(::ui::retained::UiElementFrame &frame,
                       ::ui::retained::UiElement *out) override {
        if (!out) return false;
        *out = frame.component("RetainedProbeScreenView", RetainedProbeScreenProps{},
                               render_retained_probe_screen,
                               screen_entry_key(frame, "retained-probe", entry_id()));
        return true;
    }

    void build_ui() override {}
};

class PopOnRetainedConfirmScreen final : public UiScreen {
public:
    const char *debug_name() const override { return "PopOnRetainedConfirm"; }

    bool build_element(::ui::retained::UiElementFrame &frame,
                       ::ui::retained::UiElement *out) override {
        if (!out) return false;
        *out = frame.component("PopOnRetainedConfirmScreenView",
                               PopOnRetainedConfirmScreenProps{},
                               render_pop_on_retained_confirm_screen,
                               screen_entry_key(frame, "pop-retained", entry_id()));
        return true;
    }

    void build_ui() override {}
};

struct RenderProbe {
    int render_count = 0;
    int pending_mutations_at_render = 0;
    int screen_count_at_render = 0;
};

struct RetainedRenderProbe {
    int render_count = 0;
    int draw_count = 0;
    bool saw_button_rect = false;
    bool saw_label_text = false;
    ::ui::retained::NodeId button_id = 0;
    ::ui::retained::NodeId focused_id = 0;
};

static bool ui_pipeline_frame_provider_exposes_current_frame(void) {
    react_init_runtime();
    UiPipeline pipeline;
    bool observed = false;

    CHECK(pipeline.client_ui().push_screen(
        std::make_unique<FrameProviderProbeScreen>(&observed)));
    ::ui::UiInputFrame input = {};
    input.nav_right = true;
    pipeline.render_client_ui_frame(test_frame(input), {});

    CHECK(observed);
    return true;
}

static bool pipeline_renders_before_draining_client_mutations(void) {
    react_init_runtime();
    UiPipeline pipeline;
    RenderProbe probe = {};

    CHECK(pipeline.client_ui().push_screen(
        std::make_unique<PopOnRetainedConfirmScreen>()));
    pipeline.render_client_ui_frame(test_frame(), {});
    CHECK(pipeline.client_ui().screens().count() == 1);

    ::ui::UiInputFrame confirm = {};
    confirm.confirm_pressed = true;
    confirm.confirm_down = true;
    confirm.source = ::ui::UiFocusSource::Keyboard;

    pipeline.render_client_ui_frame(test_frame(confirm), [&] {
        probe.render_count += 1;
        probe.pending_mutations_at_render = pipeline.client_ui().pending_mutation_count();
        probe.screen_count_at_render = pipeline.client_ui().screens().count();
    });

    CHECK(probe.render_count == 1);
    CHECK(probe.pending_mutations_at_render == 1);
    CHECK(probe.screen_count_at_render == 1);
    CHECK(pipeline.client_ui().screens().count() == 0);
    return true;
}

static bool pipeline_updates_retained_runtime_before_render(void) {
    react_init_runtime();
    UiPipeline pipeline;
    RetainedRenderProbe probe = {};

    CHECK(pipeline.client_ui().push_screen(std::make_unique<RetainedProbeScreen>()));

    pipeline.render_client_ui_frame(test_frame(), [&] {
        probe.render_count += 1;
        const ::ui::retained::DrawList &draw =
            pipeline.client_ui().retained_draw_list();
        probe.draw_count = draw.count;
        probe.focused_id =
            ::ui::retained::focus_focused_id(pipeline.client_ui().retained_focus());
        for (int i = 0; i < draw.count; ++i) {
            const ::ui::retained::DrawCommand &command = draw.commands[i];
            if (command.kind == ::ui::retained::DrawCommandKind::Rect) {
                probe.saw_button_rect = true;
                probe.button_id = command.node_id;
            }
            if (command.kind == ::ui::retained::DrawCommandKind::Text &&
                strcmp(command.text, "Retained") == 0) {
                probe.saw_label_text = true;
            }
        }
    });

    CHECK(probe.render_count == 1);
    CHECK(probe.draw_count == 2);
    CHECK(probe.saw_button_rect);
    CHECK(probe.saw_label_text);
    CHECK(probe.button_id != 0);
    CHECK(probe.focused_id == probe.button_id);
    return true;
}

static bool pipeline_invokes_retained_confirm_before_render(void) {
    react_init_runtime();
    UiPipeline pipeline;
    RenderProbe probe = {};

    CHECK(pipeline.client_ui().push_screen(
        std::make_unique<PopOnRetainedConfirmScreen>()));
    pipeline.render_client_ui_frame(test_frame(), {});
    CHECK(pipeline.client_ui().screens().count() == 1);

    ::ui::UiInputFrame confirm = {};
    confirm.confirm_pressed = true;
    confirm.confirm_down = true;
    confirm.source = ::ui::UiFocusSource::Keyboard;

    pipeline.render_client_ui_frame(test_frame(confirm), [&] {
        probe.render_count += 1;
        probe.pending_mutations_at_render = pipeline.client_ui().pending_mutation_count();
        probe.screen_count_at_render = pipeline.client_ui().screens().count();
    });

    CHECK(probe.render_count == 1);
    CHECK(probe.pending_mutations_at_render == 1);
    CHECK(probe.screen_count_at_render == 1);
    CHECK(pipeline.client_ui().screens().count() == 0);
    return true;
}

int main(void) {
    if (!ui_pipeline_frame_provider_exposes_current_frame()) return 1;
    if (!pipeline_renders_before_draining_client_mutations()) return 1;
    if (!pipeline_updates_retained_runtime_before_render()) return 1;
    if (!pipeline_invokes_retained_confirm_before_render()) return 1;

    react_shutdown();
    return 0;
}
