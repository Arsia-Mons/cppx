#include "ui_focus.h"

#include <algorithm>
#include <cmath>
#include <float.h>
#include <stdio.h>

namespace ui {

static UiFocusRuntime *g_current = nullptr;

static bool same_id(Clay_ElementId a, Clay_ElementId b) {
    return a.id != 0 && a.id == b.id;
}

static void report_error(UiFocusRuntime *runtime, const char *message) {
    if (!runtime) return;
    runtime->error_count++;
    fprintf(stderr, "ui_focus: %s\n", message);
}

static int clamp_limit(int value, int fallback, int max_value) {
    if (value <= 0) return fallback;
    if (value > max_value) return max_value;
    return value;
}

void ui_focus_init(UiFocusRuntime *runtime, UiRuntimeLimits limits) {
    if (!runtime) return;
    *runtime = {};
    runtime->pending_storage = std::make_unique<UiFocusableRegistration[]>(
        UI_FOCUS_MAX_SCOPES * UI_FOCUS_MAX_FOCUSABLES_PER_SCOPE);
    runtime->layout_storage = std::make_unique<UiFocusableLayout[]>(
        UI_FOCUS_MAX_SCOPES * UI_FOCUS_MAX_FOCUSABLES_PER_SCOPE);
    runtime->limits.max_focus_scopes = clamp_limit(
        limits.max_focus_scopes, UI_FOCUS_MAX_SCOPES, UI_FOCUS_MAX_SCOPES);
    runtime->limits.max_focusables_per_scope = clamp_limit(
        limits.max_focusables_per_scope,
        UI_FOCUS_MAX_FOCUSABLES_PER_SCOPE,
        UI_FOCUS_MAX_FOCUSABLES_PER_SCOPE);
    runtime->limits.max_ui_intents = limits.max_ui_intents > 0
        ? limits.max_ui_intents
        : 128;
    runtime->limits.max_screens = limits.max_screens > 0
        ? limits.max_screens
        : 32;
    ui_focus_set_current(runtime);
}

void ui_focus_set_current(UiFocusRuntime *runtime) {
    g_current = runtime;
}

UiFocusRuntime *ui_focus_current(void) {
    return g_current;
}

static UiFocusScope *find_scope(UiFocusRuntime *runtime, Clay_ElementId id) {
    if (!runtime || id.id == 0) return nullptr;
    for (int i = 0; i < runtime->scope_count; ++i) {
        if (same_id(runtime->scopes[i].id, id)) return &runtime->scopes[i];
    }
    return nullptr;
}

static int find_scope_index(UiFocusRuntime *runtime, Clay_ElementId id) {
    if (!runtime || id.id == 0) return -1;
    for (int i = 0; i < runtime->scope_count; ++i) {
        if (same_id(runtime->scopes[i].id, id)) return i;
    }
    return -1;
}

static UiFocusScope *create_scope(UiFocusRuntime *runtime, const UiFocusScopeDesc &desc) {
    if (!runtime || desc.id.id == 0) return nullptr;
    if (runtime->scope_count >= runtime->limits.max_focus_scopes) {
        report_error(runtime, "scope overflow");
        return nullptr;
    }
    int scope_index = runtime->scope_count++;
    UiFocusScope *scope = &runtime->scopes[scope_index];
    *scope = {};
    scope->pending = runtime->pending_storage.get() +
        scope_index * UI_FOCUS_MAX_FOCUSABLES_PER_SCOPE;
    scope->layout = runtime->layout_storage.get() +
        scope_index * UI_FOCUS_MAX_FOCUSABLES_PER_SCOPE;
    scope->id = desc.id;
    scope->modal = desc.modal;
    scope->wrap = desc.wrap;
    return scope;
}

static void clear_pending_registrations(UiFocusScope *scope) {
    if (!scope) return;
    for (int i = 0; i < scope->pending_count; ++i) {
        scope->pending[i] = {};
    }
    scope->pending_count = 0;
}

static UiFocusScope *scope_for_declaration(UiFocusRuntime *runtime, const UiFocusScopeDesc &desc) {
    UiFocusScope *scope = find_scope(runtime, desc.id);
    if (!scope) scope = create_scope(runtime, desc);
    if (!scope) return nullptr;

    scope->modal = desc.modal;
    scope->wrap = desc.wrap;
    if (scope->declared_frame != runtime->frame) {
        scope->declared_frame = runtime->frame;
        scope->declaration_order = runtime->next_declaration_order++;
        clear_pending_registrations(scope);
    }
    return scope;
}

static UiFocusScope *active_scope_for_declaration(UiFocusRuntime *runtime) {
    if (!runtime || runtime->scope_stack_count <= 0) return nullptr;
    int index = runtime->scope_stack[runtime->scope_stack_count - 1];
    if (index < 0 || index >= runtime->scope_count) return nullptr;
    return &runtime->scopes[index];
}

static UiFocusScope *active_declared_scope(UiFocusRuntime *runtime, uint32_t frame) {
    if (!runtime) return nullptr;
    UiFocusScope *best_modal = nullptr;
    UiFocusScope *best_any = nullptr;
    for (int i = 0; i < runtime->scope_count; ++i) {
        UiFocusScope *scope = &runtime->scopes[i];
        if (scope->declared_frame != frame) continue;
        if (!best_any || scope->declaration_order > best_any->declaration_order) {
            best_any = scope;
        }
        if (scope->modal &&
            (!best_modal || scope->declaration_order > best_modal->declaration_order)) {
            best_modal = scope;
        }
    }
    return best_modal ? best_modal : best_any;
}

static UiNavRule rule_for_dir(const UiNavRules &rules, UiNavDir dir) {
    switch (dir) {
        case UiNavDir::Up: return rules.up;
        case UiNavDir::Down: return rules.down;
        case UiNavDir::Left: return rules.left;
        case UiNavDir::Right: return rules.right;
    }
    return {};
}

static const UiFocusableLayout *find_layout(const UiFocusScope *scope, Clay_ElementId id) {
    if (!scope || id.id == 0) return nullptr;
    for (int i = 0; i < scope->layout_count; ++i) {
        if (same_id(scope->layout[i].id, id)) return &scope->layout[i];
    }
    return nullptr;
}

static bool pointer_over(Clay_ElementId id) {
    return id.id != 0 && Clay_PointerOver(id);
}

static bool contains_enabled(const UiFocusScope *scope, Clay_ElementId id) {
    const UiFocusableLayout *layout = find_layout(scope, id);
    return layout && !layout->disabled;
}

static Clay_ElementId first_enabled(const UiFocusScope *scope) {
    if (!scope) return {};
    for (int i = 0; i < scope->layout_count; ++i) {
        if (!scope->layout[i].disabled) return scope->layout[i].id;
    }
    return {};
}

static Clay_ElementId hovered_enabled(const UiFocusScope *scope) {
    if (!scope) return {};
    for (int i = scope->layout_count - 1; i >= 0; --i) {
        const UiFocusableLayout &layout = scope->layout[i];
        if (!layout.disabled && pointer_over(layout.id)) {
            return layout.id;
        }
    }
    return {};
}

static const UiFocusableRegistration *find_registration(const UiFocusScope *scope,
                                                        Clay_ElementId id) {
    if (!scope || id.id == 0) return nullptr;
    for (int i = 0; i < scope->pending_count; ++i) {
        if (same_id(scope->pending[i].id, id)) return &scope->pending[i];
    }
    return nullptr;
}

static float center_x(Clay_BoundingBox r) {
    return r.x + r.width * 0.5f;
}

static float center_y(Clay_BoundingBox r) {
    return r.y + r.height * 0.5f;
}

static bool interval_overlaps(float a0, float a1, float b0, float b1) {
    return a0 < b1 && b0 < a1;
}

static bool is_candidate_in_direction(Clay_BoundingBox from,
                                      Clay_BoundingBox to,
                                      UiNavDir dir) {
    switch (dir) {
        case UiNavDir::Left: return center_x(to) < center_x(from);
        case UiNavDir::Right: return center_x(to) > center_x(from);
        case UiNavDir::Up: return center_y(to) < center_y(from);
        case UiNavDir::Down: return center_y(to) > center_y(from);
    }
    return false;
}

static float primary_distance(Clay_BoundingBox from,
                              Clay_BoundingBox to,
                              UiNavDir dir) {
    switch (dir) {
        case UiNavDir::Left: return from.x - (to.x + to.width);
        case UiNavDir::Right: return to.x - (from.x + from.width);
        case UiNavDir::Up: return from.y - (to.y + to.height);
        case UiNavDir::Down: return to.y - (from.y + from.height);
    }
    return 0.0f;
}

static float perpendicular_miss(Clay_BoundingBox from,
                                Clay_BoundingBox to,
                                UiNavDir dir) {
    bool horizontal = dir == UiNavDir::Left || dir == UiNavDir::Right;
    if (horizontal) {
        if (interval_overlaps(from.y, from.y + from.height, to.y, to.y + to.height)) {
            return 0.0f;
        }
        return std::fabs(center_y(to) - center_y(from));
    }
    if (interval_overlaps(from.x, from.x + from.width, to.x, to.x + to.width)) {
        return 0.0f;
    }
    return std::fabs(center_x(to) - center_x(from));
}

static Clay_ElementId resolve_wrap(const UiFocusScope *scope, UiNavDir dir) {
    if (!scope) return {};
    const UiFocusableLayout *best = nullptr;
    for (int i = 0; i < scope->layout_count; ++i) {
        const UiFocusableLayout *candidate = &scope->layout[i];
        if (candidate->disabled) continue;
        if (!best) {
            best = candidate;
            continue;
        }
        switch (dir) {
            case UiNavDir::Left:
                if (center_x(candidate->rect) > center_x(best->rect)) best = candidate;
                break;
            case UiNavDir::Right:
                if (center_x(candidate->rect) < center_x(best->rect)) best = candidate;
                break;
            case UiNavDir::Up:
                if (center_y(candidate->rect) > center_y(best->rect)) best = candidate;
                break;
            case UiNavDir::Down:
                if (center_y(candidate->rect) < center_y(best->rect)) best = candidate;
                break;
        }
    }
    return best ? best->id : Clay_ElementId{};
}

static Clay_ElementId resolve_spatial(const UiFocusScope *scope,
                                      Clay_ElementId from_id,
                                      UiNavDir dir,
                                      bool force_wrap) {
    if (!scope) return {};
    const UiFocusableLayout *from = find_layout(scope, from_id);
    if (!from || from->disabled) return first_enabled(scope);

    const UiFocusableLayout *best = nullptr;
    float best_perp = 0.0f;
    float best_primary = 0.0f;
    float best_center = 0.0f;

    for (int i = 0; i < scope->layout_count; ++i) {
        const UiFocusableLayout *candidate = &scope->layout[i];
        if (same_id(candidate->id, from_id) || candidate->disabled) continue;
        if (!is_candidate_in_direction(from->rect, candidate->rect, dir)) continue;

        float primary = primary_distance(from->rect, candidate->rect, dir);
        if (primary < 0.0f) primary = 0.0f;

        float perp = perpendicular_miss(from->rect, candidate->rect, dir);
        float dx = center_x(candidate->rect) - center_x(from->rect);
        float dy = center_y(candidate->rect) - center_y(from->rect);
        float center = dx * dx + dy * dy;

        bool better =
            !best ||
            perp < best_perp ||
            (perp == best_perp && primary < best_primary) ||
            (perp == best_perp && primary == best_primary && center < best_center) ||
            (perp == best_perp && primary == best_primary &&
             center == best_center && candidate->order < best->order);

        if (better) {
            best = candidate;
            best_perp = perp;
            best_primary = primary;
            best_center = center;
        }
    }

    if (best) return best->id;
    return (scope->wrap || force_wrap) ? resolve_wrap(scope, dir) : from_id;
}

static Clay_ElementId resolve_navigation(const UiFocusScope *scope,
                                         Clay_ElementId from_id,
                                         UiNavDir dir) {
    if (!scope) return {};
    const UiFocusableLayout *from = find_layout(scope, from_id);
    UiNavRule rule = from ? rule_for_dir(from->nav, dir) : UiNavRule{};
    switch (rule.kind) {
        case UiNavRuleKind::Stop:
            return from_id;
        case UiNavRuleKind::Explicit:
            return contains_enabled(scope, rule.explicit_target)
                ? rule.explicit_target
                : from_id;
        case UiNavRuleKind::Wrap:
            return resolve_spatial(scope, from_id, dir, true);
        case UiNavRuleKind::Auto:
            return resolve_spatial(scope, from_id, dir, false);
    }
    return from_id;
}

static bool read_nav_dir(const UiInputFrame &input, UiNavDir *dir) {
    if (input.nav_up) {
        *dir = UiNavDir::Up;
        return true;
    }
    if (input.nav_down) {
        *dir = UiNavDir::Down;
        return true;
    }
    if (input.nav_left) {
        *dir = UiNavDir::Left;
        return true;
    }
    if (input.nav_right) {
        *dir = UiNavDir::Right;
        return true;
    }
    return false;
}

static UiFocusSource navigation_source(const UiInputFrame &input) {
    if (input.source == UiFocusSource::Gamepad) return UiFocusSource::Gamepad;
    if (input.source == UiFocusSource::Touch) return UiFocusSource::Touch;
    if (input.source == UiFocusSource::Mouse) return UiFocusSource::Mouse;
    return UiFocusSource::Keyboard;
}

static UiFocusSource pointer_source(const UiInputFrame &input) {
    return input.source == UiFocusSource::Touch
        ? UiFocusSource::Touch
        : UiFocusSource::Mouse;
}

void ui_focus_begin_frame(const UiInputFrame &input) {
    UiFocusRuntime *runtime = g_current;
    if (!runtime) return;

    runtime->frame++;
    runtime->scope_stack_count = 0;
    runtime->next_declaration_order = 0;
    runtime->pending_focus_callback_id = {};
    runtime->pending_pointer_confirm_id = {};
    runtime->pointer_down = input.pointer_down;

    UiFocusScope *scope = active_declared_scope(runtime, runtime->frame - 1);
    UiNavDir dir;
    if (!scope) return;

    if (read_nav_dir(input, &dir)) {
        Clay_ElementId next = resolve_navigation(scope, scope->focused_id, dir);
        if (next.id != 0 && !same_id(next, scope->focused_id)) {
            scope->focused_id = next;
            scope->source = navigation_source(input);
            runtime->pending_focus_callback_id = next;
        }
    }

    if (input.pointer_pressed) {
        Clay_ElementId hovered = hovered_enabled(scope);
        scope->pointer_press_origin = hovered;
        if (hovered.id != 0) {
            if (!same_id(scope->focused_id, hovered)) {
                runtime->pending_focus_callback_id = hovered;
            }
            scope->focused_id = hovered;
            scope->source = pointer_source(input);
        }
    }

    if (input.pointer_released) {
        Clay_ElementId hovered = hovered_enabled(scope);
        if (same_id(scope->pointer_press_origin, hovered)) {
            runtime->pending_pointer_confirm_id = hovered;
        }
        scope->pointer_press_origin = {};
    } else if (!input.pointer_down) {
        scope->pointer_press_origin = {};
    }
}

void ui_focus_push_scope(const UiFocusScopeDesc &desc) {
    UiFocusRuntime *runtime = g_current;
    if (!runtime) return;
    if (runtime->scope_stack_count >= runtime->limits.max_focus_scopes) {
        report_error(runtime, "scope stack overflow");
        return;
    }

    UiFocusScope *scope = scope_for_declaration(runtime, desc);
    if (!scope) return;
    int index = find_scope_index(runtime, scope->id);
    if (index < 0) return;
    runtime->scope_stack[runtime->scope_stack_count++] = index;
}

void ui_focus_pop_scope(void) {
    UiFocusRuntime *runtime = g_current;
    if (!runtime) return;
    if (runtime->scope_stack_count <= 0) {
        report_error(runtime, "scope pop without matching push");
        return;
    }
    runtime->scope_stack_count--;
}

void ui_focus_request_initial_focus(Clay_ElementId id) {
    UiFocusScope *scope = active_scope_for_declaration(g_current);
    if (!scope) return;
    scope->requested_initial_focus = id;
}

UiFocusableState ui_focusable(const UiFocusableDesc &desc) {
    UiFocusRuntime *runtime = g_current;
    UiFocusScope *scope = active_scope_for_declaration(runtime);
    UiFocusableState state = {};
    state.id = desc.id;
    state.disabled = desc.disabled;
    state.hovered = pointer_over(desc.id);

    if (!runtime || !scope || desc.id.id == 0) return state;
    if (scope->pending_count >= runtime->limits.max_focusables_per_scope) {
        report_error(runtime, "focusable overflow");
        return state;
    }

    scope->pending[scope->pending_count++] = {
        .id = desc.id,
        .disabled = desc.disabled,
        .nav = desc.nav,
        .on_confirm = desc.on_confirm,
        .on_focus = desc.on_focus,
    };

    state.focused = same_id(scope->focused_id, desc.id);
    state.focus_visible = state.focused &&
        (scope->source == UiFocusSource::Keyboard ||
         scope->source == UiFocusSource::Gamepad ||
         scope->source == UiFocusSource::Programmatic);
    state.pressed = runtime->pointer_down &&
        !desc.disabled &&
        state.hovered &&
        same_id(scope->pointer_press_origin, desc.id);
    return state;
}

static void harvest_scope_layout(UiFocusRuntime *runtime, UiFocusScope *scope) {
    scope->layout_count = 0;
    uint32_t order = 0;
    for (int i = 0; i < scope->pending_count; ++i) {
        const UiFocusableRegistration &entry = scope->pending[i];
        Clay_ElementData data = Clay_GetElementData(entry.id);
        if (!data.found) continue;
        if (scope->layout_count >= runtime->limits.max_focusables_per_scope) {
            report_error(runtime, "focus layout overflow");
            break;
        }
        scope->layout[scope->layout_count++] = {
            .id = entry.id,
            .rect = data.boundingBox,
            .disabled = entry.disabled,
            .order = order++,
            .nav = entry.nav,
        };
    }
}

static void invoke_focus_callback_if_registered(UiFocusScope *scope, Clay_ElementId id) {
    const UiFocusableRegistration *registration = find_registration(scope, id);
    if (registration && registration->on_focus) {
        registration->on_focus();
    }
}

void ui_focus_end_layout(const UiInputFrame &input) {
    UiFocusRuntime *runtime = g_current;
    if (!runtime) return;

    UiFocusScope *active_before_harvest = active_declared_scope(runtime, runtime->frame);
    Clay_ElementId confirm_target = active_before_harvest
        ? active_before_harvest->focused_id
        : Clay_ElementId{};

    for (int i = 0; i < runtime->scope_count; ++i) {
        UiFocusScope *scope = &runtime->scopes[i];
        if (scope->declared_frame != runtime->frame) continue;

        Clay_ElementId previous_focus = scope->focused_id;
        harvest_scope_layout(runtime, scope);

        if (!contains_enabled(scope, scope->focused_id)) {
            Clay_ElementId next = first_enabled(scope);
            if (contains_enabled(scope, scope->requested_initial_focus)) {
                next = scope->requested_initial_focus;
            }
            scope->focused_id = next;
            scope->source = next.id != 0 ? UiFocusSource::Programmatic : UiFocusSource::None;
        }

        if (!same_id(previous_focus, scope->focused_id) && scope->focused_id.id != 0) {
            invoke_focus_callback_if_registered(scope, scope->focused_id);
        } else if (same_id(runtime->pending_focus_callback_id, scope->focused_id)) {
            invoke_focus_callback_if_registered(scope, scope->focused_id);
        }

        scope->requested_initial_focus = {};
    }

    UiFocusScope *active = active_declared_scope(runtime, runtime->frame);
    bool confirm_dispatched = false;
    if (active && input.confirm_pressed && contains_enabled(active, confirm_target)) {
        const UiFocusableRegistration *registration =
            find_registration(active, confirm_target);
        if (registration && !registration->disabled && registration->on_confirm) {
            registration->on_confirm();
            confirm_dispatched = true;
        }
    }
    if (active &&
        runtime->pending_pointer_confirm_id.id != 0 &&
        (!confirm_dispatched || !same_id(runtime->pending_pointer_confirm_id, confirm_target)) &&
        contains_enabled(active, runtime->pending_pointer_confirm_id)) {
        const UiFocusableRegistration *registration =
            find_registration(active, runtime->pending_pointer_confirm_id);
        if (registration && !registration->disabled && registration->on_confirm) {
            registration->on_confirm();
        }
    }

    for (int i = 0; i < runtime->scope_count; ++i) {
        UiFocusScope *scope = &runtime->scopes[i];
        if (scope->declared_frame != runtime->frame) continue;
        clear_pending_registrations(scope);
    }
}

Clay_ElementId ui_focus_focused_id_for_scope(Clay_ElementId scope_id) {
    UiFocusScope *scope = find_scope(g_current, scope_id);
    return scope ? scope->focused_id : Clay_ElementId{};
}

Clay_ElementId ui_focus_focused_id(void) {
    UiFocusRuntime *runtime = g_current;
    UiFocusScope *scope = active_declared_scope(runtime, runtime ? runtime->frame : 0);
    return scope ? scope->focused_id : Clay_ElementId{};
}

UiFocusSource ui_focus_source(void) {
    UiFocusRuntime *runtime = g_current;
    UiFocusScope *scope = active_declared_scope(runtime, runtime ? runtime->frame : 0);
    return scope ? scope->source : UiFocusSource::None;
}

UiFocusSource ui_focus_source_for_scope(Clay_ElementId scope_id) {
    UiFocusScope *scope = find_scope(g_current, scope_id);
    return scope ? scope->source : UiFocusSource::None;
}

int ui_focus_error_count(void) {
    return g_current ? g_current->error_count : 0;
}

} // namespace ui
