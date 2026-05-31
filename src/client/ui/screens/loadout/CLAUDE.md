# src/client/ui/screens/loadout/

The loadout screen — the most complex screen, fully decomposed into semantic components + a screen-local React-style provider. Treat this as the reference template for a feature module: a screen component, a private provider, public hooks, and a `components/` dir with one semantic component per file. Add screen-local `hooks/`, `providers/`, or `lib/` folders only when a concept needs that boundary inside the feature.

## Files

- `loadout_screen.{h,cppx}` — the screen entry point and screen component. Keep the class wrapper free of member UI state; the component wraps descendants in `LoadoutProvider` and keeps the provider-child content component local to the screen file.
- `providers/loadout_provider.{h,cpp}` — `LoadoutProvider`, its private `LoadoutContext`, the `use_state` slots, and setter implementations. The context value struct stays private to this file.
- `hooks/use_loadout.h` — the `use_loadout()` consumer hook. It returns a `LoadoutValue` object with read fields (`compare_enabled`, `selected_weapon_tile`, `pending`) plus setter callbacks.
- `loadout_tokens.h` — `shooter::loadout` feature-local tokens (root dialog bg, tab/grid/details geometry); reuses the shared `shooter::tokens` builders.
- `components/loadout_screen_frame.{hx,cppx}` — `LoadoutScreenFrame` (root `Dialog`, owns `modal=!confirm_open`).
- `components/loadout_title.{hx,cppx}`, `loadout_tabs.{hx,cppx}` (`LoadoutTabs` + compound `LoadoutTabs::Tab`, role=Tab internal), `loadout_body.{hx,cppx}`, `loadout_weapon_grid.{hx,cppx}` (reproduces the per-tab grid rows), `loadout_details.{hx,cppx}` (`Panel` Sunken + the frozen ordered action/slot children).
- `components/weapon_tile.{h,cpp}` — `WeaponTile` + tab constants (`LOADOUT_TAB_WEAPONS`, `LOADOUT_TAB_GEAR`, `WEAPON_TILE_CONTROL_ID`) + helpers; adapts `ui::components::Button` directly (bespoke tile geometry).
- `components/equipment_slot.{h,cpp}` — `EquipmentSlot` (adapts `Button` directly).
- `components/confirm_dialog.{h,cpp}` — `LoadoutConfirmDialog` (reads pending state from `LoadoutContext`).
- `hooks/use_weapons.{h,cpp}` — loadout-local aggregate weapon hook over `ShooterGame`; returns count, indexed read fields, and deferred `select`/`buy`/`equip` actions.

`weapon_tile`/`equipment_slot`/`confirm_dialog` stay plain `.h/.cpp`: they are already clean domain components (semantic props, adapt primitives directly, `tokens::` paint), so JSX conversion would be cosmetic.

## Conventions for screen-local state

- **Don't** store screen-local UI state as a member on the screen class. Hold it inside the provider component with `use_state<T>` and expose it through hooks.
- The pattern: the screen component renders `LoadoutProvider`, and `LoadoutProvider` declares the `use_state<bool>(...)` / `use_state<int>(...)` / `use_state<LoadoutValue::PendingAction>({})` slots before returning the private `LoadoutContext` provider.
- Descendant components consume state via `use_loadout()` and mutate through the setter callbacks on the returned `LoadoutValue`. Don't add a back-channel that reaches into the screen class.
- Setters returned from `use_loadout()` already schedule deferred UI mutations — so they're safe to call from retained control callbacks (`on_activate`, `on_focus`, etc.). Keep the low-level mutation sink inside `providers/loadout_provider.cpp`.
- Focus handlers (`on_focus`) must only mutate UI state. Game-state mutations (e.g. `select_weapon`, `buy_weapon`, `equip_weapon`) belong on confirm flows, not on focus traversal.

## When to graduate a component to the parent dir

- A component used by another screen → move to `client/ui/components/`.
- A hook used by another screen → move to `client/ui/hooks/`.
- A provider/context used by another screen → move to `client/ui/providers/`.

Co-location wins by default; promote only when there's a real second consumer.
