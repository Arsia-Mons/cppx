#include "ui/focus/ui_focus.h"

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

using namespace ui;

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

static Clay_ElementId test_id(const char *name) {
    return Clay_GetElementId(Clay_String{ false, (int32_t)strlen(name), name });
}

static bool same_id(Clay_ElementId a, Clay_ElementId b) {
    return a.id != 0 && a.id == b.id;
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

static void focus_box(Clay_ElementId id,
                      bool disabled = false,
                      UiNavRules nav = {},
                      std::function<void()> on_confirm = {},
                      std::function<void()> on_focus = {}) {
    (void)ui_focusable({
        .id = id,
        .disabled = disabled,
        .nav = nav,
        .on_confirm = on_confirm,
        .on_focus = on_focus,
    });
    CLAY({
        .id = id,
        .layout = {
            .sizing = { CLAY_SIZING_FIXED(60), CLAY_SIZING_FIXED(32) },
        },
    }) {}
}

template <typename Build>
static void run_focus_frame(UiFocusRuntime &focus,
                            const UiInputFrame &input,
                            Build build,
                            Clay_Dimensions dimensions = { 640, 480 },
                            Clay_Vector2 pointer = { -1000.0f, -1000.0f }) {
    ui_focus_set_current(&focus);
    Clay_SetLayoutDimensions(dimensions);
    Clay_SetPointerState(pointer, input.pointer_down);
    ui_focus_begin_frame(input);
    Clay_BeginLayout();
    CLAY({
        .id = test_id("TestRoot"),
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .childGap = 8,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        build();
    }
    (void)Clay_EndLayout();
    ui_focus_end_layout(input);
}

static void simple_stack_scope(Clay_ElementId scope,
                               Clay_ElementId a,
                               Clay_ElementId b,
                               Clay_ElementId c,
                               bool b_disabled = false) {
    ui_focus_push_scope({ .id = scope });
    CLAY({
        .id = test_id("Stack"),
        .layout = {
            .childGap = 8,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        focus_box(a);
        focus_box(b, b_disabled);
        focus_box(c);
    }
    ui_focus_pop_scope();
}

static bool stacked_buttons_navigate_from_harvested_rectangles(void) {
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("StackScope");
    Clay_ElementId a = test_id("StackA");
    Clay_ElementId b = test_id("StackB");
    Clay_ElementId c = test_id("StackC");

    run_focus_frame(focus, {}, [&] {
        simple_stack_scope(scope, a, b, c);
    });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), a));

    run_focus_frame(focus, { .nav_down = true, .source = UiFocusSource::Keyboard }, [&] {
        simple_stack_scope(scope, a, b, c);
    });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), b));

    run_focus_frame(focus, { .nav_down = true, .source = UiFocusSource::Gamepad }, [&] {
        simple_stack_scope(scope, a, b, c);
    });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), c));
    CHECK(ui_focus_source_for_scope(scope) == UiFocusSource::Gamepad);

    run_focus_frame(focus, { .nav_up = true, .source = UiFocusSource::Keyboard }, [&] {
        simple_stack_scope(scope, a, b, c);
    });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), b));
    return true;
}

static bool grid_navigation_uses_geometry_without_neighbor_tables(void) {
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("GridScope");
    Clay_ElementId top = test_id("GridTop");
    Clay_ElementId lower_left = test_id("GridLowerLeft");
    Clay_ElementId lower_right = test_id("GridLowerRight");

    auto build_grid = [&] {
        ui_focus_push_scope({ .id = scope });
        CLAY({
            .id = test_id("GridRows"),
            .layout = {
                .childGap = 8,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }) {
            CLAY({ .id = test_id("GridRowTop") }) {
                focus_box(top);
            }
            CLAY({
                .id = test_id("GridRowBottom"),
                .layout = {
                    .childGap = 8,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }) {
                focus_box(lower_left);
                focus_box(lower_right);
            }
        }
        ui_focus_pop_scope();
    };

    run_focus_frame(focus, {}, build_grid);
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), top));

    run_focus_frame(focus, { .nav_down = true }, build_grid);
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), lower_left));

    run_focus_frame(focus, { .nav_right = true }, build_grid);
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), lower_right));
    return true;
}

