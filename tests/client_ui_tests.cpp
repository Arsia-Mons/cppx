#include "client/ui/client_ui.h"
#include "react.h"
#include "ui/retained/components.h"
#include "ui/retained/yoga_flex_layout.h"

#include <clay.h>

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr)                                                              \
    do {                                                                         \
        if (!(expr)) {                                                           \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__,  \
                    #expr);                                                      \
            return false;                                                        \
        }                                                                        \
    } while (0)

using namespace client::ui;

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

class RecordingScreen final : public UiScreen {
public:
    RecordingScreen(const char *name, bool overlay, int *build_count)
        : UiScreen(overlay ? ScreenKind::Overlay : ScreenKind::Normal),
          name_(name), build_count_(build_count) {}

    const char *debug_name() const override { return name_; }

    void build_ui() override {
        if (build_count_) {
            *build_count_ += 1;
        }
    }

private:
    const char *name_;
    int *build_count_;
};

class PopSelfScreen final : public OverlayScreen {
public:
    explicit PopSelfScreen(int *build_count) : build_count_(build_count) {}

    const char *debug_name() const override { return "PopSelf"; }

    void build_ui() override {
        if (build_count_) {
            *build_count_ += 1;
        }
        ScreenNavigator nav = use_screen_navigator();
        nav.pop_current();
    }

private:
    int *build_count_;
};

class PushScreen final : public UiScreen {
public:
    explicit PushScreen(int *build_count) : build_count_(build_count) {}

    const char *debug_name() const override { return "PushScreen"; }

    void build_ui() override {
        if (build_count_) {
            *build_count_ += 1;
        }
        ScreenNavigator nav = use_screen_navigator();
        nav.push(std::make_unique<RecordingScreen>("Pushed", false, nullptr));
    }

private:
    int *build_count_;
};

class ResetToScreen final : public OverlayScreen {
public:
    ResetToScreen(int *build_count, int *destroy_count)
        : build_count_(build_count), destroy_count_(destroy_count) {}
    ~ResetToScreen() override {
        if (destroy_count_) {
            *destroy_count_ += 1;
        }
    }

    const char *debug_name() const override { return "ResetToScreen"; }

    void build_ui() override {
        if (build_count_) {
            *build_count_ += 1;
        }
        ScreenNavigator nav = use_screen_navigator();
        nav.reset_to(std::make_unique<RecordingScreen>("ResetRoot", false, nullptr));
    }

private:
    int *build_count_;
    int *destroy_count_;
};

class DestroyCountingScreen final : public UiScreen {
public:
    explicit DestroyCountingScreen(int *destroy_count) : destroy_count_(destroy_count) {}
    ~DestroyCountingScreen() override {
        if (destroy_count_) {
            *destroy_count_ += 1;
        }
    }

    const char *debug_name() const override { return "DestroyCounting"; }
    void build_ui() override {}

private:
    int *destroy_count_;
};

class HookStateScreen final : public UiScreen {
public:
    explicit HookStateScreen(int *observed) : observed_(observed) {}

    const char *debug_name() const override { return "HookState"; }

    void build_ui() override {
        REACT_COMPONENT_BEGIN_KEY("HookStateScreenView", entry_id()) {
            int *value = use_state_int(0);
            *observed_ = *value;
            *value += 1;
        } REACT_COMPONENT_END();
    }

private:
    int *observed_;
};

static void run_client_frame(ClientUi &client_ui, const ::ui::UiInputFrame &input = {}) {
    Clay_SetLayoutDimensions({ 640, 480 });
    Clay_SetPointerState({ -1000.0f, -1000.0f }, input.pointer_down);
    client_ui.begin_frame(input);
    react_begin_frame();
    Clay_BeginLayout();
    CLAY({
        .id = Clay_GetElementId(CLAY_STRING("ClientUiRoot")),
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
        },
    }) {
        client_ui.build_visible_screens();
    }
    (void)Clay_EndLayout();
    client_ui.end_layout(input);
    react_end_frame();
    client_ui.drain_deferred_mutations();
}

