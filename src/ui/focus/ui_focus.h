#pragma once

#include <array>
#include <functional>
#include <memory>
#include <stdint.h>

#include <clay.h>

#include "../input.h"
#include "../span.h"

namespace ui {

struct UiRuntimeLimits {
    int max_focus_scopes = 16;
    int max_focusables_per_scope = 256;
    int max_ui_intents = 128;
    int max_screens = 32;
};

enum class UiNavDir {
    Up,
    Down,
    Left,
    Right,
};

enum class UiNavRuleKind {
    Auto,
    Stop,
    Wrap,
    Explicit,
};

struct UiNavRule {
    UiNavRuleKind kind = UiNavRuleKind::Auto;
    Clay_ElementId explicit_target = {};
};

struct UiNavRules {
    UiNavRule up;
    UiNavRule down;
    UiNavRule left;
    UiNavRule right;
};

struct UiFocusScopeDesc {
    Clay_ElementId id = {};
    bool modal = false;
    bool wrap = false;
};

struct UiFocusableDesc {
    Clay_ElementId id = {};
    bool disabled = false;
    UiNavRules nav = {};
    std::function<void()> on_confirm = {};
    std::function<void()> on_focus = {};
};

struct UiFocusableState {
    Clay_ElementId id = {};
    bool focused = false;
    bool focus_visible = false;
    bool hovered = false;
    bool pressed = false;
    bool disabled = false;
};

constexpr int UI_FOCUS_MAX_SCOPES = 16;
constexpr int UI_FOCUS_MAX_FOCUSABLES_PER_SCOPE = 256;

struct UiFocusableLayout {
    Clay_ElementId id = {};
    Clay_BoundingBox rect = {};
    bool disabled = false;
    uint32_t order = 0;
    UiNavRules nav = {};
};

struct UiFocusableRegistration {
    Clay_ElementId id = {};
    bool disabled = false;
    UiNavRules nav = {};
    std::function<void()> on_confirm = {};
    std::function<void()> on_focus = {};
};

struct UiFocusScope {
    Clay_ElementId id = {};
    bool modal = false;
    bool wrap = false;
    Clay_ElementId focused_id = {};
    Clay_ElementId pointer_press_origin = {};
    UiFocusSource source = UiFocusSource::None;
    Clay_ElementId requested_initial_focus = {};
    uint32_t declared_frame = 0;
    uint32_t declaration_order = 0;

    UiFocusableRegistration *pending = nullptr;
    int pending_count = 0;

    UiFocusableLayout *layout = nullptr;
    int layout_count = 0;
};

struct UiFocusRuntime {
    UiRuntimeLimits limits = {};
    std::array<UiFocusScope, UI_FOCUS_MAX_SCOPES> scopes = {};
    int scope_count = 0;

    std::unique_ptr<UiFocusableRegistration[]> pending_storage = {};
    std::unique_ptr<UiFocusableLayout[]> layout_storage = {};

    int scope_stack[UI_FOCUS_MAX_SCOPES] = {};
    int scope_stack_count = 0;

    uint32_t frame = 0;
    uint32_t next_declaration_order = 0;
    int error_count = 0;

    Clay_ElementId pending_focus_callback_id = {};
    Clay_ElementId pending_pointer_confirm_id = {};
    bool pointer_down = false;
};

void ui_focus_init(UiFocusRuntime *runtime, UiRuntimeLimits limits = {});
void ui_focus_set_current(UiFocusRuntime *runtime);
UiFocusRuntime *ui_focus_current(void);

void ui_focus_begin_frame(const UiInputFrame &input);
void ui_focus_end_layout(const UiInputFrame &input = {});

void ui_focus_push_scope(const UiFocusScopeDesc &desc);
void ui_focus_pop_scope(void);
void ui_focus_request_initial_focus(Clay_ElementId id);

UiFocusableState ui_focusable(const UiFocusableDesc &desc);

Clay_ElementId ui_focus_focused_id(void);
Clay_ElementId ui_focus_focused_id_for_scope(Clay_ElementId scope_id);
UiFocusSource ui_focus_source(void);
UiFocusSource ui_focus_source_for_scope(Clay_ElementId scope_id);
int ui_focus_error_count(void);

} // namespace ui
