#include "loadout_state.h"

#include "../../../../react.h"
#include "../../callback_deps.h"
#include "../../internal/deferred_ui_mutation.h"

namespace shooter {

static ReactContext LoadoutContext = {};

LoadoutContextValue use_loadout_context_value(bool *compare_enabled,
                                              int *selected_weapon_tile,
                                              LoadoutPendingAction *pending) {
  client::ui::internal::DeferredUiMutationSink mutations =
      client::ui::internal::use_deferred_ui_mutations();

  return {
      .compare_enabled = compare_enabled,
      .selected_weapon_tile = selected_weapon_tile,
      .pending = pending,
      .set_compare_enabled = use_callback<void(bool)>(
          [compare_enabled, mutations](bool enabled) {
            if (compare_enabled && mutations) {
              mutations.submit(
                  [compare_enabled, enabled] { *compare_enabled = enabled; });
            }
          },
          client::ui::callback_deps(
              client::ui::callback_deps_ptr(compare_enabled),
              client::ui::callback_deps_ptr(mutations.owner()))),
      .set_selected_weapon_tile = use_callback<void(int)>(
          [selected_weapon_tile, mutations](int index) {
            if (selected_weapon_tile && mutations) {
              mutations.submit([selected_weapon_tile, index] {
                *selected_weapon_tile = index;
              });
            }
          },
          client::ui::callback_deps(
              client::ui::callback_deps_ptr(selected_weapon_tile),
              client::ui::callback_deps_ptr(mutations.owner()))),
      .set_pending_action = use_callback<void(LoadoutPendingAction)>(
          [pending, mutations](LoadoutPendingAction next) {
            if (pending && mutations) {
              mutations.submit([pending, next] { *pending = next; });
            }
          },
          client::ui::callback_deps(
              client::ui::callback_deps_ptr(pending),
              client::ui::callback_deps_ptr(mutations.owner()))),
      .clear_pending_action = use_callback(
          [pending, mutations] {
            if (pending && mutations) {
              mutations.submit(
                  [pending] { pending->action = LOADOUT_ACTION_NONE; });
            }
          },
          client::ui::callback_deps(
              client::ui::callback_deps_ptr(pending),
              client::ui::callback_deps_ptr(mutations.owner()))),
  };
}

void loadout_provider_push(const LoadoutContextValue *value) {
  react_provider_push(&LoadoutContext,
                      const_cast<LoadoutContextValue *>(value));
}

void loadout_provider_pop() { react_provider_pop(&LoadoutContext); }

::ui::UiElement LoadoutProvider(const LoadoutContextValue &value,
                                ::ui::UiChildren children) {
  const LoadoutContextValue *stored = ::ui::copy_value(value);
  if (!stored)
    return ::ui::empty();
  return ::ui::provider("LoadoutProvider", &LoadoutContext,
                        const_cast<LoadoutContextValue *>(stored), children);
}

static LoadoutContextValue *current_loadout_value(const char *hook_name) {
  LoadoutContextValue *value =
      static_cast<LoadoutContextValue *>(use_context(&LoadoutContext));
  if (!value) {
    react_report_error("loadout: missing LoadoutProvider for %s\n", hook_name);
  }
  return value;
}

bool use_compare_enabled() {
  LoadoutContextValue *value = current_loadout_value("use_compare_enabled");
  return (value && value->compare_enabled) ? *value->compare_enabled : false;
}

int use_selected_weapon_tile() {
  LoadoutContextValue *value =
      current_loadout_value("use_selected_weapon_tile");
  return (value && value->selected_weapon_tile) ? *value->selected_weapon_tile
                                                : 0;
}

LoadoutPendingAction use_pending_loadout_action() {
  LoadoutContextValue *value =
      current_loadout_value("use_pending_loadout_action");
  return (value && value->pending) ? *value->pending : LoadoutPendingAction{};
}

std::function<void(bool)> use_set_compare_enabled() {
  LoadoutContextValue *value = current_loadout_value("use_set_compare_enabled");
  return value ? value->set_compare_enabled : std::function<void(bool)>{};
}

std::function<void(int)> use_set_selected_weapon_tile() {
  LoadoutContextValue *value =
      current_loadout_value("use_set_selected_weapon_tile");
  return value ? value->set_selected_weapon_tile : std::function<void(int)>{};
}

std::function<void(LoadoutPendingAction)> use_set_pending_loadout_action() {
  LoadoutContextValue *value =
      current_loadout_value("use_set_pending_loadout_action");
  return value ? value->set_pending_action
               : std::function<void(LoadoutPendingAction)>{};
}

std::function<void()> use_clear_pending_loadout_action() {
  LoadoutContextValue *value =
      current_loadout_value("use_clear_pending_loadout_action");
  return value ? value->clear_pending_action : std::function<void()>{};
}

} // namespace shooter
