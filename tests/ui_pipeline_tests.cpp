#include "client/ui/ui_pipeline.h"

#include "client/ui/navigation/ui_screen.h"
#include "react.h"
#include "ui/focus/ui_focus.h"
#include "ui/primitives/button.h"

#include <clay.h>

#include <memory>
#include <stdio.h>
#include <stdlib.h>

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

static Clay_Context *g_clay = nullptr;
static void *g_clay_memory = nullptr;

static void on_clay_error(Clay_ErrorData error) {
    fprintf(stderr, "clay: %.*s\n", (int)error.errorText.length, error.errorText.chars);
}

static Clay_Dimensions measure_text(Clay_StringSlice text,
                                    Clay_TextElementConfig *,
                                    void *) {
    return Clay_Dimensions{ (float)text.length * 8.0f, 16.0f };
}

static bool init_clay_once(void) {
    if (g_clay) return true;

    uint32_t clay_memory_size = Clay_MinMemorySize();
    g_clay_memory = malloc(clay_memory_size);
    CHECK(g_clay_memory != nullptr);

    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(clay_memory_size, g_clay_memory);
    g_clay = Clay_Initialize(arena, Clay_Dimensions{ 640, 480 },
                             Clay_ErrorHandler{ on_clay_error, nullptr });
    CHECK(g_clay != nullptr);
    Clay_SetMeasureTextFunction(measure_text, nullptr);
    return true;
}

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

class PopOnConfirmScreen final : public UiScreen {
public:
    const char *debug_name() const override { return "PopOnConfirm"; }

    void build_ui() override {
        REACT_COMPONENT_BEGIN_KEY("PopOnConfirmScreenView", entry_id()) {
            ScreenNavigator nav = client::ui::use_screen_navigator();
            ::ui::ui_focus_push_scope({
                .id = CLAY_ID("PipelineFocusScope"),
            });
            ::ui::ui_focus_request_initial_focus(CLAY_ID("PipelinePopButton"));
            ::ui::Button({
                .id = CLAY_ID("PipelinePopButton"),
                .label = "Pop",
                .on_confirm = nav.pop_current,
            });
            ::ui::ui_focus_pop_scope();
        } REACT_COMPONENT_END();
    }
};

struct RenderProbe {
    int render_count = 0;
    int pending_writes_at_render = 0;
    int screen_count_at_render = 0;
};

static bool ui_pipeline_frame_provider_exposes_current_frame(void) {
    react_init(g_clay);
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

static bool pipeline_renders_before_draining_client_writes(void) {
    react_init(g_clay);
    UiPipeline pipeline;
    RenderProbe probe = {};

    CHECK(pipeline.client_ui().push_screen(std::make_unique<PopOnConfirmScreen>()));
    pipeline.render_client_ui_frame(test_frame(), {});
    CHECK(pipeline.client_ui().screens().count() == 1);

    ::ui::UiInputFrame confirm = {};
    confirm.confirm_pressed = true;
    confirm.confirm_down = true;
    confirm.source = ::ui::UiFocusSource::Keyboard;

    pipeline.render_client_ui_frame(test_frame(confirm), [&](Clay_RenderCommandArray &) {
        probe.render_count += 1;
        probe.pending_writes_at_render = pipeline.client_ui().pending_write_count();
        probe.screen_count_at_render = pipeline.client_ui().screens().count();
    });

    CHECK(probe.render_count == 1);
    CHECK(probe.pending_writes_at_render == 1);
    CHECK(probe.screen_count_at_render == 1);
    CHECK(pipeline.client_ui().screens().count() == 0);
    return true;
}

int main(void) {
    if (!init_clay_once()) return 1;

    if (!ui_pipeline_frame_provider_exposes_current_frame()) return 1;
    if (!pipeline_renders_before_draining_client_writes()) return 1;

    react_shutdown();
    free(g_clay_memory);
    return 0;
}
