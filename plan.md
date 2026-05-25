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
- [ ] **Write path** — UI never mutates the world directly. It calls **domain
      action hooks** (`use_run_actions()`, `use_pause_actions()`,
      `use_upgrade_actions()`, `use_settings_actions()`, etc.) that wrap public
      methods on the owning subsystem of the game object. There is no
      component-facing `use_navigation()` exposing raw `push/pop/replace`; the
      three nav primitives live on `game.nav` and are reached only through
      domain hooks. The hook layer exists only for mutations a UI surface
      *initiates* — buttons, menu choices, settings toggles. In-world controls
      (player movement, firing, AI) stay engine-internal; the engine reads
      `Input` and drives them directly, never through hooks. A formal command
      queue is *not* a foundation; introduce one inside a specific subsystem
      only if that subsystem needs ordering / replay / network authority.
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

### Writes: components call domain action hooks that wrap subsystem methods

UI never mutates the world directly. It calls **domain action hooks** owned
by the subsystem most relevant to the verb:

```cpp
auto pause    = use_pause_actions();
auto upgrades = use_upgrade_actions();
auto settings = use_settings_actions();

pause.resume();
upgrades.choose(option_id);
settings.set_master_volume(v);
```

Each hook's implementation is a thin wrapper over a public method on the
owning subsystem of the game object — `game.pause.exit()`,
`game.upgrades.choose(...)`, `game.audio.set_master_volume(...)`. The
subsystem owns validation and business logic. For verbs that mutate the
screen stack, the subsystem method calls into `game.nav` (the single
serialization point — see below). There is no component-facing
`use_navigation()` hook; raw `push/pop/replace` are not user verbs.

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

### Navigation: single serialization point, narrow vocabulary

`game.nav` is the single serialization point for stack mutations. Both
engine-direct calls (e.g. level-up detection in `engine_tick`) and
UI-initiated calls (which arrive via domain action hooks → subsystem
methods → `game.nav`) enter the same public methods on `game.nav`. Those
methods validate against the current stack state (reject duplicate-top
pushes, reject pops on an empty stack, reject illegal transitions like
pushing `GameOver` when `GameOver` is already top) and are the only
writers of the stack. No other subsystem and no UI surface mutates the
stack directly.

`game.nav`'s public surface is exactly three methods:

```cpp
uint32_t push_screen(ScreenId id, /* payload args */);          // returns entry_id
void     pop_screen(uint32_t entry_id);                          // identity-addressed
void     replace_screen(uint32_t entry_id, ScreenId, /* ... */); // identity-addressed
```

`pop_screen` and `replace_screen` are **identity-addressed, not
position-addressed**. They take the `entry_id` of the entry the caller
*believes* is on top and reject (or no-op with a logged warning) if that
entry is not actually top. Position-addressed pops are unsafe under the
engine-first-then-UI frame ordering (`src\main.cpp:166-171`): an engine
push in `engine_tick` lands before UI render, so a UI action that wanted to
pop "its own modal" could instead pop the entry the engine just pushed
above it. Forcing every pop to name its target makes that race a loud
failure instead of a silent stack corruption.

UI surfaces never call `game.nav.*` directly. They reach the stack only
through **domain action hooks** owned by the relevant subsystem —
`use_pause_actions()`, `use_run_actions()`, `use_upgrade_actions()`,
`use_settings_actions()`. Each named verb (`resume`, `restart`,
`quit_to_title`, `play`, `choose_upgrade`) is a one-line wrapper that calls
its subsystem method, which in turn calls `game.nav` if needed.

**`EntryContext` — how domain hooks know which entry they're inside.**
`<ScreenStackRenderer>` wraps each entry's screen body in
`PROVIDE(&EntryContext, &entry) { … }` so any descendant fiber can read the
enclosing `ScreenEntry` via `use_context(&EntryContext)`. Domain action
hooks read `entry_id` from this context at render time and pass it to
`game.nav.pop_screen(entry_id)` / `replace_screen(entry_id, …)`. No
component code ever threads `entry_id` through props. Because the React
runtime re-runs every fiber every frame (`src\react.cpp:363-370`) and
contexts are scoped to the fiber subtree, the captured id is always
current-frame; there is no stale-capture window. Same mechanism as the
existing `WorldContext` — no new primitive.

