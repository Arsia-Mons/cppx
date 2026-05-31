# src/game/

Game truth: rules, state, simulation. This is the shooter — health, armor, credits, weapons, buy economy, inventory. The split mirrors HL1/CS-era patterns (player state, inventory, rules) without inventing entity hierarchies we don't need yet.

## Files

- `shooter_game.{h,cpp}` — façade aggregate. Owns `PlayerState` + `Inventory`; delegates queries/mutations.
- `player_state.h` — health, armor, credits. Header-only.
- `weapon_defs.h` — `WeaponSpec` (immutable per-weapon data).
- `weapon_state.h` — `WeaponState` (per-instance: spec + owned + equipped).
- `inventory.{h,cpp}` — `Inventory` (array of weapons, selected index, equip rules).
- `economy.{h,cpp}` — free functions over `PlayerState` + `Inventory`: `can_buy_weapon`, `try_buy_weapon`.

## Hard rules

- **No SDL or UI headers.** If you have to include one, the file is in the wrong directory.
- **No per-frame timing or rendering concerns.** Game functions accept the inputs they need and return state changes; `app/GameLoop` decides when to call them.
- UI state (selected tab, compare panel toggle, hover, modal visibility) does NOT live here. That's `client/ui/screens/<screen>/`.
- The façade pattern on `ShooterGame` is for stability of UI call sites. Direct callers (eventually `app/`, server-side simulation) can use the decomposed types directly — `player_state`, `inventory`, `economy`.
