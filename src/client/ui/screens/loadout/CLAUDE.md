# src/client/ui/screens/loadout/

The loadout screen — the only screen complex enough to need its own components dir + a screen-local React-style provider. Treat this as the reference template for screens that grow beyond a single `<screen>_screen.{h,cpp}` file.

## Files

- `loadout_screen.{h,cpp}` — `LoadoutScreen` class (no member fields; the class is just the `UiScreen` entry point) + `LoadoutScreenView` body + `use_push_loadout_screen`.
- `loadout_state.{h,cpp}` — `LoadoutPendingAction` + `LoadoutContextValue` + the `LoadoutContext` provider (`use_loadout_context_value`, `loadout_provider_push/pop`) + hooks (`use_compare_enabled`, `use_set_compare_enabled`, `use_selected_weapon_tile`, `use_set_selected_weapon_tile`, `use_pending_loadout_action`, `use_set_pending_loadout_action`, `use_clear_pending_loadout_action`).
- `components/weapon_tile.{h,cpp}` — `WeaponTile` + tab constants (`LOADOUT_TAB_WEAPONS`, `LOADOUT_TAB_GEAR`) + helpers.
- `components/equipment_slot.{h,cpp}` — `EquipmentSlot`.
- `components/confirm_dialog.{h,cpp}` — `LoadoutConfirmDialog` (no props; reads pending state from `LoadoutContext`).

## Conventions for screen-local state

- **Don't** store screen-local UI state as a member on the screen class. Hold it inside `build_ui()` with `use_state<T>` and expose it through a provider.
- The pattern: `LoadoutScreen::build_ui` declares `use_state<bool>(...)` / `use_state<int>(...)` / `use_state<LoadoutPendingAction>({})` slots, builds a context value with `use_loadout_context_value(...)`, wraps the body with `loadout_provider_push(&ctx)` / `loadout_provider_pop()`, then renders `LoadoutScreenView()`.
- Descendant components consume state via the typed hooks (`use_compare_enabled()`, `use_selected_weapon_tile()`, `use_pending_loadout_action()`) and mutate it via the setter hooks. Don't add a back-channel that reaches into the screen class.
- Setters returned from `use_set_*` hooks already schedule deferred UI mutations — so they're safe to call from inside Clay layout (button `on_confirm`, focus `on_focus`, etc.). Keep the low-level mutation sink inside `loadout_state.cpp`.
- Focus handlers (`on_focus`) must only mutate UI state. Game-state mutations (e.g. `select_weapon`, `buy_weapon`, `equip_weapon`) belong on confirm flows, not on focus traversal.

## When to graduate a component to the parent dir

- A component used by another screen → move to `client/ui/components/`.
- A hook used by another screen → move to `client/ui/hooks/`.
- A provider/context used by another screen → move to `client/ui/providers/`.

Co-location wins by default; promote only when there's a real second consumer.