There is intentionally **no component-facing `use_navigation()` hook** that
exposes raw `push_screen/pop_screen/replace_screen`. Every UI mutation is
named in user terms; raw nav primitives are an implementation detail of
domain hooks. This rule kills two failure modes: a swelling 30-method
navigation god object, and the ambiguity of "is this verb a navigation
primitive or a domain verb." If the verb names *what the user is doing*
(resuming the game, restarting the run, choosing an upgrade) it's a domain
verb. The push/pop/replace primitives are not user verbs.

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

**UI payload pools.** The same `Pool<T,N>` / `Handle<T>` machinery also
backs UI-side payloads — `ChooseUpgradePayload`, future `DialogPayload`,
etc. — owned by the relevant subsystem. Sizing is driven by maximum
in-flight count for that screen kind, which for the level-up loop and
similar serial-by-design UX is exactly **one** (see Phase 8's
`Optional<ChooseUpgradePayload> pending_choice`). Payload pools are *never*
silently recycled; overflow blocks the requesting subsystem (the engine
defers the trigger until the slot is free) rather than dropping a payload
or stomping a live one.

The "grow by doubling" hint above applies only to world-actor pools where
contiguous expansion is safe between frames. Payload pools are fixed-size
and serial-by-design; growth is not a foundation concern for them.

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

**Foundations:** write path, screen stack (depth-1 form).

**Goal:** establish the action-hook plumbing using the first place UI
legitimately mutates state — a Title screen whose Play button starts the game.
In-world controls remain engine-internal. Land the stack in its final shape
at depth 1 so later phases add depth and lifecycle without changing the type.

**Scope:**
- [ ] Define the top-level `Game` object with subsystems (e.g. `game.nav`,
      and the existing player/combat code now grouped under `game.player`,
      `game.combat`). Subsystems expose public methods for the mutations they
      own.
- [ ] Add `ui_stack: std::vector<ScreenEntry>` on the world. `ScreenEntry`
      for this phase is `{ ScreenId id; uint32_t entry_id; }` — `entry_id`
      is a monotonic `u32` minted by `game.nav.push` (used for fiber-stable
      keying in Phase 6 and for lifecycle event addressing in Phase 7).
      `ScreenId` is `Title | Gameplay` for now. The stack always holds
      exactly one entry; depth grows in Phase 6.
- [ ] `game.nav` exposes `push_screen(id) -> entry_id`,
      `pop_screen(entry_id)`, `replace_screen(entry_id, id) -> entry_id` as
      the only stack writers. Identity-addressed per the UI contract:
      pop/replace reject if their `entry_id` argument is not the current
      top (logged warning, no mutation). `push_screen` mints and returns a
      monotonic `entry_id`. State validation also rejects duplicate-top
      pushes and pop on a depth-1 stack (until Phase 6 lifts the depth
      constraint).
- [ ] Domain action hook `use_run_actions()` exposes `play()` and
      `quit_to_title()` as one-line wrappers. Each captures the current
      top's `entry_id` at render time and passes it to `game.nav.replace_screen`.
      No component-facing `use_navigation()` hook exists.
- [ ] Plumb `Game&` through context the same way `WorldContext` carries the
      read view; action hooks pull it from context.
- [ ] Engine tick gates gameplay systems on `stack.back().id == Gameplay`:
      on `Title`, enemies and player input are frozen. Reading
      `stack.back()` here is the seed of the Phase 6 `SystemPhase` dispatch;
      keep the read in one helper so Phase 6 can swap it without churn.
- [ ] Add a Title screen component with a Play button that calls
      `use_run_actions().play()`.

