#pragma once

#include "../../../../../ui/runtime/element.h"

namespace shooter {

// Reads the pending action from `LoadoutContext` and renders the confirm
// modal. Returns early when no action is pending. Action constants and the
// pending struct live in `loadout_state.h`.
::ui::retained::UiElement
LoadoutConfirmDialog(::ui::retained::UiElementFrame &frame);

} // namespace shooter
