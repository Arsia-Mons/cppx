#include "react.h"
#include "ui/focus/ui_focus.h"
#include "ui/primitives/button.h"
#include "ui/primitives/focusable.h"
#include "ui/primitives/selectable.h"
#include "ui/primitives/toggle.h"
#include "ui/primitives/visual_state.h"

#include <clay.h>

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

template <typename Build>
static void run_primitive_frame(UiFocusRuntime &focus,
                                const UiInputFrame &input,
                                Build build,
                                Clay_Vector2 pointer = { -1000.0f, -1000.0f }) {
    ui_focus_set_current(&focus);
    Clay_SetLayoutDimensions({ 640, 480 });
    Clay_SetPointerState(pointer, input.pointer_down);
    ui_focus_begin_frame(input);
    react_begin_frame();
    Clay_BeginLayout();
    CLAY({
        .id = test_id("PrimitiveRoot"),
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childGap = 8,
        },
    }) {
        build();
    }
    (void)Clay_EndLayout();
    ui_focus_end_layout(input);
    react_end_frame();
}

static bool same_id(Clay_ElementId a, Clay_ElementId b) {
    return a.id != 0 && a.id == b.id;
}

static bool visual_state_is_derived_from_focus_and_control_state(void) {
    UiFocusableState focus = {
        .focused = true,
        .focus_visible = true,
        .hovered = false,
        .pressed = true,
    };
    VisualState visual = derive_visual_state(focus, {
        .checked = true,
    });

    CHECK(visual.targeted);
    CHECK(visual.active);
    CHECK(visual.chosen);
    CHECK(!visual.unavailable);

    visual = derive_visual_state({ .hovered = true }, {
        .disabled = true,
    });
    CHECK(visual.targeted);
    CHECK(!visual.active);
    CHECK(!visual.chosen);
    CHECK(visual.unavailable);
    return true;
}

static bool button_confirms_once_for_keyboard_and_gamepad_edges(void) {
    react_init(g_clay);
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("ButtonScope");
    Clay_ElementId button = test_id("ConfirmButton");
    int confirm_count = 0;

    auto build = [&] {
        ui_focus_push_scope({ .id = scope });
        ui_focus_request_initial_focus(button);
        Button({
            .id = button,
            .label = "Confirm",
            .on_confirm = [&] { confirm_count++; },
        });
        ui_focus_pop_scope();
    };

    run_primitive_frame(focus, {}, build);
    CHECK(same_id(ui_focus_focused_id_for_scope(scope), button));

    run_primitive_frame(focus, { .confirm_pressed = true, .source = UiFocusSource::Keyboard }, build);
    CHECK(confirm_count == 1);

    run_primitive_frame(focus, { .confirm_down = true, .source = UiFocusSource::Keyboard }, build);
    CHECK(confirm_count == 1);

    run_primitive_frame(focus, { .confirm_pressed = true, .source = UiFocusSource::Gamepad }, build);
    CHECK(confirm_count == 2);
    return true;
}

static bool pointer_press_drag_and_release_confirm_button_once(void) {
    react_init(g_clay);
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("PointerButtonScope");
    Clay_ElementId button = test_id("PointerButton");
    int confirm_count = 0;

    auto build = [&] {
        ui_focus_push_scope({ .id = scope });
        Button({
            .id = button,
            .label = "Pointer",
            .on_confirm = [&] { confirm_count++; },
        });
        ui_focus_pop_scope();
    };

    run_primitive_frame(focus, {}, build);
    Clay_ElementData data = Clay_GetElementData(button);
    CHECK(data.found);
    Clay_Vector2 inside = {
        data.boundingBox.x + data.boundingBox.width * 0.5f,
        data.boundingBox.y + data.boundingBox.height * 0.5f,
    };
    Clay_Vector2 outside = {
        data.boundingBox.x + data.boundingBox.width + 80.0f,
        data.boundingBox.y + data.boundingBox.height + 80.0f,
    };

    run_primitive_frame(
        focus,
        { .pointer_pressed = true, .pointer_down = true, .source = UiFocusSource::Mouse },
        build,
        outside);
    run_primitive_frame(
        focus,
        { .pointer_released = true, .source = UiFocusSource::Mouse },
        build,
        outside);
    CHECK(confirm_count == 0);

    run_primitive_frame(
        focus,
        { .pointer_pressed = true, .pointer_down = true, .source = UiFocusSource::Mouse },
        build,
        inside);
    run_primitive_frame(
        focus,
        { .pointer_down = true, .source = UiFocusSource::Mouse },
        build,
        outside);
    run_primitive_frame(
        focus,
        { .pointer_released = true, .source = UiFocusSource::Mouse },
        build,
        outside);
    CHECK(confirm_count == 0);

    run_primitive_frame(
        focus,
        { .pointer_pressed = true, .pointer_down = true, .source = UiFocusSource::Mouse },
        build,
        inside);
    run_primitive_frame(
        focus,
        { .pointer_down = true, .source = UiFocusSource::Mouse },
        build,
        outside);
    run_primitive_frame(
        focus,
        { .pointer_down = true, .source = UiFocusSource::Mouse },
        build,
        inside);
    run_primitive_frame(
        focus,
        { .pointer_released = true, .source = UiFocusSource::Mouse },
        build,
        inside);
    CHECK(confirm_count == 1);
    return true;
}

static bool toggle_calls_caller_owned_setter(void) {
    react_init(g_clay);
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("ToggleScope");
    Clay_ElementId toggle = test_id("ToggleFullscreen");
    bool checked = false;

    auto build = [&] {
        ui_focus_push_scope({ .id = scope });
        ui_focus_request_initial_focus(toggle);
        Toggle({
            .id = toggle,
            .label = "Fullscreen",
            .checked = checked,
            .on_change = [&](bool next) { checked = next; },
        });
        ui_focus_pop_scope();
    };

    run_primitive_frame(focus, {}, build);
    run_primitive_frame(focus, { .confirm_pressed = true }, build);
    CHECK(checked);

    run_primitive_frame(focus, { .confirm_pressed = true }, build);
    CHECK(!checked);
    return true;
}

static bool selectable_uses_caller_owned_selection(void) {
    react_init(g_clay);
    UiFocusRuntime focus;
    ui_focus_init(&focus);

    Clay_ElementId scope = test_id("SelectableScope");
    Clay_ElementId selectable = test_id("SelectableRow");
    bool selected = false;

    auto build = [&] {
        ui_focus_push_scope({ .id = scope });
        ui_focus_request_initial_focus(selectable);
        Selectable({
            .id = selectable,
            .label = "Rifle",
            .selected = selected,
            .on_select = [&] { selected = true; },
        });
        ui_focus_pop_scope();
    };

    run_primitive_frame(focus, {}, build);
    run_primitive_frame(focus, { .confirm_pressed = true }, build);
    CHECK(selected);
    return true;
}

int main(void) {
    if (!init_clay_once()) return 1;

    if (!visual_state_is_derived_from_focus_and_control_state()) return 1;
    if (!button_confirms_once_for_keyboard_and_gamepad_edges()) return 1;
    if (!pointer_press_drag_and_release_confirm_button_once()) return 1;
    if (!toggle_calls_caller_owned_setter()) return 1;
    if (!selectable_uses_caller_owned_selection()) return 1;

    react_shutdown();
    free(g_clay_memory);
    return 0;
}