**Acceptance:**
- Boot lands on Title; world is frozen, player input ignored.
- Clicking Play transitions to Gameplay; WASD and enemies behave as in Phase 2.
- In-world controls (movement, enemy ticking) are still driven by `engine_tick`
  reading `Input` — Phase 1's input path is unchanged, no hook touches them.
- `stack.size() == 1` is invariant through the phase; assert on it in debug.
- `entry_id` is unique across the lifetime of the run — verify by logging
  each minted id and confirming monotonicity across a Title → Gameplay →
  Title → Gameplay sequence.

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

### Phase 6 — Screen stack: depth, payloads, system-phase dispatch

**Foundations:** screen stack, engine-driven tick gating.

**Goal:** the engine owns navigation at arbitrary stack depth; UI is its
viewer. Payloads, fiber identity, and tick gating are all explicit.

**Scope:**
- [ ] Promote `ScreenId` to a full enum: `Title`, `Gameplay`, `Pause`.
- [ ] Extend `ScreenEntry` (the type already on the world from Phase 3) to:
      ```cpp
      struct ScreenEntry {
          ScreenId  id;
          uint32_t  entry_id;          // monotonic, minted by game.nav.push
          uint32_t  payload_handle;    // index into per-id payload pool; 0 = none
          uint32_t  payload_generation;// matched for safe resolve
          void    (*release_payload)(Game *);  // called on entry retirement; null = no payload
      };
      ```
      Transition fields (`phase`, `clock_ms`, `enter_ms`, `exit_ms`) are added
      in Phase 7, not here.
- [ ] **Payload typing rule (binding for the rest of the plan):** every
      screen that needs a payload declares its own payload pool, owned by
      the relevant subsystem (e.g. `game.upgrades` holds an
      `Optional<ChooseUpgradePayload> pending_choice;` — a single-slot
      pool, since the level-up loop fundamentally serializes choices; see
      Phase 8). The subsystem's `open_*()` method allocates the payload
      *and* returns the `release_payload` function pointer the engine will
      invoke when the entry is retired (Phase 7). `game.nav.push(id, ...)`
      calls into the owning subsystem, receives `{handle, generation,
      release_payload}`, and stores all three on the entry. The owning
      subsystem may refuse to honour the push if it can't allocate right
      now (`pending_choice` already full); `push` then becomes a logged
      no-op rather than corrupting state.

      Screens read the payload through a typed domain hook
      `use_screen_payload<T>(entry)`. Per-type behaviour is provided by
      compile-time `template <>` specialization declared in the header
      next to the subsystem that owns the pool — e.g. the specialization
      `template<> const ChooseUpgradePayload *use_screen_payload<ChooseUpgradePayload>(const ScreenEntry &);`
      lives next to `game.upgrades` and resolves against
      `game.upgrades.pending_choice`. No runtime registration table, no
      type-id map. The hook returns `nullptr` if the slot is dead (or has
      been recycled). Screens take only the entry; the handle+generation
      encoding never appears in component code. This reuses the actor-pool
      generation safety (see "Actor model") and eliminates `void*` or a
      central `std::variant`.
- [ ] **Fiber identity rule (binding):** `<ScreenStackRenderer>` MUST render
      each entry under `REACT_COMPONENT_BEGIN_KEY("Screen", entry.entry_id)`
      so two entries of the same `ScreenId` (nested same-kind modals; queued
      level-ups) get distinct Clay IDs and distinct hook state. Same-`ScreenId`
      siblings without unique keys collide on the parent-hashed Clay ID per
      `src/react.h:53-72` and `src/react.cpp:232-258`.
- [ ] Domain hooks land in this phase:
      - `use_screen_stack()` — projects the read-only stack from `WorldContext`.
      - `use_screen_payload<T>(entry)` — typed payload resolution; one
        line per call site, encoding lives in the hook.
      - `use_pause_actions()` — exposes `resume()` only. `quit_to_title()`
        is **not** here; it stays on `use_run_actions()` (one verb, one
        home). The Pause menu's "Quit to Title" button calls
        `use_run_actions().quit_to_title()` directly.
      Per the UI contract, no component-facing `use_navigation()` hook is
      added. The three nav primitives stay on `game.nav` and are reached
      only through domain hooks.
