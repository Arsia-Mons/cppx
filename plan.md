# Plan: Survivor Arena — foundation hardening

A staged plan for evolving the current React-over-Clay runtime into a real,
scalable game-UI foundation by building one playable thing — a minimal
top-down survivor arena — that exercises every primitive in the architecture.

The goal is not the game. The goal is to prove the architecture under load,
in a system small enough to keep on a desk.

---

## North star

Single-screen top-down 2D arena:

- One player pawn (auto-fires at nearest enemy).
- Enemies spawn in waves and walk toward the player.
- Kills drop XP gems; player auto-collects on touch.
- Every N XP, gameplay **pauses** and a **Choose Upgrade** modal pushes onto
  the screen stack. Picking an upgrade pops the modal and resumes gameplay.
- ESC opens a Pause menu. Settings is reachable from Pause.
- Death → Game Over → Restart from clean state.
- Title screen at boot.

Why this shape, and not anything else: it is the smallest playable loop that
forces the read and write paths between UI and game, the engine-owned screen
stack with real pause semantics, and a level-up modal that interrupts gameplay
— i.e. every primitive in the architecture, all at once, under real
actor-count load.

---

## Foundations under test

The whole point of building this is to perfect these foundations. Every phase
maps to one or more of:

- [ ] **Engine seam** — engine tick runs before `react_begin_frame`; world is
      committed before UI reconciles.
- [ ] **Actor model** — typed pools with `Handle = {index, generation}`,
      cache-friendly, dangling-safe, O(1) spawn / resolve / destroy.
- [ ] **Read path** — components read state through named domain hooks
      (`use_player()`, `use_enemies()`, …) that pull `const World&` from
      context and project typed slices. Components never touch raw
      `use_context(&WorldContext)`. Because the current React-over-Clay
      runtime re-runs every fiber every frame, there is no subscription /
      memoization layer; adding one is a future optimization, not a
      foundation.
- [ ] **Write path** — UI never mutates the world directly. It calls hook actions
      (`use_navigation()`, `use_upgrade_actions()`, `use_settings_actions()`, etc.)
      that wrap public methods on the owning subsystem of the game object. The hook
      layer exists only for mutations a UI surface *initiates* — buttons, menu
      choices, settings toggles. In-world controls (player movement, firing, AI)
      stay engine-internal; the engine reads `Input` and drives them directly,
      never through hooks. A formal command queue is *not* a foundation; introduce
      one inside a specific subsystem only if that subsystem needs ordering /
      replay / network authority.
- [ ] **Screen stack** — `ui_stack` lives on the world; engine reads it to
      gate which systems run; UI is a transparent renderer of it.
- [ ] **Screen lifecycle** — `entering / entered / exiting / exited` phase
      per entry; `use_screen_lifecycle({on_enter, on_exit, on_focus, on_blur})`
      hook fires in deterministic order across stack transitions.
- [ ] **Identity discipline** — every UI element has a fiber-stable Clay ID;
      handles are 64-bit; no raw pointers escape the engine.
- [ ] **Lifecycle correctness under cancellation** — die mid-fetch,
      mid-level-up, mid-transition: no leaks, no UB, no stale UI state.

If, at the end, every box is checked and the demo plays without hacks, the
foundation is real.

---

## UI contract

The contract is about *roles*, not enforcement machinery. Two rules:

### Reads: components call named domain hooks

A component asks for what it wants by name:

```cpp
auto player  = use_player();
auto enemies = use_enemies();
auto stack   = use_screen_stack();
```

Each domain hook pulls `const World&` from context and projects out the typed
slice its consumers actually want. Hooks live alongside the owning subsystem;
components don't navigate the world's shape themselves, and they never touch
raw `use_context(&WorldContext)`.

If some future read is genuinely one-off and adding a domain hook feels like
ceremony, that is a signal to stop and either (a) name the concept properly
or (b) accept that the read belongs at a higher level of the tree where a hook
already exists. A generic selector escape hatch is *not* the answer — we are
choosing not to have one.

