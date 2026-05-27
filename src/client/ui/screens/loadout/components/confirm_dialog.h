#pragma once

namespace shooter {

// Reads the pending action from `LoadoutContext` and renders the confirm
// modal. Returns early when no action is pending. Action constants and the
// pending struct live in `loadout_state.h`.
void LoadoutConfirmDialog();

} // namespace shooter
