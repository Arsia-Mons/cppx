# src/client/ui/screens/loadout/

The loadout screen — the most complex screen, fully decomposed into semantic components + a screen-local React-style provider. Treat this as the reference template for a feature module: a thin screen entry point, a `_state` provider, public action hooks in `_actions`, and a `components/` dir with one semantic component per file.

## Files

- `loadout_screen.{h,cpp}` — `LoadoutScreen` class (no member fields; just the `UiScreen` entry point) + `LoadoutScreenView` (declares the `use_state` slots + `LoadoutProvider` wrap, renders `LoadoutScreenBody`). Re-includes `loadout_actions.h` so consumers resolve the nav hook via this header.
- `loadout_actions.{h,cpp}` — `use_push_loadout_screen()` (public navigation hook).
- `loadout_state.{h,cpp}` — `LoadoutPendingAction` + `LoadoutContextValue` + the `LoadoutContext` provider (`use_loadout_context_value`, `loadout_provider_push/pop`) + hooks (`use_compare_enabled`, `use_set_compare_enabled`, `use_selected_weapon_tile`, `use_set_selected_weapon_tile`, `use_pending_loadout_action`, `use_set_pending_loadout_action`, `use_clear_pending_loadout_action`).
- `loadout_tokens.h` — `shooter::loadout` feature-local tokens (root dialog bg, tab/grid/details geometry); reuses the shared `shooter::tokens` builders.
- `components/loadout_content.{h,cpp}` — `LoadoutScreenBody`: holds all loadout hook reads + the action closures (incl. the gear-tab 2-deferred-write contract), returns the hand-written `::ui::fragment(LoadoutConfirmDialog, LoadoutScreenFrame{...})`. Plain `.cpp` (a fragment body can't be JSX — the transpiler enters JSX only on a leading `<`).
- `components/loadout_screen_frame.{hx,cppx}` — `LoadoutScreenFrame` (root `Dialog`, owns `modal=!confirm_open`).
- `components/loadout_title.{hx,cppx}`, `loadout_tabs.{hx,cppx}` (`LoadoutTabs` + compound `LoadoutTabs::Tab`, role=Tab internal), `loadout_body.{hx,cppx}`, `loadout_weapon_grid.{hx,cppx}` (reproduces the per-tab grid rows), `loadout_details.{hx,cppx}` (`Panel` Sunken + the frozen ordered action/slot children).
- `components/weapon_tile.{h,cpp}` — `WeaponTile` + tab constants (`LOADOUT_TAB_WEAPONS`, `LOADOUT_TAB_GEAR`, `WEAPON_TILE_CONTROL_ID`) + helpers; adapts `ui::components::Button` directly (bespoke tile geometry).
- `components/equipment_slot.{h,cpp}` — `EquipmentSlot` (adapts `Button` directly).
- `components/confirm_dialog.{h,cpp}` — `LoadoutConfirmDialog` (reads pending state from `LoadoutContext`).

`weapon_tile`/`equipment_slot`/`confirm_dialog` stay plain `.h/.cpp`: they are already clean domain components (semantic props, adapt primitives directly, `tokens::` paint), so JSX conversion would be cosmetic.

## Conventions for screen-local state

- **Don't** store screen-local UI state as a member on the screen class. Hold it inside `build_ui()` with `use_state<T>` and expose it through a provider.
- The pattern: `LoadoutScreen::build_ui` declares `use_state<bool>(...)` / `use_state<int>(...)` / `use_state<LoadoutPendingAction>({})` slots, builds a context value with `use_loadout_context_value(...)`, wraps the body with `loadout_provider_push(&ctx)` / `loadout_provider_pop()`, then renders `LoadoutScreenView()`.
- Descendant components consume state via the typed hooks (`use_compare_enabled()`, `use_selected_weapon_tile()`, `use_pending_loadout_action()`) and mutate it via the setter hooks. Don't add a back-channel that reaches into the screen class.
- Setters returned from `use_set_*` hooks already schedule deferred UI mutations — so they're safe to call from retained control callbacks (`on_activate`, `on_focus`, etc.). Keep the low-level mutation sink inside `loadout_state.cpp`.
- Focus handlers (`on_focus`) must only mutate UI state. Game-state mutations (e.g. `select_weapon`, `buy_weapon`, `equip_weapon`) belong on confirm flows, not on focus traversal.

## When to graduate a component to the parent dir

- A component used by another screen → move to `client/ui/components/`.
- A hook used by another screen → move to `client/ui/hooks/`.
- A provider/context used by another screen → move to `client/ui/providers/`.

Co-location wins by default; promote only when there's a real second consumer.