There is no subscription tracking, no version stamps, no memoization — the
React-over-Clay runtime re-runs every fiber every frame anyway, so reads are
already as cheap as they're going to be. If profiling later shows reconciliation
cost is the bottleneck, *then* add memoization. Until then, don't.

### Writes: components call hook actions that wrap subsystem methods

UI never mutates the world directly. It calls named action hooks:

```cpp
auto nav      = use_navigation();
auto upgrades = use_upgrade_actions();
auto settings = use_settings_actions();

nav.pause();
upgrades.choose(option_id);
settings.set_master_volume(v);
```

The hook implementation is a thin wrapper over a public method on the owning
subsystem of the game object — `game.nav.pause()`, `game.upgrades.choose(...)`,
`game.audio.set_master_volume(...)`. The subsystem owns validation and business
logic.

**Hook actions are exclusively for mutations a UI surface initiates** — Play
buttons, Pause/Resume, modal selections, settings toggles, Restart. In-world
controls (player movement, auto-firing, enemy AI, XP collection, level-up
triggering) are engine-internal: the engine reads `Input` directly inside
`engine_tick` and drives the relevant subsystem itself. The UI never sees a
`move_player` or `fire_at` hook because it has no business calling one.

There is **no universal command bus** and **no internal command queue** at the
foundation layer. If a specific subsystem later needs deterministic ordering,
replay, or network authority, that subsystem can introduce a queue inside its
own implementation without changing any caller. Until a real subsystem needs
that, the simpler shape is the right shape.