static bool screen_stack_push_pop_replace_and_visible_ordering(void) {
    ScreenStack stack;
    CHECK(stack.push(std::make_unique<RecordingScreen>("Base", false, nullptr)));
    CHECK(stack.push(std::make_unique<RecordingScreen>("Overlay", true, nullptr)));
    CHECK(stack.count() == 2);

    ::ui::Span<UiScreen *> visible = stack.visible_screens();
    CHECK(visible.count == 2);
    CHECK(strcmp(visible[0]->debug_name(), "Base") == 0);
    CHECK(strcmp(visible[1]->debug_name(), "Overlay") == 0);

    CHECK(stack.push(std::make_unique<RecordingScreen>("Opaque", false, nullptr)));
    visible = stack.visible_screens();
    CHECK(visible.count == 1);
    CHECK(strcmp(visible[0]->debug_name(), "Opaque") == 0);

    CHECK(stack.replace_top(std::make_unique<RecordingScreen>("Replacement", false, nullptr)));
    CHECK(strcmp(stack.top()->debug_name(), "Replacement") == 0);
    CHECK(stack.pop_top());
    CHECK(stack.count() == 2);
    CHECK(stack.pop_entry(stack.top()->entry_id()));
    CHECK(stack.count() == 1);
    CHECK(stack.reset_to(std::make_unique<RecordingScreen>("Root", false, nullptr)));
    CHECK(stack.count() == 1);
    CHECK(strcmp(stack.top()->debug_name(), "Root") == 0);
    return true;
}

static bool client_ui_builds_visible_screens_in_order(void) {
    react_init(g_clay);
    ClientUi client_ui;
    int base_builds = 0;
    int overlay_builds = 0;
    int hidden_builds = 0;

    CHECK(client_ui.push_screen(std::make_unique<RecordingScreen>("Base", false, &base_builds)));
    CHECK(client_ui.push_screen(std::make_unique<RecordingScreen>("Overlay", true, &overlay_builds)));
    run_client_frame(client_ui);

    CHECK(base_builds == 1);
    CHECK(overlay_builds == 1);

    CHECK(client_ui.push_screen(std::make_unique<RecordingScreen>("HiddenBase", false, &hidden_builds)));
    run_client_frame(client_ui);

    CHECK(base_builds == 1);
    CHECK(overlay_builds == 1);
    CHECK(hidden_builds == 1);
    return true;
}

static bool overlay_screens_float_over_base_screen_area(void) {
    react_init(g_clay);
    ClientUi client_ui;
    int base_builds = 0;
    int overlay_builds = 0;

    CHECK(client_ui.push_screen(std::make_unique<RecordingScreen>("Base", false, &base_builds)));
    CHECK(client_ui.push_screen(
        std::make_unique<RecordingScreen>("Overlay", true, &overlay_builds)));
    UiScreenEntryId base_id = client_ui.screens().at(0)->entry_id();
    UiScreenEntryId overlay_id = client_ui.screens().at(1)->entry_id();

    run_client_frame(client_ui);

    Clay_ElementData base_frame =
        Clay_GetElementData(CLAY_IDI("ClientUiScreenFrame", base_id));
    Clay_ElementData overlay_frame =
        Clay_GetElementData(CLAY_IDI("ClientUiOverlayScreenFrame", overlay_id));
    CHECK(base_frame.found);
    CHECK(overlay_frame.found);
    CHECK(base_frame.boundingBox.x == 0.0f);
    CHECK(base_frame.boundingBox.y == 0.0f);
    CHECK(base_frame.boundingBox.width == 640.0f);
    CHECK(base_frame.boundingBox.height == 480.0f);
    CHECK(overlay_frame.boundingBox.x == 0.0f);
    CHECK(overlay_frame.boundingBox.y == 0.0f);
    CHECK(overlay_frame.boundingBox.width == 640.0f);
    CHECK(overlay_frame.boundingBox.height == 480.0f);
    CHECK(base_builds == 1);
    CHECK(overlay_builds == 1);
    return true;
}