- [ ] **`SystemPhase` derivation (binding):** the world exposes a derived
      enum read by `engine_tick`:
      ```cpp
      enum class SystemPhase {
          GameplayActive,   // top is Gameplay
          GameplayPaused,   // Gameplay present in stack but not top
          NoGameplay,       // no Gameplay in stack (Title, GameOver)
      };
      SystemPhase nav_system_phase(const World &);
      ```
      Each subsystem declares which phases it runs in:
      - Combat / AI / physics / XP collection / fire system: `GameplayActive` only.
      - Damage-number fade: `GameplayActive | GameplayPaused`. (A killing
        blow that pushes `GameOver` mid-frame must let the resulting damage
        number finish its fade rather than freezing mid-air; the gameplay
        world remains visible behind GameOver, so its cosmetics keep
        animating until the run ends.)
      - Transition clocks (Phase 7): all phases — transitions never freeze.
      - Level-up trigger loop (Phase 8): `GameplayActive` only. The
        `GameplayActive` gate is what makes restart-drain safe: once any
        non-Gameplay screen is top (GameOver, an outgoing ChooseUpgrade
        in `exiting`), the trigger is suppressed and the drain can run to
        completion without re-pushing modals.
      - `nav_system_phase` rule for `exiting` entries: an `exiting`
        Gameplay entry counts as **absent** for top-of-stack purposes —
        once Gameplay is popped and entering `exiting`, the phase becomes
        `NoGameplay` (assuming nothing else gameplay-active is in the
        stack). This is why the trigger gate alone is sufficient; the
        moment restart-drain pops Gameplay, the trigger is locked out.
- [ ] **`engine_tick` dispatch ordering (binding):** within a single tick:
      ```
      // Phase A: nav mutations that are not phase-gated
      run transition_clock_advancer();// advances clocks, retires entries,
                                       // fires payload-release callbacks.
      run death_detector();            // HP <= 0 push GameOver. Runs in
                                       // any phase because death is real
                                       // regardless of which screen is up,
                                       // but the call is itself idempotent
                                       // (game.nav.push rejects if GameOver
                                       // already top).
      run restart_drain();             // if pending_restart and top exists
                                       // and top is not exiting:
                                       // game.nav.pop_screen(top.entry_id).
                                       // If ui_stack.empty() and pending_restart:
                                       // run ResetWorld, push boot screen,
                                       // clear pending_restart.

      // Phase B: read nav_system_phase ONCE, dispatch gated systems
      auto p = nav_system_phase(world);
      for each subsystem: if subsystem.runs_in(p) { subsystem.tick(); }
      // Phase B subsystems include: combat, AI, physics, XP collection,
      // fire system, damage-number fade, level_up_trigger.
      ```
      Phase-A writes land first; phase-B systems then see the post-mutation
      `nav_system_phase` for the rest of this tick. The level-up trigger
      lives in phase B (not A) so its `GameplayActive` declaration actually
      gates it — during restart-drain or after death, the trigger is
      simply not dispatched, which is the structural way to prevent the
      drain from re-pushing modals. No `pending_restart` guard inside the
      trigger body is needed; the dispatcher does the work.
- [ ] `<ScreenStackRenderer>` renders all entries bottom-to-top; lower
      entries stay mounted, painted behind upper ones (matters for the
      gameplay world remaining visible behind Pause).
- [ ] Implement `Title`, `Gameplay`, `Pause` screen components. Pause has
      Resume and Quit-to-Title actions through `use_pause_actions()`.
- [ ] ESC during Gameplay calls `use_pause_actions().pause()` (which calls
      `game.pause.enter()`, which calls `game.nav.push(Pause)`).
