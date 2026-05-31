#include "loadout_provider.h"

#include "client/ui/app_shell/deferred_ui_mutation.h"
#include "client/ui/callback_deps.h"
#include "client/ui/screens/loadout/hooks/use_loadout.h"
#include "react.h"

namespace shooter {

namespace {

struct LoadoutContextValue {
  bool *compare_enabled = nullptr;
  int *selected_weapon_tile = nullptr;
  LoadoutValue::PendingAction *pending = nullptr;
  std::function<void(bool)> set_compare_enabled = {};
  std::function<void(int)> set_selected_weapon_tile = {};
  std::function<void(LoadoutValue::PendingAction)> set_pending_action = {};
  std::function<void()> clear_pending_action = {};
};

ReactContext LoadoutContext = {};

LoadoutContextValue make_loadout_context_value(bool *compare_enabled,
                                               int *selected_weapon_tile,
                                               LoadoutValue::PendingAction *pending) {
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
      .set_pending_action = use_callback<void(LoadoutValue::PendingAction)>(
          [pending, mutations](LoadoutValue::PendingAction next) {
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
                  [pending] { pending->action = LoadoutValue::Action::None; });
            }
          },
          client::ui::callback_deps(
              client::ui::callback_deps_ptr(pending),
              client::ui::callback_deps_ptr(mutations.owner()))),
  };
}

LoadoutContextValue *current_loadout_value(const char *hook_name) {
  LoadoutContextValue *value =
      static_cast<LoadoutContextValue *>(use_context(&LoadoutContext));
  if (!value) {
    react_report_error("loadout: missing LoadoutProvider for %s\n", hook_name);
  }
  return value;
}

} // namespace

::ui::UiElement LoadoutProvider(const LoadoutProviderProps &props) {
  bool *compare_enabled = use_state<bool>(false);
  int *selected_index = use_state<int>(0);
  LoadoutValue::PendingAction *pending =
      use_state<LoadoutValue::PendingAction>({});
  if (!compare_enabled || !selected_index || !pending)
    return ::ui::empty();

  LoadoutContextValue value =
      make_loadout_context_value(compare_enabled, selected_index, pending);
  const LoadoutContextValue *stored = ::ui::copy_value(value);
  if (!stored)
    return ::ui::empty();

  return ::ui::provider("LoadoutProvider", &LoadoutContext,
                        const_cast<LoadoutContextValue *>(stored),
                        props.children, props.key);
}

LoadoutValue use_loadout() {
  LoadoutContextValue *value = current_loadout_value("use_loadout");
  if (!value)
    return {};

  return {
      .compare_enabled =
          value->compare_enabled ? *value->compare_enabled : false,
      .selected_weapon_tile =
          value->selected_weapon_tile ? *value->selected_weapon_tile : 0,
      .pending =
          value->pending ? *value->pending : LoadoutValue::PendingAction{},
      .set_compare_enabled = value->set_compare_enabled,
      .set_selected_weapon_tile = value->set_selected_weapon_tile,
      .set_pending_action = value->set_pending_action,
      .clear_pending_action = value->clear_pending_action,
  };
}

} // namespace shooter