static bool screen_navigator_pop_current_drains_after_layout(void) {
    react_init(g_clay);
    ClientUi client_ui;
    int base_builds = 0;
    int overlay_builds = 0;

    CHECK(client_ui.push_screen(std::make_unique<RecordingScreen>("Base", false, &base_builds)));
    CHECK(client_ui.push_screen(std::make_unique<PopSelfScreen>(&overlay_builds)));

    Clay_SetLayoutDimensions({ 640, 480 });
    Clay_SetPointerState({ -1000.0f, -1000.0f }, false);
    client_ui.begin_frame({});
    react_begin_frame();
    Clay_BeginLayout();
    client_ui.build_visible_screens();
    CHECK(client_ui.screens().count() == 2);
    CHECK(client_ui.pending_mutation_count() == 1);
    (void)Clay_EndLayout();
    client_ui.end_layout({});
    react_end_frame();

    CHECK(client_ui.screens().count() == 2);
    client_ui.drain_deferred_mutations();
    CHECK(client_ui.screens().count() == 1);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "Base") == 0);
    CHECK(base_builds == 1);
    CHECK(overlay_builds == 1);
    return true;
}

static bool screen_navigator_push_drains_after_layout(void) {
    react_init(g_clay);
    ClientUi client_ui;
    int build_count = 0;

    CHECK(client_ui.push_screen(std::make_unique<PushScreen>(&build_count)));

    Clay_SetLayoutDimensions({ 640, 480 });
    Clay_SetPointerState({ -1000.0f, -1000.0f }, false);
    client_ui.begin_frame({});
    react_begin_frame();
    Clay_BeginLayout();
    client_ui.build_visible_screens();
    CHECK(client_ui.screens().count() == 1);
    CHECK(client_ui.pending_mutation_count() == 1);
    (void)Clay_EndLayout();
    client_ui.end_layout({});
    react_end_frame();

    CHECK(client_ui.screens().count() == 1);
    client_ui.drain_deferred_mutations();
    CHECK(client_ui.screens().count() == 2);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "Pushed") == 0);
    CHECK(build_count == 1);
    return true;
}

static bool screen_navigator_reset_to_drains_after_layout(void) {
    react_init(g_clay);
    ClientUi client_ui;
    int base_destroy_count = 0;
    int reset_build_count = 0;
    int reset_destroy_count = 0;

    CHECK(client_ui.push_screen(std::make_unique<DestroyCountingScreen>(&base_destroy_count)));
    CHECK(client_ui.push_screen(
        std::make_unique<ResetToScreen>(&reset_build_count, &reset_destroy_count)));
    UiScreenEntryId old_top_id = client_ui.screens().top()->entry_id();

    Clay_SetLayoutDimensions({ 640, 480 });
    Clay_SetPointerState({ -1000.0f, -1000.0f }, false);
    client_ui.begin_frame({});
    react_begin_frame();
    Clay_BeginLayout();
    client_ui.build_visible_screens();
    CHECK(client_ui.screens().count() == 2);
    CHECK(client_ui.pending_mutation_count() == 1);
    (void)Clay_EndLayout();
    client_ui.end_layout({});
    react_end_frame();

    CHECK(client_ui.screens().count() == 2);
    client_ui.drain_deferred_mutations();
    CHECK(client_ui.screens().count() == 1);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "ResetRoot") == 0);
    CHECK(client_ui.screens().top()->entry_id() > old_top_id);
    CHECK(base_destroy_count == 1);
    CHECK(reset_destroy_count == 1);
    CHECK(reset_build_count == 1);
    return true;
}