- [ ] Pop addressing: `use_pause_actions().resume()` captures the Pause
      entry's `entry_id` at render time and passes it to
      `game.nav.pop_screen(entry_id)`. If a same-frame engine push lands a
      new top above Pause, the captured id no longer matches `back()` and
      the resume becomes a logged no-op rather than popping the wrong
      screen.

**Acceptance:**
- ESC pushes Pause; combat/AI/physics frozen but the gameplay world is
  rendered behind the menu (verify by watching enemies stop moving while
  remaining drawn).
- ESC (or Resume) pops Pause; gameplay resumes seamlessly.
- Title → Gameplay flow works on boot.
- Stack depth of 2 works: Pause pushed over Gameplay.
- **Stack depth of 2 with the same `ScreenId` works**: push a debug second
  `Pause` over `Pause` (debug hotkey is fine) — both render distinctly,
  neither component logs a `react: hook count changed` error, and each
  carries independent local state. This verifies the `entry_id` keying.
- Gameplay pools are unchanged while Pause / Title is topmost (verifiable by
  watching live counts / positions).
- `nav_system_phase` returns the expected enum across boot, push Pause,
  push GameOver-stub: `NoGameplay → GameplayActive → GameplayPaused → NoGameplay`.

---

### Phase 7 — Lifecycle + transitions

**Foundations:** screen lifecycle.

**Goal:** screens have real lifecycle and animated transitions; events fire
in a deterministic order across stack changes.

**Ownership split — the rule for this phase:**

The engine owns the **transition state machine, the clock, and the duration
budget**. Screens own the **visual interpretation** of normalized progress.
This split is non-negotiable because lifecycle ordering, cancellation, and
tick-gating all depend on a single coordinator knowing when a screen is
"really entered" or "really gone." A screen that owned its own clock would
need a UI→engine feedback channel the architecture deliberately doesn't have
(and a buggy screen that never reports `exited` would wedge the stack).

What the engine owns:
- The discrete phase (`entering / entered / exiting / exited`).
- `clock_ms` on each `ScreenEntry`, advanced in `engine_tick`.
- The per-screen-type duration table (`enter_ms`, `exit_ms`) — a static map
  keyed by `ScreenId`, looked up at push time and cached on the entry.
- Phase transitions and lifecycle event emission.

What the screen owns:
- Easing curves, what property animates (opacity, scale, slide), how the
  backdrop renders, layout.
- All of this is pure render-time: the screen reads
  `float t = entry.clock_ms / entry.enter_ms;` (or the exit equivalent) and
  draws accordingly. No state. No engine coupling.

**Scope:**
- [ ] Add `transition_phase` and `clock_ms` to `ScreenEntry`. Add a static
      `screen_transition_table[ScreenId]` returning `{enter_ms, exit_ms}`;
      `game.nav.push` looks it up and caches `enter_ms` / `exit_ms` on the
      entry at construction time.
- [ ] Engine advances `clock_ms` each tick unconditionally — transition
      clocks run even when gameplay is paused, even when no `Gameplay` entry
      is in the stack. (`engine_tick` per Phase 6 gating still skips combat
      systems; phase-clock advance is in the always-run group, alongside the
      `SystemPhase`-derived dispatch from Finding 7.)
- [ ] Engine drives phase edges deterministically:
      - On `push`: new entry starts `entering`, `clock_ms = 0`. When
        `clock_ms >= enter_ms`, flip to `entered`.
      - On `pop`: top entry flips to `exiting`, `clock_ms = 0`. When
        `clock_ms >= exit_ms`, remove the entry and call the entry's
        registered payload-release callback (if any) so the owning
        subsystem can free its pool slot.
      - On reversed push (pop while still `entering`): flip directly to
        `exiting` and seed `clock_ms = exit_ms * (1 - progress)` so the
        visual continues from where it was rather than snapping.
      - On `replace(entry_id, new_id)`: atomic stack edit emitted as
        "pop old + push new" in a single nav write. Both entries coexist
        on the stack during the transition; the old one is `exiting` with
        `clock_ms = 0`, the new one is `entering` with `clock_ms = 0`. The
        new entry is appended above the old; when the old retires (its
        `exit_ms` elapses) it's removed and the new entry remains.
        Lifecycle event order: `on_blur(old) → on_exit(old) → on_enter(new)`,
        all on the same frame as the replace call, then `on_focus(new)`
        when the new entry reaches `entered`. This is what makes the
        Title↔Gameplay crossfade work — both screens render
        simultaneously for the duration of the transition.