static bool disabled_controls_are_skipped_for_navigation_and_confirm(void) {
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("DisabledScope");
    Clay_ElementId a = test_id("DisabledA");
    Clay_ElementId b = test_id("DisabledB");
    Clay_ElementId c = test_id("DisabledC");
    int confirm_count = 0;

    run_focus_frame(focus, {}, [&] {
        simple_stack_scope(scope, a, b, c, true);
    });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), a));

    run_focus_frame(focus, { .nav_down = true }, [&] {
        simple_stack_scope(scope, a, b, c, true);
    });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), c));

    run_focus_frame(focus, { .confirm_pressed = true }, [&] {
        ui_focus_push_scope({ .id = scope });
        focus_box(a, false, {}, [&] { confirm_count++; });
        focus_box(c, true, {}, [&] { confirm_count++; });
        ui_focus_pop_scope();
    });
    CHECK(confirm_count == 0);
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), a));
    return true;
}

static bool local_boundary_rules_stop_wrap_and_explicit_targets(void) {
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("RulesScope");
    Clay_ElementId a = test_id("RulesA");
    Clay_ElementId b = test_id("RulesB");
    Clay_ElementId c = test_id("RulesC");

    UiNavRules a_rules = {};
    a_rules.left.kind = UiNavRuleKind::Stop;
    a_rules.right.kind = UiNavRuleKind::Explicit;
    a_rules.right.explicit_target = c;

    UiNavRules c_rules = {};
    c_rules.right.kind = UiNavRuleKind::Wrap;

    auto build = [&] {
        ui_focus_push_scope({ .id = scope });
        CLAY({
            .id = test_id("RulesRow"),
            .layout = {
                .childGap = 8,
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            focus_box(a, false, a_rules);
            focus_box(b);
            focus_box(c, false, c_rules);
        }
        ui_focus_pop_scope();
    };

    run_focus_frame(focus, {}, build);
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), a));

    run_focus_frame(focus, { .nav_left = true }, build);
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), a));

    run_focus_frame(focus, { .nav_right = true }, build);
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), c));

    run_focus_frame(focus, { .nav_right = true }, build);
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), a));
    return true;
}

static bool focus_callbacks_use_current_frame_registration(void) {
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("CallbackScope");
    Clay_ElementId a = test_id("CallbackA");
    Clay_ElementId b = test_id("CallbackB");
    int callback_value = 0;

    auto build = [&](int version, Clay_ElementId initial = {}) {
        ui_focus_push_scope({ .id = scope });
        if (initial.id != 0) {
            ui_focus_request_initial_focus(initial);
        }
        focus_box(a, false, {}, {}, [&, version] {
            callback_value = version * 10 + 1;
        });
        focus_box(b, false, {}, {}, [&, version] {
            callback_value = version * 10 + 2;
        });
        ui_focus_pop_scope();
    };

    run_focus_frame(focus, {}, [&] {
        build(1, b);
    });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), b));
    CHECK(callback_value == 12);

    run_focus_frame(focus, { .nav_up = true }, [&] {
        build(2);
    });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), a));
    CHECK(callback_value == 21);
    return true;
}

static bool frame_local_callbacks_are_released_after_dispatch(void) {
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("CallbackLifetimeScope");
    Clay_ElementId a = test_id("CallbackLifetimeA");
    std::weak_ptr<int> callback_capture;

    run_focus_frame(focus, {}, [&] {
        std::shared_ptr<int> captured = std::make_shared<int>(42);
        callback_capture = captured;
        ui_focus_push_scope({ .id = scope });
        focus_box(a, false, {}, [captured] {}, [captured] {});
        ui_focus_pop_scope();
    });

    CHECK(callback_capture.expired());
    return true;
}