static bool cancel_pops_top_overlay_after_layout(void) {
    react_init(g_clay);
    ClientUi client_ui;
    int base_builds = 0;
    int overlay_builds = 0;

    CHECK(client_ui.push_screen(std::make_unique<RecordingScreen>("Base", false, &base_builds)));
    CHECK(client_ui.push_screen(
        std::make_unique<RecordingScreen>("Overlay", true, &overlay_builds)));

    ::ui::UiInputFrame cancel = {
        .cancel_pressed = true,
        .cancel_down = true,
        .source = ::ui::UiFocusSource::Keyboard,
    };
    run_client_frame(client_ui, cancel);

    CHECK(client_ui.screens().count() == 1);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "Base") == 0);
    CHECK(base_builds == 1);
    CHECK(overlay_builds == 1);
    return true;
}

static bool queued_push_screen_releases_if_frame_resets_before_drain(void) {
    react_init(g_clay);
    ClientUi client_ui;
    int destroy_count = 0;

    CHECK(client_ui.queue_push_screen(std::make_unique<DestroyCountingScreen>(&destroy_count)));
    CHECK(client_ui.pending_mutation_count() == 1);
    CHECK(destroy_count == 0);

    client_ui.begin_frame({});
    CHECK(client_ui.pending_mutation_count() == 0);
    CHECK(destroy_count == 1);
    return true;
}

static bool screen_local_hook_state_survives_rerender_and_resets_on_unmount(void) {
    react_init(g_clay);
    ClientUi client_ui;
    int observed = -1;

    CHECK(client_ui.push_screen(std::make_unique<HookStateScreen>(&observed)));
    run_client_frame(client_ui);
    CHECK(observed == 0);

    run_client_frame(client_ui);
    CHECK(observed == 1);

    CHECK(client_ui.screens().pop_top());
    CHECK(client_ui.push_screen(std::make_unique<HookStateScreen>(&observed)));
    run_client_frame(client_ui);
    CHECK(observed == 0);
    return true;
}

static bool client_ui_owns_retained_runtime_outputs(void) {
    react_init_runtime();
    ClientUi client_ui;

    CHECK(::ui::retained::begin_retained_frame(client_ui.retained_tree(), 240.0f, 120.0f));
    ::ui::retained::Button(::ui::retained::ButtonProps{
        .key = "confirm",
        .id = "ConfirmRetainedButton",
        .label = "Confirm",
    });
    CHECK(::ui::retained::end_retained_frame());

    ::ui::retained::NodeId button_id = client_ui.retained_tree().child_at(client_ui.retained_tree().root_id(), 0);
    CHECK(button_id != 0);

    ::ui::retained::FlexLayoutAdapter adapter = ::ui::retained::make_yoga_flex_layout_adapter();
    CHECK(client_ui.update_retained_runtime(adapter, { 240.0f, 120.0f }, {}));
    CHECK(::ui::retained::focus_focused_id(client_ui.retained_focus()) == button_id);

    const ::ui::retained::DrawList &draw = client_ui.retained_draw_list();
    CHECK(draw.error_count == 0);
    CHECK(draw.count == 2);
    CHECK(draw.commands[0].kind == ::ui::retained::DrawCommandKind::Rect);
    CHECK(draw.commands[0].node_id == button_id);
    CHECK(draw.commands[1].kind == ::ui::retained::DrawCommandKind::Text);
    CHECK(strcmp(draw.commands[1].text, "Confirm") == 0);
    return true;
}

int main(void) {
    if (!init_clay_once()) return 1;

    if (!screen_stack_push_pop_replace_and_visible_ordering()) return 1;
    if (!client_ui_builds_visible_screens_in_order()) return 1;
    if (!overlay_screens_float_over_base_screen_area()) return 1;
    if (!screen_navigator_pop_current_drains_after_layout()) return 1;
    if (!screen_navigator_push_drains_after_layout()) return 1;
    if (!screen_navigator_reset_to_drains_after_layout()) return 1;
    if (!cancel_pops_top_overlay_after_layout()) return 1;
    if (!queued_push_screen_releases_if_frame_resets_before_drain()) return 1;
    if (!screen_local_hook_state_survives_rerender_and_resets_on_unmount()) return 1;
    if (!client_ui_owns_retained_runtime_outputs()) return 1;

    react_shutdown();
    free(g_clay_memory);
    return 0;
}