- [ ] Add `use_screen_lifecycle({on_enter, on_exit, on_focus, on_blur})`.
      The hook subscribes to lifecycle events the engine emits per
      `EntryId` (the monotonic id from Finding 2). Event timing:
      - `on_enter` — fires the frame the entry is appended (entry → `entering`).
      - `on_focus` — fires when the entry's phase becomes `entered`.
      - `on_blur` — fires on the previously-top entry the frame a new entry
        is pushed above it.
      - `on_exit` — fires the frame the entry's phase becomes `exiting`,
        *before* the entry is removed, so the component is still mounted and
        can release payload handles cleanly.
      Order across a push: previous-top `on_blur` → new-top `on_enter` →
      (later, when enter clock completes) new-top `on_focus`. Reverse on pop.
- [ ] Implementation note: `use_screen_lifecycle` is a thin wrapper over
      `use_effect` whose `deps_hash` is derived from the entry's phase plus a
      per-edge event counter the engine maintains. This keeps the runtime
      primitive (`src/react.h:104`) load-bearing and the hook a naming
      convenience, not a parallel mechanism.
- [ ] Implement three reference visual transitions — each lives entirely in
      the screen component, reading `t` from its entry:
      - Modal scale-in / fade-in (`ChooseUpgrade`, `Pause`).
      - Backdrop opacity fade for any modal over `Gameplay`.
      - Crossfade for full-screen swaps (`Title` ↔ `Gameplay`).
- [ ] Audio side-effect test: dock a music-duck on `Pause`'s `on_enter`,
      undock on `on_exit`. Audio system may be a stub that logs.

