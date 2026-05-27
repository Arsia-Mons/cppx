# src/client/ui/screens/loadout/

The loadout screen — the only screen complex enough to need its own components dir + screen-local UI state. Treat this as the reference template for screens that grow beyond a single `<screen>_screen.{h,cpp}` file.

## Files

- `loadout_screen.{h,cpp}` — `LoadoutScreen` class (UI state lives here: `compare_enabled_`), `LoadoutScreenView`, `use_push_loadout_screen`, `use_compare_enabled` / `use_set_compare_enabled` hooks.
- `components/weapon_tile.{h,cpp}` — `WeaponTile` + tab constants (`LOADOUT_TAB_WEAPONS`, `LOADOUT_TAB_GEAR`) + helpers.
- `components/equipment_slot.{h,cpp}` — `EquipmentSlot`.
- `components/confirm_dialog.{h,cpp}` — `LoadoutConfirmDialog` + action constants (`LOADOUT_ACTION_*`).

## Conventions for screen-local state

- Screen-local state (e.g., `compare_enabled_`) is a private member on the screen class, with public getter/setter.
- Expose the screen to descendant components via a `ReactContext` provided in `build_ui()`. Descendant hooks call `use_current_loadout_screen()` to reach the state.
- Mutations from inside Clay layout MUST go through `client::ui::use_ui_write_queue()` — never write directly.

## When to graduate a component to the parent dir

- A component used by another screen → move to `client/ui/components/`.
- A hook used by another screen → move to `client/ui/hooks/`.
- A provider/context used by another screen → move to `client/ui/providers/`.

Co-location wins by default; promote only when there's a real second consumer.
