#pragma once

#include "../../../../../ui/runtime/element.h"

namespace shooter {

struct LoadoutConfirmDialogProps {
  uint32_t unused = 0;
};

// Reads the pending action from the loadout state provider and renders the
// confirm modal. Returns early when no action is pending.
::ui::UiElement LoadoutConfirmDialog(const LoadoutConfirmDialogProps &props);

} // namespace shooter