static bool focus_source_tracks_mouse_and_touch_inputs(void) {
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("SourceScope");
    Clay_ElementId a = test_id("SourceA");
    Clay_ElementId b = test_id("SourceB");

    run_focus_frame(focus, {}, [&] {
        simple_stack_scope(scope, a, b, test_id("SourceC"));
    });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), a));

    run_focus_frame(focus, { .nav_down = true, .source = UiFocusSource::Mouse }, [&] {
        simple_stack_scope(scope, a, b, test_id("SourceC"));
    });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), b));
    CHECK(ui_focus_source_for_scope(scope) == UiFocusSource::Mouse);

    run_focus_frame(focus, { .nav_up = true, .source = UiFocusSource::Touch }, [&] {
        simple_stack_scope(scope, a, b, test_id("SourceC"));
    });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), a));
    CHECK(ui_focus_source_for_scope(scope) == UiFocusSource::Touch);
    return true;
}

static bool pointer_release_confirms_only_original_hovered_target(void) {
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("PointerScope");
    Clay_ElementId a = test_id("PointerA");
    Clay_ElementId b = test_id("PointerB");
    int confirm_count = 0;

    auto build = [&] {
        ui_focus_push_scope({ .id = scope });
        focus_box(a, false, {}, [&] { confirm_count++; });
        focus_box(b, false, {}, [&] { confirm_count++; });
        ui_focus_pop_scope();
    };

    run_focus_frame(focus, {}, build);
    Clay_ElementData a_data = Clay_GetElementData(a);
    CHECK(a_data.found);
    Clay_Vector2 inside_a = {
        a_data.boundingBox.x + a_data.boundingBox.width * 0.5f,
        a_data.boundingBox.y + a_data.boundingBox.height * 0.5f,
    };
    Clay_Vector2 outside = {
        a_data.boundingBox.x + a_data.boundingBox.width + 140.0f,
        a_data.boundingBox.y + a_data.boundingBox.height + 140.0f,
    };

    run_focus_frame(
        focus,
        { .pointer_pressed = true, .pointer_down = true, .source = UiFocusSource::Mouse },
        build,
        { 640, 480 },
        inside_a);
    CHECK(confirm_count == 0);
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), a));
    CHECK(ui_focus_source_for_scope(scope) == UiFocusSource::Mouse);

    run_focus_frame(
        focus,
        { .pointer_down = true, .source = UiFocusSource::Mouse },
        build,
        { 640, 480 },
        outside);
    CHECK(confirm_count == 0);

    run_focus_frame(
        focus,
        { .pointer_released = true, .source = UiFocusSource::Mouse },
        build,
        { 640, 480 },
        outside);
    CHECK(confirm_count == 0);

    run_focus_frame(
        focus,
        { .pointer_pressed = true, .pointer_down = true, .source = UiFocusSource::Touch },
        build,
        { 640, 480 },
        inside_a);
    run_focus_frame(
        focus,
        { .pointer_down = true, .source = UiFocusSource::Touch },
        build,
        { 640, 480 },
        outside);
    run_focus_frame(
        focus,
        { .pointer_down = true, .source = UiFocusSource::Touch },
        build,
        { 640, 480 },
        inside_a);
    run_focus_frame(
        focus,
        { .pointer_released = true, .source = UiFocusSource::Touch },
        build,
        { 640, 480 },
        inside_a);
    CHECK(confirm_count == 1);
    CHECK(ui_focus_source_for_scope(scope) == UiFocusSource::Touch);
    return true;
}

static bool initial_focus_chooses_requested_enabled_element(void) {
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("InitialScope");
    Clay_ElementId a = test_id("InitialA");
    Clay_ElementId b = test_id("InitialB");

    run_focus_frame(focus, {}, [&] {
        ui_focus_push_scope({ .id = scope });
        ui_focus_request_initial_focus(b);
        focus_box(a);
        focus_box(b);
        ui_focus_pop_scope();
    });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), b));
    CHECK(ui_focus_source_for_scope(scope) == UiFocusSource::Programmatic);
    return true;
}

