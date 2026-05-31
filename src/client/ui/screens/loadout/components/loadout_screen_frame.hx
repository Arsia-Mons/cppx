#pragma once

// LoadoutScreenFrame: the loadout root surface. Adapts ui::components::Dialog
// with modal = !confirm_open so the dialog yields modality (and records
// previous_focus_before_modal) when the confirm modal opens. Loadout-owned
// because its modal flag is dynamic, unlike the shared ScreenLayout frames.

#include "ui/components/common.h"

namespace shooter {

struct LoadoutScreenFrameProps {
  const char *key = nullptr;
  bool confirm_open = false;
  ::ui::UiChildren children = {};
};

::ui::UiElement LoadoutScreenFrame(const LoadoutScreenFrameProps &props);

} // namespace shooter