The point of the hook layer is the React-shaped seam — actions are looked up
through context (so unmounted components can't keep calling them), hooks don't
hand out long-lived references that survive renders, and components stay
declarative. Not a compile-time `World&` ban.

### Rendering boundary

Direct SDL drawing is allowed for the world-scene pass: arena background,
player, enemies, bullets, gems. React/Clay owns HUD, menus, modals, screen
stack rendering, and floating damage numbers. Foundation-relevant visuals must
expose stable Clay IDs.

---

## Explicit non-goals (out of scope)

These are research projects in their own right; each will distort the
foundation if pulled in too early. Excluded for this plan:

- **ECS** (archetypes, queries, system scheduling). See "Actor model" below.
- Networking, replication, rollback.
- Save / load.
- Mod loading, scripting, asset hot-reload.
- Animation system beyond linear tweens.
- Particles beyond floating damage numbers.
- Sound design beyond two music tracks + duck-on-pause.
- Pathfinding (enemies move in straight lines toward the player).
- Spatial partitioning (brute-force collision is fine at this scale; revisit
  only if profiling demands it).
- Content tooling, editor, level files.

If any of these become necessary to ship a phase, that is a signal that the
phase is over-scoped, not that the non-goal needs to move.

---

## Actor model: pools + handles, not ECS

We need a real, scalable actor model. We don't need ECS to get one.

**The model:**

```cpp
template <typename T, uint32_t N>
struct Pool {
    T        data[N];
    uint32_t generation[N];   // bumped each time a slot is reused
    bool     alive[N];
    uint32_t free_head;       // intrusive free list through data[].next_free
    uint32_t count;           // live count, for fast iteration bounds
};

template <typename T>
struct Handle {
    uint32_t index;
    uint32_t generation;       // 0 == null
};

// Resolve returns nullptr if the slot is dead or generation mismatches.
template <typename T, uint32_t N>
T *resolve(Pool<T,N> &pool, Handle<T> h);
```

**Why this is enough:**

- Contiguous arrays per actor type → cache-friendly iteration.
- Stable 64-bit handles → safe to hold across frames, across UI, across
  worker threads. A stale handle resolves to `nullptr`, not a segfault.
- No allocation per spawn (pools pre-sized; can grow by doubling if needed).
- Loops over actors are written by hand: explicit, debuggable, no query DSL.

**What we give up vs ECS:**

- No ad-hoc composition: an Enemy can't grow a `Burning` component without a
  code change. (For the survivor scope, all enemy state fits in one struct.)
- No cross-type queries: a "bullet hits enemy" loop is written explicitly
  rather than synthesised. (We have one or two such pairings, by hand, fine.)
- No system scheduler / parallelism. (Survivor doesn't need it.)

**The actor types in this app (final state):**

| Type           | Pool size hint | Notes                                          |
|----------------|----------------|------------------------------------------------|
| `Player`       | 1              | Singleton, but lives in a pool for uniformity. |
| `Enemy`        | 1024           | Brute-force collision against bullets.         |
| `Bullet`       | 2048           | High churn, short lifetime.                    |
| `XpGem`        | 512            |                                                |
| `DamageNumber` | 256            | World-space UI hybrid; floats up and fades.    |

If a pool fills up, oldest non-essential entries are recycled (bullets, gems,
damage numbers). The player and enemies are never silently recycled.

---

## Phases

Each phase is shippable (the build runs and demonstrates the phase's goal)
and demoable (you can watch it on screen). **Do not start a phase until the
previous phase's acceptance is met.** Profile at the end of each phase.

---

### Phase 1 — Engine seam + actor pools

**Foundations:** engine seam, actor model.

**Goal:** prove the engine→render→UI ordering and the actor-pool primitive.

**Scope:**
- [ ] Add `src/engine/world.h` / `world.cpp` with a `World` struct.
- [ ] Add `src/engine/pool.h` with the `Pool<T,N>` template + `Handle<T>`.
- [ ] Add `Player` actor type (position, hp). Spawn one on world init.
- [ ] Add `src/engine/engine.h` / `engine.cpp` with `engine_tick(World&, const Input&, float dt)`.
      The engine reads keyboard state from the `Input` snapshot and writes
      player position directly inside tick. Player movement stays engine-internal
      throughout the plan — the UI never drives it. Phase 3 adds the hook
      action layer for the mutations UI legitimately owns (navigation first).
- [ ] Wire `engine_tick` into `main.cpp` immediately before
      `react_begin_frame()`, feeding it the same `Input` struct already
      threaded into `App()`. Add a `WorldContext` provider in `App()` carrying
      `const World&` to children.
- [ ] Add a `use_player()` domain hook that reads the world from context and
      returns player info (position, hp, …). Components consume domain hooks
      like this; they never touch raw `use_context(&WorldContext)`.
- [ ] HUD: render `HP: %d` via `auto p = use_player(); ... p.hp`.
- [ ] Render the player as a colored circle through the chosen world-scene pass.
      If that pass is direct SDL, Phase 1 must still prove the React/Clay HUD
      reads the committed world after `engine_tick` and uses stable Clay IDs.

**Acceptance:**
- WASD moves the player visibly.
- HUD shows the player's HP from the world.
- 60fps with vsync.
- No leaks on clean shutdown.
- `Handle<Player>` resolves correctly; resolving a manually-destroyed handle
  returns `nullptr`.

---

### Phase 2 — Many actors stress test

**Foundations:** actor model under load.

**Goal:** prove pools and the tick scale before adding game logic on top.

**Scope:**
- [ ] Add an `Enemy` pool and a debug `spawn_enemies(count)` call wired to a
      hotkey.
- [ ] Enemies have position + velocity; tick: `pos += vel * dt`, wrap or
      bounce at arena edges.
- [ ] Render all live enemies as circles each frame.
- [ ] Add an on-screen perf overlay: live entity count, tick ms, render ms,
      total frame ms.

**Acceptance:**
- 1,000 enemies ticking and rendering at ≥60fps.
- Linear scaling (no sudden cliffs) — verify by sweeping 100 / 500 / 1000.
- Destroying enemies (via another hotkey) leaves the pool clean: live count
  drops correctly, generation bumps, resolve of old handles returns null.

---

### Phase 3 — Write path: navigation as the first UI-driven action

**Foundations:** write path.

**Goal:** establish the action-hook plumbing using the first place UI
legitimately mutates state — a Title screen whose Play button starts the game.
In-world controls remain engine-internal.

**Scope:**
- [ ] Define the top-level `Game` object with subsystems (e.g. `game.nav`,
      and the existing player/combat code now grouped under `game.player`,
      `game.combat`). Subsystems expose public methods for the mutations they
      own.
- [ ] Add a minimal `current_screen` field on the world — `Title | Gameplay`.
      (The full stack arrives in Phase 6; for now a single scalar is enough.)
      `game.nav` owns it.
- [ ] Add `use_navigation()` returning bound callables: `play()`,
      `quit_to_title()`. Each forwards to a method on `game.nav`.
- [ ] Plumb `Game&` through context the same way `WorldContext` carries the
      read view; action hooks pull it from context.
- [ ] Engine tick gates gameplay systems on `current_screen == Gameplay`: on
      `Title`, enemies and player input are frozen.
- [ ] Add a Title screen component with a Play button that calls `nav.play()`.

**Acceptance:**
- Boot lands on Title; world is frozen, player input ignored.
- Clicking Play transitions to Gameplay; WASD and enemies behave as in Phase 2.
- In-world controls (movement, enemy ticking) are still driven by `engine_tick`
  reading `Input` — Phase 1's input path is unchanged, no hook touches them.
- Action hooks compose cleanly — adding a new subsystem method and exposing it
  through a hook is a small, local change.

---

### Phase 4 — Bullets, collisions, enemy death

**Foundations:** actor model, read path, write path (integration test under
real game logic).

**Goal:** a working combat loop, no UI polish yet.

**Scope:**
- [ ] Add a `Bullet` pool (position, velocity, owner handle, lifetime).
- [ ] Engine fire system: each tick, find the nearest enemy to the player,
      spawn a bullet aimed at it on a cooldown.
- [ ] Bullet tick: integrate position, decrement lifetime, despawn at zero.
- [ ] Collision: brute-force pairwise loop over bullets × enemies. On hit:
      bullet despawns, enemy takes damage, dead enemy spawns an XP gem.
- [ ] Add an `XpGem` pool (position, value). Player auto-collects within
      radius on tick.
- [ ] HUD: add `Kills: N` and `XP: N` from the world.

**Acceptance:**
- 100 enemies + 100 bullets in flight at ≥60fps.
- Player can clear an arena of enemies; XP accumulates.
- Enemy death always frees its pool slot; generation bumps verified.
- Stop-the-world test: spawn 500 enemies + 500 bullets, then trigger
  collisions; no hangs, no leaks.

---

### Phase 5 — Smooth visuals: tweens and floating damage numbers

**Foundations:** none new. This phase exercises the read path under
continuously-animating UI without introducing a new lane.

**Goal:** make the HUD feel alive — HP bar interpolates smoothly, damage
numbers float and fade — using only the seams already established.

**Scope:**
- [ ] Add tween state to the HP bar component using `use_state_int` to hold
      `displayed_hp_centi` (HP × 100). Each render, lerp it toward
      `actual_hp_centi`; divide by 100 for the bar fill. This uses only the
      hook the runtime exposes today (`src/react.h:99`); no new primitive.
- [ ] Add a `DamageNumber` pool to the world (position, value, elapsed time,
      lifetime). Spawn on enemy hits; engine ticks the elapsed clock and
      retires entries past their lifetime.
- [ ] Render damage numbers as Clay elements: a React component reads the
      pool via a `use_damage_numbers()` domain hook and emits one element per
      live entry, with layout position computed each frame from
      `world_pos + tween(elapsed)`. The component re-runs every frame; this
      is fine.
- [ ] If profiling later shows per-frame reconciliation of the HUD subtree
      is a real cost, design selective memoization or a sidecar binding
      mechanism then. Not before.

**Acceptance:**
- HP bar fills/empties smoothly even when HP itself only changes in
  discrete steps.
- Damage numbers track moving enemies and fade out.
- 60fps holds with the HUD running every frame at full reconciliation.

---

### Phase 6 — Screen stack

**Foundations:** screen stack, engine-driven tick gating.

**Goal:** the engine owns navigation; UI is its viewer.

**Scope:**
- [ ] Promote `ScreenId` to a full enum: `Title`, `Gameplay`, `Pause`.
- [ ] Replace the Phase 3 `current_screen` scalar with `ui_stack:
      std::vector<ScreenEntry>` on the world. `game.nav` now owns the stack.
- [ ] Extend `use_navigation()` with the full vocabulary: `push_screen(id)`,
      `pop_screen()`, `replace_screen(id)`, plus named helpers (`pause()`,
      `resume()`, `quit_to_title()`; `play()` from Phase 3 is preserved).
      All wrap public methods on `game.nav`.
- [ ] Engine tick reads the top of the stack to decide which systems run:
      `Gameplay` → run combat / AI / physics; `Pause` / `Title` → skip
      gameplay systems, keep cosmetic ones (e.g. damage-number fade is
      paused too).
- [ ] Add a `use_screen_stack()` domain hook and a `<ScreenStackRenderer>`
      component that reads the stack through it and renders the screen
      component(s) for each entry. Lower entries stay mounted but visually
      layered behind upper ones.
- [ ] Implement `Title`, `Gameplay`, `Pause` screen components. Title has a
      Play button that calls a navigation action. Pause has Resume and Quit
      actions.
- [ ] ESC during Gameplay calls the pause navigation action.

**Acceptance:**
- ESC pushes Pause; gameplay tick is frozen but world remains rendered behind
  the menu (verify by watching enemies stop moving).
- ESC (or Resume) pops Pause; gameplay resumes seamlessly.
- Title → Gameplay flow works on boot.
- Stack depth of 2+ works (e.g. Pause pushed over Gameplay).
- Gameplay pools are unchanged while Pause / Title is topmost (verifiable by
  watching live counts / positions). Transition clocks still advance.

---

### Phase 7 — Lifecycle + transitions

**Foundations:** screen lifecycle.

**Goal:** screens have real lifecycle and animated transitions; events fire
in a deterministic order across stack changes.

**Scope:**
- [ ] Each `ScreenEntry` carries a transition phase (`entering`, `entered`,
      `exiting`, `exited`) and a phase clock (e.g. 200 ms enter / exit).
- [ ] Engine advances phase clocks each tick (and does so even when
      gameplay is paused; transitions never freeze).
- [ ] Add `use_screen_lifecycle({on_enter, on_exit, on_focus, on_blur})` hook.
      Order across a push: previous-top `on_blur` → new-top `on_enter` →
      new-top `on_focus` (after `entered`). Reverse on pop.
- [ ] Visual transitions: backdrop fade for modals, scale-in for modals,
      crossfade for screen swaps.
- [ ] Audio side-effect tests: dock a music-duck on Pause's `on_enter`,
      undock on `on_exit`. (Audio system may be a stub that logs.)

**Acceptance:**
- Smooth animated push and pop; no popping / flashing.
- Performing Boot → Play → Pause → Resume → Quit by hand produces a debug
  `printf` trail of lifecycle events whose order matches the spec
  (previous-top `on_blur` → new-top `on_enter` → new-top `on_focus` on push,
  reversed on pop). Eyeball-verified during the demo.
- Reversing a transition mid-flight works (push Pause, immediately pop while
  still `entering` — ends `exited` correctly, also visible in the printf trail).

---

### Phase 8 — Level-up loop (integration test)

**Foundations:** all of the above, simultaneously.

**Goal:** prove the architecture under the canonical hard case — gameplay
interrupted by a modal that pauses the world, takes input, and applies a
mutation through the write lane.

**Scope:**
- [ ] Add player `level` and an XP threshold table.
- [ ] When `player.xp >= threshold[level]`: engine code (inside `engine_tick`)
      detects the threshold and calls `game.nav.push(ChooseUpgrade, payload)`
      directly, where `payload` is 3 randomly-selected upgrade options. The
      engine bypasses the hook layer here — only UI surfaces use hooks; engine
      code mutating its own subsystems is the normal in-process path.
- [ ] Add `ChooseUpgrade` screen: renders the 3 options, selects one by
      keyboard or click, and calls a hook action such as
      `use_upgrade_actions().choose(id)`. The action wraps a public method on
      `game.upgrades` that validates the offer, applies the stat change, and
      closes the modal.
- [ ] Upgrades affect real player stats (fire rate, damage, move speed,
      max HP, etc. — small fixed catalogue).
- [ ] Multiple level-ups in quick succession queue correctly: pop → next
      level threshold met → push again immediately.

**Acceptance:**
- Full loop plays: kill enemies → XP fills → modal appears, gameplay frozen
  → choose upgrade → modal closes, gameplay resumes with new stats.
- Modal scale-in / out animates cleanly (phase 7 still works).
- HUD HP / XP bars update smoothly throughout (phase 5 still works).
- Triggering a level-up while another modal is animating in/out behaves
  deterministically (queued, not dropped, not interleaved).

---

### Phase 9 — Death, restart, title shell

**Foundations:** lifecycle correctness under cancellation; cleanup.

**Goal:** clean boot, clean death, clean restart, no leaks.

**Scope:**
- [ ] HP ≤ 0 → engine code (inside `engine_tick`) calls `game.nav.push(GameOver)`
      directly. This is engine-internal, not a hook action — the *engine* detected
      the death condition, not a UI surface.
- [ ] `GameOver` screen has Restart → calls `use_run_actions().restart()`, which
      wraps `game.run.restart()` (a public method that performs `ResetWorld` plus
      the navigation transition back to `Gameplay`).
- [ ] `ResetWorld` operation: clear all pools, reset player, reset stack to
      `[Gameplay]` (or `[Title]` depending on flow choice).
- [ ] Add a Settings screen reachable from Pause (and from Title). Two
      settings: master volume, fullscreen toggle. UI calls
      `use_settings_actions().set_master_volume(v)` and `.set_fullscreen(b)`,
      which wrap public methods on `game.settings`. Persist nothing for now.
- [ ] Boot flow: stack starts as `[Title]`.
- [ ] Edge tests: die during a level-up modal; die during a screen
      transition; quit mid-fetch (carry over the existing Image fetch
      cancellation pattern from `image.cpp` — `cancel_fetch` is the
      reference shape for cancellable async).

**Acceptance:**
- Clean boot to Title.
- Play → die → Game Over → Restart → fresh game, no stale entities, no
  stale UI state.
- Quit during any screen transition leaves no leaked threads / textures /
  listeners.
- Dying with a level-up modal already on the stack resolves to GameOver
  with the upgrade modal correctly torn down.

---

## Definition of done (whole plan)

All nine phases meet acceptance, and:

- [ ] All "Foundations under test" checkboxes ticked.
- [ ] No phase required a hack that violated the architecture rules
      (UI never mutates world directly; world never holds raw UI pointers;
      screen lifecycle is deterministic).
- [ ] 1,000 enemies + 200 bullets + HUD sustains ≥60fps.
- [ ] The level-up loop in Phase 8 plays cleanly back-to-back ten times
      without a single frame of jank or a missed lifecycle event.

When this is true, the foundation is real and the next game built on it does
not need to revisit any of these decisions.
