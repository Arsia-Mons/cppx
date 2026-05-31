#pragma once

// LoadoutDetails: the right-hand details column (a Sunken Panel) holding the
// selected-weapon summary, the compare toggle, the Buy/Equip/Back actions, the
// "Equipment Slots" heading, and the two equipment slots — in the frozen order
// the focus contract depends on (keyboard-down from the grid lands on Buy). All
// closures are built by the owning screen and passed in; this is a pure factory.

#include "ui/components/common.h"

#include <functional>

namespace shooter {

struct LoadoutDetailsProps {
  const char *key = nullptr;
  const char *summary = nullptr;
  bool can_buy = false;
  bool can_equip = false;
  bool compare_enabled = false;
  std::function<void(bool)> on_compare_change = {};
  std::function<void()> on_buy = {};
  std::function<void()> on_equip = {};
  std::function<void()> on_back = {};
};

::ui::UiElement LoadoutDetails(const LoadoutDetailsProps &props);

} // namespace shooter
