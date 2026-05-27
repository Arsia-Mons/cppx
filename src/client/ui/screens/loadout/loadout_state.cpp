#include "loadout_state.h"

#include "../../../../react.h"

namespace shooter {

static ReactContext LoadoutContext = {};

void loadout_provider_push(const LoadoutContextValue *value) {
    react_provider_push(&LoadoutContext,
                        const_cast<LoadoutContextValue *>(value));
}

void loadout_provider_pop() {
    react_provider_pop(&LoadoutContext);
}

static LoadoutContextValue *current_loadout_value() {
    return static_cast<LoadoutContextValue *>(use_context(&LoadoutContext));
}

bool use_compare_enabled() {
    LoadoutContextValue *value = current_loadout_value();
    return (value && value->compare_enabled) ? *value->compare_enabled : false;
}

int use_selected_weapon_tile() {
    LoadoutContextValue *value = current_loadout_value();
    return (value && value->selected_weapon_tile) ? *value->selected_weapon_tile : 0;
}

LoadoutPendingAction use_pending_loadout_action() {
    LoadoutContextValue *value = current_loadout_value();
    return (value && value->pending) ? *value->pending : LoadoutPendingAction{};
}

std::function<void(bool)> use_set_compare_enabled() {
    LoadoutContextValue *value = current_loadout_value();
    return value ? value->set_compare_enabled : std::function<void(bool)>{};
}

std::function<void(int)> use_set_selected_weapon_tile() {
    LoadoutContextValue *value = current_loadout_value();
    return value ? value->set_selected_weapon_tile : std::function<void(int)>{};
}

std::function<void(LoadoutPendingAction)> use_set_pending_loadout_action() {
    LoadoutContextValue *value = current_loadout_value();
    return value ? value->set_pending_action : std::function<void(LoadoutPendingAction)>{};
}

std::function<void()> use_clear_pending_loadout_action() {
    LoadoutContextValue *value = current_loadout_value();
    return value ? value->clear_pending_action : std::function<void()>{};
}

} // namespace shooter