**Explicit non-goal for this phase:** non-time-based transitions ("stay in
`entering` until an async fetch resolves"). If a future screen needs it, add
a per-screen `enter_complete_predicate` the engine polls instead of
comparing `clock_ms >= enter_ms`. Don't build that scaffolding now.

**Acceptance:**
- Smooth animated push and pop; no popping / flashing.
- Boot → Play → Pause → Resume → Quit produces a debug `printf` trail of
  lifecycle events whose order matches the spec
  (previous-top `on_blur` → new-top `on_enter` → new-top `on_focus` after
  `entered`, reversed on pop). Eyeball-verified during the demo.
- Reversed transition: push Pause, immediately pop while still `entering`.
  Engine flips to `exiting` with `clock_ms` seeded for visual continuity;
  final phase is `exited`; lifecycle trail shows `on_enter` then `on_exit`
  with no spurious `on_focus`.
- Two screens with different `enter_ms` values (e.g. Pause at 150 ms,
  ChooseUpgrade at 250 ms) both animate smoothly with their declared
  durations — proves duration is read per-entry, not hard-coded.

---

### Phase 8 — Level-up loop (integration test)

**Foundations:** all of the above, simultaneously.

**Goal:** prove the architecture under the canonical hard case — gameplay
interrupted by a modal that pauses the world, takes input, and applies a
mutation through the write lane.

**Scope:**
- [ ] Add player `level` and an XP threshold table.
- [ ] `game.upgrades` owns the payload as a single-slot
      `Optional<ChooseUpgradePayload> pending_choice` (with a `generation`
      counter bumped each time it's allocated, for handle safety). One slot,
      not eight: the player resolves one upgrade at a time, so concurrent
      offers are not a real requirement. This deletes the
      pool-overflow class of bug at the design layer.
- [ ] Engine-side level-up trigger (dispatched in `engine_tick` Phase B,
      runs in `GameplayActive` only — see Phase 6's
      `engine_tick` dispatch ordering):
      ```
      while (game.upgrades.pending_choice.empty()
             && player.xp >= threshold[player.level]) {
          auto handle = game.upgrades.open_choice();   // fills pending_choice
          game.nav.push(ChooseUpgrade, handle);
      }
      ```
      Two gates serialize back-to-back level-ups:
      (1) the dispatcher's `GameplayActive` check — once the freshly-pushed
      modal puts ChooseUpgrade on top, `nav_system_phase` becomes
      `GameplayPaused` and the trigger is not dispatched next tick;
      (2) the `pending_choice.empty()` body guard — within a single tick
      that crosses several thresholds, only one push lands.
      The dispatcher gate is also what makes restart-drain safe: an
      `exiting` Gameplay entry no longer counts as top, so the trigger is
      locked out for the whole drain.

      Cross several thresholds at once and only one modal is pushed; the
      next triggers as soon as the player dismisses the current
      one and `pending_choice` is released. No queue, no pool overflow.
      Both writes go through `game.nav.push` (the single serialization
      point) — the engine never reaches into `ui_stack` directly.
- [ ] Add `ChooseUpgrade` screen. It reads its payload via
      `use_screen_payload<ChooseUpgradePayload>(entry)` (the generic hook
      from Phase 6) — the hook resolves the handle against
      `game.upgrades.pending_choice`. The payload stays alive for the full
      `entering → entered → exiting` lifetime; it is only released at the
      end of `exiting` when the engine retires the entry, so the screen
      can render real option labels through the exit animation. The hook
      returns `nullptr` only if the slot was independently recycled
      (e.g. by a future subsystem with its own reuse policy); for
      `pending_choice` the slot stays valid until retirement. No
      handle/generation encoding appears in the component.
- [ ] `game.upgrades.open_choice()` allocates `pending_choice` *and*
      registers a payload-release callback on the entry it's about to be
      attached to. The callback is `game.upgrades.release_pending_choice`;
      `game.nav.push` stores it in the `ScreenEntry` per the Phase 7
      "engine drives phase edges" contract.
- [ ] Selection calls `use_upgrade_actions().choose(option_id)`. The hook
      reads `entry_id` from `EntryContext` and calls
      `game.upgrades.choose(entry_id, option_id)`, which validates the
      offer, applies the stat change, and calls
      `game.nav.pop_screen(entry_id)`. **`choose()` does not release
      `pending_choice`**; it just pops.
- [ ] **`pending_choice` release happens in exactly one place: the engine's
      entry-retirement step at the end of `exiting`** (Phase 7). The engine
      calls the registered payload-release callback when `clock_ms >= exit_ms`
      and removes the entry. Single release path covers selection,
      restart-drain (Phase 9), and death-during-modal identically — no
      branch in `choose()` or in `on_exit`. As a side benefit, the payload
      stays alive for the whole exit animation, so the screen can render
      its fading option cards from real data rather than placeholders.
- [ ] `ChooseUpgrade`'s `on_exit` (if it uses one) is for *screen-local*
      cleanup only (animation refs, effect listeners). It does not touch
      `pending_choice`.
- [ ] Upgrades affect real player stats (fire rate, damage, move speed,
      max HP, etc. — small fixed catalogue).
- [ ] Back-to-back level-ups: after choose-and-pop, the screen sits in
      `exiting` for `exit_ms` worth of frames with `pending_choice` still
      alive (so the exit animation renders real option labels). When the
      engine retires the entry it invokes the registered release callback,
      clearing `pending_choice`. The trigger loop on the next `engine_tick`
      sees `pending_choice.empty()` and pushes the next offer if XP still
      exceeds the next threshold. Exactly one `ChooseUpgrade` entry exists
      on the stack at any moment — the forced gap of `exit_ms` between
      modals is the visible "breath" between consecutive level-ups, not a
      bug.

**Acceptance:**
- Full loop plays: kill enemies → XP fills → modal appears, gameplay frozen
  → choose upgrade → modal closes, gameplay resumes with new stats.
- Modal scale-in / out animates cleanly (phase 7 still works).
- HUD HP / XP bars update smoothly throughout (phase 5 still works).
- Cross several level thresholds in a single frame (debug hotkey grants
  large XP at once). Exactly one modal is on the stack at a time; the
  next one appears `exit_ms` after dismissal (one `pending_choice`
  release → one trigger-loop firing). No level-up is dropped, no two
  `ChooseUpgrade` entries ever coexist on the stack.

---

### Phase 9 — Death, restart, title shell

**Foundations:** lifecycle correctness under cancellation; cleanup.

**Goal:** clean boot, clean death, clean restart, no leaks.

**Scope:**
- [ ] HP ≤ 0 → engine code (inside `engine_tick`) calls `game.nav.push(GameOver)`
      directly. Engine and UI both enter `game.nav` — the single serialization
      point — so a same-frame UI pop and engine death push are linearized by
      that one writer rather than racing.
- [ ] `GameOver` screen has Restart → calls `use_run_actions().restart()`, which
      wraps `game.run.restart()`.
- [ ] **`game.run.restart()` is the *only* path that resets a run** and it
      cooperates with the engine-owned phase machine instead of fighting it.
      A synchronous mid-render drain would never fire `on_exit`: per
      `src\react.cpp:301-326` and `:382-389`, lifecycle effects only run
      when a screen *renders* in the new phase, and they're flushed once at
      `react_end_frame`. Removing entries inside the click handler skips
      that re-render entirely and the unmount sweep would destroy the fiber
      with only the previously-active cleanup attached, never the
      `on_exit`. So:
      1. `game.run.restart()` sets `game.run.pending_restart = true` and
         returns. No stack mutation in the click handler.
      2. Each subsequent `engine_tick`, while `pending_restart` is true:
         - If `ui_stack` is non-empty and the top entry is not yet in
           `exiting`, call `game.nav.pop_screen(stack.back().entry_id)` —
           which kicks it into `exiting`, allowing the next React render
           to observe the phase change and fire `on_exit` through the
           normal lifecycle path.
         - If the top is already `exiting`, do nothing: the standard
           `clock_ms >= exit_ms` rule retires it.
         - The loop drains naturally over `sum(exit_ms)` frames (typically
           under 500 ms across a 2-3 deep stack).
      3. The frame after `ui_stack` becomes empty: run `ResetWorld` (clear
         `enemies`, `bullets`, `xp_gems`, `damage_numbers`; reset player
         stats; reset RNG seed) and call `game.nav.push(Gameplay)` (or
         `Title`, depending on flow choice). Clear `pending_restart`.
      Because every entry leaves through the same `pop_screen → exiting →
      remove` path normal pops use, every screen's `on_exit` runs and every
      payload handle is released by its owning subsystem. Plain
      `ui_stack.clear()` is banned: there is no second exit path.
- [ ] Add a Settings screen reachable from Pause (and from Title). Two
      settings: master volume, fullscreen toggle. UI calls
      `use_settings_actions().set_master_volume(v)` and `.set_fullscreen(b)`,
      which wrap public methods on `game.settings`. Persist nothing for now.
- [ ] Boot flow: stack starts as `[Title]` (constructed via the same
      `game.nav.push(Title)` path used everywhere else, so the boot entry
      gets a real `entry_id` and runs `on_enter` like any other).
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
  with the upgrade modal correctly torn down. After Restart, the
  `upgrade_payloads` pool reports `count == 0` and the next level-up
  allocates from slot 0 with a bumped generation — proves the lifecycle
  path released the handle rather than leaking it.
- Asserting `ui_stack.empty()` mid-restart (between the lifecycle drain and
  the boot push) succeeds — no stale entries.

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