static bool modal_scope_traps_navigation_and_parent_resumes(void) {
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId parent = test_id("ParentScope");
    Clay_ElementId modal = test_id("ModalScope");
    Clay_ElementId a = test_id("ParentA");
    Clay_ElementId b = test_id("ParentB");
    Clay_ElementId m1 = test_id("ModalOne");
    Clay_ElementId m2 = test_id("ModalTwo");

    auto build = [&](bool show_modal) {
        ui_focus_push_scope({ .id = parent });
        focus_box(a);
        focus_box(b);
        ui_focus_pop_scope();

        if (show_modal) {
            ui_focus_push_scope({ .id = modal, .modal = true, .wrap = true });
            focus_box(m1);
            focus_box(m2);
            ui_focus_pop_scope();
        }
    };

    run_focus_frame(focus, {}, [&] { build(false); });
    CHECK(same_id(ui_focus_focused_id_for_scope(parent), a));

    run_focus_frame(focus, { .nav_down = true }, [&] { build(false); });
    CHECK(same_id(ui_focus_focused_id_for_scope(parent), b));

    run_focus_frame(focus, {}, [&] { build(true); });
    CHECK(same_id(ui_focus_focused_id_for_scope(modal), m1));
    CHECK(same_id(ui_focus_focused_id_for_scope(parent), b));

    run_focus_frame(focus, { .nav_down = true }, [&] { build(true); });
    CHECK(same_id(ui_focus_focused_id_for_scope(modal), m2));
    CHECK(same_id(ui_focus_focused_id_for_scope(parent), b));

    run_focus_frame(focus, {}, [&] { build(false); });
    CHECK(same_id(ui_focus_focused_id_for_scope(parent), b));

    run_focus_frame(focus, { .nav_up = true }, [&] { build(false); });
    CHECK(same_id(ui_focus_focused_id_for_scope(parent), a));
    return true;
}

static bool focus_survives_reflow_and_next_navigation_uses_new_rectangles(void) {
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("ReflowScope");
    Clay_ElementId a = test_id("ReflowA");
    Clay_ElementId b = test_id("ReflowB");

    auto build = [&](bool horizontal) {
        ui_focus_push_scope({ .id = scope });
        ui_focus_request_initial_focus(b);
        CLAY({
            .id = test_id("ReflowBody"),
            .layout = {
                .childGap = 8,
                .layoutDirection = horizontal ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM,
            },
        }) {
            focus_box(a);
            focus_box(b);
        }
        ui_focus_pop_scope();
    };

    run_focus_frame(focus, {}, [&] { build(false); });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), b));

    run_focus_frame(focus, {}, [&] { build(true); }, Clay_Dimensions{ 900, 480 });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), b));

    run_focus_frame(focus, { .nav_left = true }, [&] { build(true); }, Clay_Dimensions{ 900, 480 });
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), a));
    return true;
}

static bool focus_overflow_reports_diagnostics(void) {
    UiFocusRuntime focus;
    ui_focus_init(&focus, {
        .max_focus_scopes = 1,
        .max_focusables_per_scope = 1,
    });

    Clay_ElementId scope = test_id("OverflowScope");
    Clay_ElementId a = test_id("OverflowA");
    Clay_ElementId b = test_id("OverflowB");

    run_focus_frame(focus, {}, [&] {
        ui_focus_push_scope({ .id = scope });
        focus_box(a);
        focus_box(b);
        ui_focus_pop_scope();
    });
    CHECK(ui_focus_error_count() == 1);
    return true;
}

int main(void) {
    if (!init_clay_once()) return 1;

    if (!stacked_buttons_navigate_from_harvested_rectangles()) return 1;
    if (!grid_navigation_uses_geometry_without_neighbor_tables()) return 1;
    if (!disabled_controls_are_skipped_for_navigation_and_confirm()) return 1;
    if (!local_boundary_rules_stop_wrap_and_explicit_targets()) return 1;
    if (!focus_callbacks_use_current_frame_registration()) return 1;
    if (!frame_local_callbacks_are_released_after_dispatch()) return 1;
    if (!focus_source_tracks_mouse_and_touch_inputs()) return 1;
    if (!pointer_release_confirms_only_original_hovered_target()) return 1;
    if (!initial_focus_chooses_requested_enabled_element()) return 1;
    if (!modal_scope_traps_navigation_and_parent_resumes()) return 1;
    if (!focus_survives_reflow_and_next_navigation_uses_new_rectangles()) return 1;
    if (!focus_overflow_reports_diagnostics()) return 1;

    free(g_clay_memory);
    return 0;
}
