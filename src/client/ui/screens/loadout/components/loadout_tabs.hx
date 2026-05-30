#pragma once

// LoadoutTabs: the loadout tab strip (a Row Box) plus its compound item
// LoadoutTabs::Tab. The Tab adapts ui::components::Button directly so it can set
// accessibility role = Tab internally (role never leaks onto the shared
// AppButton). The owning screen builds each tab's on_select closure (where the
// 2-deferred-write contract lives); Tab only wraps on_select -> on_activate.

#include "ui/components/common.h"

#include <functional>

namespace shooter {

struct LoadoutTabs {
  struct TabProps {
    const char *key = nullptr;
    const char *control_id = nullptr;
    const char *label = nullptr;
    std::function<void()> on_select = {};
  };

  static ::ui::UiElement Tab(const TabProps &props);
};

struct LoadoutTabsProps {
  const char *key = nullptr;
  ::ui::UiChildren children = {};
};

::ui::UiElement LoadoutTabs(const LoadoutTabsProps &props);

} // namespace shooter
