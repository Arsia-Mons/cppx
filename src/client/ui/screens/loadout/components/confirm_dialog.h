#pragma once

#include "../../../../../ui/runtime/element.h"

namespace shooter {

struct LoadoutConfirmDialogProps {
  uint32_t unused = 0;
};

// Reads the pending action from `LoadoutContext` and renders the confirm
// modal. Returns early when no action is pending. Action constants and the
// pending struct live in `loadout_state.h`.
::ui::UiElement LoadoutConfirmDialog(const LoadoutConfirmDialogProps &props);

} // namespace shooter
