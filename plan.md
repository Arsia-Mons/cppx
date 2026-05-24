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

Why this shape, and not anything else, is documented in this repo's
conversation history; the short version is that it is the smallest playable
loop that forces all three bridge lanes (read / write / fast), the engine-owned
screen stack with real pause semantics, and a level-up modal that interrupts
gameplay — i.e. every primitive in the architecture, all at once, under
real actor-count load.

---

## Foundations under test

The whole point of building this is to perfect these foundations. Every phase
maps to one or more of:

- [ ] **Engine seam** — engine tick runs before `react_begin_frame`;
      simulation state is committed before UI reconciles.
- [ ] **Actor model** — typed pools with `Handle = {index, generation}`,
      cache-friendly, dangling-safe, O(1) spawn / resolve / destroy.
- [ ] **Read lane** — UI reads through typed read capabilities
      (`PlayerReadView`, `CombatReadView`, `ScreenStackView`, etc.), with
      declared dependency/version stamps for cheap subscription early-outs.
- [ ] **Write lane** — UI never mutates simulation state; it dispatches typed
      command capabilities that the engine/client boundary drains at the top of
      each tick.
- [ ] **Fast lane** — `use_bind(...)` registers a sidecar binding applied at
      the frame barrier without re-running the component that declared it.
- [ ] **Screen stack** — navigation state lives outside the simulation world;
      engine/frame orchestration reads it to gate systems; UI is a transparent
      renderer of it.
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

## Implementation control contract

This plan is intentionally easy to reward-hack if "done" means a visible demo
or checked boxes. That is not enough. Each phase exits only when it leaves
reproducible evidence that the foundation it claims to prove is actually true.

### Ownership and capability boundaries

Do not implement a generic `WorldContext`, `world()` getter, or selector API
that hands UI code a `World&`, `SimulationWorld&`, or `const World&`. That would
turn the world object into a context bag and let future work pass visible demos
while quietly making every subsystem globally reachable.

The planned ownership model is:

- `SimulationWorld` owns gameplay truth only: actor pools, deterministic
  gameplay facts, RNG seed/state for simulation, and simulation-local clocks.
- `ClientState` / `EngineState` owns frame orchestration, command queues,
  navigation stack, pending modal payloads, transition clocks, settings, and
  presentation-only async/cancellation state.
- The renderer receives explicit render inputs: a world-scene snapshot for the
  high-volume direct-SDL pass and Clay command arrays/binding targets for UI.
- UI receives only typed read capabilities and typed dispatch capabilities.
  Examples: `PlayerReadView`, `CombatReadView`, `PerfReadView`,
  `ScreenStackView`, `GameCommands`, and `NavigationCommands`.

Read hooks must be shaped around those capabilities, not around a generic
world value. Acceptable forms include narrow hooks such as
`use_player_value([](const PlayerReadView &player) { return player.hp(); })`,
or a generic hook whose first argument is a typed capability object with
declared dependencies. Unacceptable forms include
`use_game_value([](const World &w) { ... })`, `WorldContext`, `use_world()`,
or selectors that can silently touch unrelated pools.

Dispatch hooks must also be capability-specific. `use_game_commands()` may
dispatch deterministic simulation commands; `use_navigation_commands()` may
push/pop/replace screens. A single universal `use_dispatch()` into one global
command bus is out of bounds unless a phase evidence file proves the bus still
preserves separate ownership, ordering, replay, and compile-time access rules.

Every phase that touches these boundaries must include compile-fail or
boundary-test artifacts proving that UI/client read-lane code cannot include
mutable simulation internals, cannot call pool `resolve` helpers, cannot acquire
mutable simulation state, and cannot route unrelated navigation/debug/settings
work through a gameplay-only command sink.

### Phase exit evidence

Every phase must add or update a `docs/phase-evidence/phase-N.md` file before
the next phase starts. That file is part of the deliverable, not a note to write
later. It must include:

- The commit SHA being evaluated and the exact phase number.
- The exact build command, test command(s), sanitizer command(s), profiler or
  perf command(s), and scripted demo command(s) used for the phase.
- The artifact paths for logs, perf output, sanitizer output, screenshots or
  clips, lifecycle transcripts, replay transcripts, and any soak-test output.
- A requirement-by-requirement table mapping each phase acceptance bullet and
  each relevant foundation checkbox to the artifact that proves it.
- Any intentionally deferred risk, with the phase where it must be closed.

Manual inspection may supplement the evidence, but it never replaces scripted
commands, logs, counters, transcripts, or committed artifacts. A checked box
without an artifact link is treated as unchecked.

### Anti-reward-hack checks

Each foundation must have at least one positive behavior test and one negative
test that would fail if the implementation took the easiest visible shortcut:

- **Engine seam:** a frame-order transcript proves `engine_tick` completes
  before `react_begin_frame`; a negative test fails if UI reconciliation reads
  partially-mutated simulation state.
- **Actor model:** handle/generation tests prove stale handles resolve to null
  after destroy/reuse; a negative test fails on raw pointer escape from pools or
  on UI/client code including engine-internal pool resolution APIs.
- **Read lane:** selector counters prove unrelated pool version changes do not
  re-run a pure selector; a negative test fails if selectors can accept
  `World&` / `SimulationWorld&`, call a generic world getter, or silently read
  the whole world every frame.
- **Write lane:** command replay proves deterministic end state; a negative
  test proves dispatching from UI cannot mutate simulation state until the next
  engine tick drains commands. The proof must include a compile-time boundary
  check showing UI code cannot acquire mutable simulation state, use a universal
  command sink to bypass ownership, or call engine-only mutation helpers
  directly.
- **Fast lane:** reconciliation counters prove bound visuals update while the
  declaring subtree stays flat; a negative test fails if `use_bind` falls back
  to ordinary per-frame reconciliation outside the explicit disable-bindings
  comparison mode.
- **Screen stack:** an inspection test proves `ClientState` / navigation state
  is the owner and UI only renders it; a tick-gating transcript proves gameplay
  systems do not run or mutate gameplay pools while Pause / Title is on top; a
  negative test fails if screen state is owned by the simulation world,
  UI-only globals, component-local state, or a render-only pause that leaves
  gameplay simulation running.
- **Screen lifecycle:** lifecycle transcripts prove deterministic enter, focus,
  blur, exit ordering; cancellation tests fail on missed cleanup, duplicate
  callbacks, or reordered events.
- **Identity discipline:** Clay IDs, actor handles, and binding targets must be
  stable across reorder/remount cases; binding targets are opaque IDs / handles,
  not raw UI pointers or component addresses; negative tests fail on
  position-only identity where keys or handles are required and on pointer-based
  binding targets that survive only because the visible tree did not remount.

### Design checkpoints

Before implementing any new foundation, add a short design checkpoint to that
phase's evidence file. It must name the chosen API, owner, lifetime rules,
failure modes, cleanup behavior, negative tests, and why the design cannot
silently collapse into a visible-only shortcut.

This is mandatory for deferred choices such as typed read hooks,
typed command capabilities, `use_bind`, `ScreenStackRenderer`,
`use_screen_lifecycle`, `ResetSimulation`, async cancellation, and any
render-command or sidecar binding mechanism. The checkpoint does not need to be
long, but it must exist before the implementation that depends on it.

### Rendering boundary

Direct SDL drawing is allowed only for the world-scene pass: arena background,
player, enemies, bullets, gems, and other high-volume gameplay actors. The
React/Clay foundation remains responsible for HUD, menus, modals, screen stack
rendering, lifecycle-visible screens, damage numbers, binding-owned attributes,
and every ID proof.

Foundation-relevant visuals must expose stable Clay IDs or explicit binding
targets. A phase cannot claim the React-over-Clay foundation is proven by
rendering those visuals exclusively through direct SDL.

### Performance and soak protocol

Performance claims must be measured from a release build at a fixed 1280x720
window size with vsync configuration recorded, a deterministic seed, and a
scripted scenario that can be rerun. Before any phase claims a framerate or soak
result, commit `docs/phase-evidence/perf-target.md` naming the target CPU, GPU,
RAM, OS, compiler/toolchain, build preset, display mode, power mode, and vsync
configuration. Every perf artifact must cite that target file plus its build
type, commit SHA, scenario seed, run duration, entity counts, p50/p95/max frame
time, p95/max engine tick time, p95/max render time, and any dropped-frame
count. Changing the target machine or configuration invalidates earlier perf
claims unless the affected phase artifacts are rerun on the new target.

Phase-level 60fps claims require a continuous 60-second scripted run unless a
phase specifies a longer duration. Whole-plan soak requires a 30-minute
scripted run with RSS sampled at least once per minute; pass/fail is based on
no upward RSS trend beyond pool caps after warmup, no sanitizer findings, no
thread leaks, and no missed lifecycle or command-replay assertions.

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
    uint64_t version;         // bumped on any spawn/destroy/mutation
};

template <typename T>
struct Handle {
    uint32_t index;
    uint32_t generation;       // 0 == null
};

// Engine-internal only. Resolve returns nullptr if the slot is dead or
// generation mismatches; raw pointers must not cross the engine boundary.
template <typename T, uint32_t N>
T *resolve_mutable(Pool<T,N> &pool, Handle<T> h);

template <typename T, uint32_t N>
const T *resolve_readonly(const Pool<T,N> &pool, Handle<T> h);
```

**Why this is enough:**

- Contiguous arrays per actor type → cache-friendly iteration.
- Stable 64-bit handles → safe to hold across frames, across UI, across
  worker threads. A stale handle resolves to `nullptr` inside engine-owned
  scoped resolution, not a segfault.
- No allocation per spawn (pools pre-sized; can grow by doubling if needed).
- Per-pool `version` is the read-lane subscription primitive — selectors
  early-out when no pool they touched has changed since last frame.
- Loops over actors are written by hand: explicit, debuggable, no query DSL.

Raw actor pointers are a local implementation detail of engine systems. Any
cross-boundary API must carry handles, copied snapshots, typed read views,
or short-lived spans/proxies whose lifetime is bounded by the frame/evidence
test. UI/client read-lane code must not include the internal pool header that
exposes `resolve_mutable` / `resolve_readonly`.

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

**Goal:** prove the engine→snapshot/render→UI ordering and the actor-pool
primitive.

**Scope:**
- [ ] Add `src/engine/simulation_world.h` / `simulation_world.cpp` with a
      `SimulationWorld` struct for gameplay truth only.
- [ ] Add `src/engine/pool.h` with the `Pool<T,N>` template + `Handle<T>`.
- [ ] Add `Player` actor type (position, hp). Spawn one on world init.
- [ ] Add `src/engine/engine.h` / `engine.cpp` with
      `engine_tick(SimulationWorld&, float dt)`.
      For now: WASD updates player position directly inside tick (commands
      come in phase 4).
- [ ] Wire `engine_tick` into `main.cpp` immediately before
      the UI frame. Build a read-only frame snapshot after tick completion and
      pass that snapshot/read capability to `App()`; do not pass a mutable or
      const world pointer to UI.
- [ ] HUD: render `HP: %d` reading through a narrow `PlayerReadView` /
      `PlayerHudView` provider. Do not add `WorldContext`.
- [ ] Render the player as a colored circle through the chosen world-scene pass.
      If that pass is direct SDL, Phase 1 must still prove the React/Clay HUD
      reads the committed snapshot after `engine_tick` and uses stable Clay IDs.

**Acceptance:**
- WASD moves the player visibly.
- HUD shows the player's HP from the committed read capability.
- 60fps with vsync.
- No leaks on clean shutdown (Valgrind / AddressSanitizer clean).
- `Handle<Player>` resolves correctly; resolving a manually-destroyed handle
  returns `nullptr`.
- Phase evidence includes a frame-order transcript proving `engine_tick`
  finishes before `react_begin_frame`, a stable-ID inspection for the HUD, and
  a boundary artifact proving UI code cannot include mutable simulation/pool
  internals or obtain a generic world pointer.

---

### Phase 2 — Read lane: selector hooks

**Foundations:** read lane.

**Goal:** components subscribe to slices of world state, not the whole world.

**Scope:**
- [ ] Add typed read hooks returning values by copy, such as
      `use_player_value<T>(selector)`, `use_combat_value<T>(selector)`, or
      `use_game_value<T>(capability, selector)` where `capability` is a typed
      read view. Do not add a hook that passes `World&`, `SimulationWorld&`, or
      `auto&` world-shaped objects to selectors.
- [ ] Selectors receive only the typed read view they subscribe to
      (`const PlayerReadView&`, `const CombatReadView&`, etc.) and return a
      value (POD or small).
- [ ] Per-pool `version` bumped on every spawn, destroy, and mutation site.
      Mutation sites go through small inline helpers (`damage()`, `move_to()`)
      that bump the version; no naked field writes from outside the engine.
- [ ] Hook records the declared dependency versions for its typed view; if all
      unchanged from previous frame **and** the selector is marked pure, skip
      reinvocation and reuse the last value. (Cheap optimisation; correctness
      must not depend on it.)
- [ ] HUD HP text moves from the Phase 1 `PlayerHudView` pull to
      `use_player_value([](const PlayerReadView &player) { return player.hp(); })`.

**Acceptance:**
- HUD reads HP via selector; flipping HP in code updates the HUD next frame.
- Spawning 200 dummy moving entities does **not** cause the HP selector to
  re-run (verified via a counter / log).
- Selector outputs are deterministic across identical world states.
- Phase evidence includes selector invocation counters and a negative test where
  unrelated pool mutations do not re-run the HP selector.
- Phase evidence includes compile-fail or boundary-test artifacts proving UI
  selectors cannot accept `World&` / `SimulationWorld&`, cannot call `world()`,
  and cannot subscribe to every pool by default.

---

### Phase 3 — Many actors stress test

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
- 1,000 enemies ticking and rendering at ≥60fps on the target machine.
- Linear scaling (no sudden cliffs) — verify by sweeping 100 / 500 / 1000.
- Destroying enemies (via another hotkey) leaves the pool clean: live count
  drops correctly, generation bumps, resolve of old handles returns null.
- Phase evidence includes the 100 / 500 / 1000 sweep artifact from the
  performance protocol and stale-handle negative tests after destroy/reuse.

---

### Phase 4 — Write lane: commands

**Foundations:** write lane.

**Goal:** UI and engine talk only through a typed command buffer.

**Scope:**
- [ ] Add a `Command` variant (`std::variant` or tagged union) with the few
      commands needed so far: `MovePlayer{dir}`, `SpawnEnemies{count}`,
      `DamagePlayer{n}` for debug.
- [ ] Add command queues on `EngineState` / the client-engine boundary, not on
      `SimulationWorld`. Gameplay commands are drained at the **top** of
      `engine_tick`; navigation/presentation commands are drained by the frame
      orchestration path that owns `ClientState`.
- [ ] Add typed dispatch hooks returning narrow capabilities:
      `use_game_commands()` for deterministic simulation commands and
      `use_navigation_commands()` for screen-stack commands. Do not add a
      single global `use_dispatch()` unless its evidence proves the capability
      split remains enforceable.
- [ ] Replace direct mutations from the input path: input handler dispatches
      commands; engine applies them inside the tick.
- [ ] Add a compile-time mutation boundary: UI-facing code receives only a
      typed read view plus typed command capabilities, while non-const
      `SimulationWorld&` and mutation helpers are engine-owned. Grep/search
      checks may supplement this, but they cannot be the phase-exit proof.

**Acceptance:**
- Player movement works identically to phase 1 but is now command-driven.
- Replaying a recorded command stream against the same initial simulation state
  produces an identical end state (smoke test for determinism / replay).
- Mid-frame: dispatching a command does not take effect until the next tick.
- Phase evidence includes a compile-fail or boundary-test artifact proving UI
  code cannot acquire mutable simulation state, write world fields directly,
  call engine-only mutation helpers, or send navigation/debug/settings work
  through a gameplay-only command capability.

---

### Phase 5 — Bullets, collisions, enemy death

**Foundations:** actor model, read lane, write lane (integration test under
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
- [ ] HUD: add `Kills: N` and `XP: N` through typed read hooks
      (`PlayerReadView` / `CombatReadView`), not through a generic world
      pointer.

**Acceptance:**
- 100 enemies + 100 bullets in flight at ≥60fps.
- Player can clear an arena of enemies; XP accumulates.
- Enemy death always frees its pool slot; generation bumps verified.
- Stop-the-world test: spawn 500 enemies + 500 bullets, then trigger
  collisions; no hangs, no leaks.
- Phase evidence includes a deterministic combat transcript proving kills, XP,
  gem collection, pool frees, and generation bumps in one run.

---

### Phase 6 — Fast lane: bindings

**Foundations:** fast lane.

**Goal:** continuously-changing visuals update every frame without re-running
the React component that owns them.

**Scope:**
- [ ] Design and add `use_bind(target, setter, selector)`:
      registers a binding from a selector over a typed read capability to a
      target attribute (e.g. an HP-bar fill width, a world-space label
      position).
      The design checkpoint must specify target identity, unmount cleanup,
      transition behavior, failure behavior when the target is missing, and why
      the implementation cannot silently fall back to full reconciliation. The
      target identity must be an opaque stable ID / handle, never a raw UI
      pointer, component address, or lifetime-dependent Clay internals pointer.
- [ ] Implement binding application: after `react_end_frame()` and before
      `SDL_RenderPresent()`, walk active bindings and apply the latest
      selector value to the target. The exact mechanism may be a parallel
      "instance attribute" map keyed by Clay element ID and consumed during
      draw, or a Clay render-command patch pass — to be decided in this
      phase, **not in advance**.
- [ ] Convert HP bar to a binding-driven smooth fill.
- [ ] Add world-space floating damage numbers (DamageNumber pool) whose
      screen positions are bindings against the corresponding entity's
      world position + elapsed-time tween.
- [ ] Add a profile/log mode that proves the HUD subtree no longer
      re-reconciles each frame while the HP bar still updates visibly.

**Acceptance:**
- HP bar fills/empties smoothly even when HP itself only changes in
  discrete steps (binding interpolates).
- Damage numbers track moving enemies and fade out.
- Reconciliation counter for the HUD subtree is flat across frames.
- Disabling all bindings reverts the visual to per-frame full reconciliation
  with no visible difference except framerate / re-render counters.
- Phase evidence includes both binding-enabled and binding-disabled counter
  artifacts from the same deterministic HP/damage-number scenario, plus an
  unmount/remount negative test proving stale binding targets are invalidated
  instead of accidentally updating a recycled UI pointer.

---

### Phase 7 — Screen stack

**Foundations:** screen stack, engine-driven tick gating.

**Goal:** the engine owns navigation; UI is its viewer.

**Scope:**
- [ ] Add `ScreenId` enum: `Title`, `Gameplay`, `Pause`.
- [ ] Add `NavigationState` / `ClientState.ui_stack:
      std::vector<ScreenEntry>` outside `SimulationWorld`.
- [ ] Add navigation commands `PushScreen{id}`, `PopScreen{}`,
      `ReplaceScreen{id}` to `NavigationCommands`, not to the gameplay command
      queue.
- [ ] Frame orchestration reads the top of the stack to decide which systems run:
      `Gameplay` → run combat / AI / physics; `Pause` / `Title` → skip
      gameplay systems, keep cosmetic ones (e.g. damage-number fade is
      paused too).
- [ ] Add `<ScreenStackRenderer>` component that reads the stack via
      `use_screen_stack_value` / `ScreenStackView` and renders the screen
      component(s) for each entry. Lower entries stay mounted but visually
      layered behind upper ones.
- [ ] Implement `Title`, `Gameplay`, `Pause` screen components. Title has a
      Play button (dispatches `ReplaceScreen{Gameplay}`). Pause has Resume
      (`PopScreen`) and Quit.
- [ ] ESC during Gameplay dispatches `PushScreen{Pause}`.

**Acceptance:**
- ESC pushes Pause; gameplay tick is frozen but the last gameplay scene remains
  rendered behind the menu (verify by watching enemies stop moving).
- ESC (or Resume) pops Pause; gameplay resumes seamlessly.
- Title → Gameplay flow works on boot.
- Stack depth of 2+ works (e.g. Pause pushed over Gameplay).
- Phase evidence includes an inspection artifact proving `ClientState` /
  navigation state owns the stack and screen components only render the entries.
- Phase evidence includes a pause-gating transcript with gameplay system
  counters and gameplay pool versions unchanged while Pause / Title is topmost,
  while transition clocks and other allowed non-gameplay systems continue.

---

### Phase 8 — Lifecycle + transitions

**Foundations:** screen lifecycle.

**Goal:** screens have real lifecycle and animated transitions; events fire
in a deterministic order across stack changes.

**Scope:**
- [ ] Each `ScreenEntry` in `ClientState` carries a transition phase
      (`entering`, `entered`, `exiting`, `exited`) and a phase clock (e.g.
      200 ms enter / exit).
- [ ] Frame orchestration advances phase clocks each tick (and does so even
      when gameplay is paused; transitions never freeze).
- [ ] Add `use_screen_lifecycle({on_enter, on_exit, on_focus, on_blur})` hook.
      Order across a push: previous-top `on_blur` → new-top `on_enter` →
      new-top `on_focus` (after `entered`). Reverse on pop.
- [ ] Visual transitions: backdrop fade for modals, scale-in for modals,
      crossfade for screen swaps.
- [ ] Audio side-effect tests: dock a music-duck on Pause's `on_enter`,
      undock on `on_exit`. (Audio system may be a stub that logs.)

**Acceptance:**
- Smooth animated push and pop; no popping / flashing.
- Lifecycle event ordering verified by log (a deterministic transcript for a
  scripted sequence: Boot → Play → Pause → Resume → Quit).
- Reversing a transition mid-flight works (push Pause, immediately pop while
  still `entering` — ends `exited` correctly).
- Phase evidence includes negative transcripts for cancellation, duplicate
  callbacks, and reversed mid-flight transitions.

---

### Phase 9 — Level-up loop (integration test)

**Foundations:** all of the above, simultaneously.

**Goal:** prove the architecture under the canonical hard case — gameplay
interrupted by a modal that pauses simulation, takes input, and applies a
mutation through the write lane.

**Scope:**
- [ ] Add player `level` and an XP threshold table.
- [ ] When `player.xp >= threshold[level]`, simulation emits a deterministic
      level-up event with 3 randomly-selected upgrade options. Frame
      orchestration converts that event into
      `NavigationCommands::PushScreen{ChooseUpgrade, payload}`; the payload is
      owned by `ClientState` / navigation state, not by `SimulationWorld`.
- [ ] Add `ChooseUpgrade` screen: renders the 3 options, selects one by
      keyboard or click, dispatches `GameCommands::ApplyUpgrade{id}` +
      `NavigationCommands::PopScreen`.
- [ ] Upgrades affect real player stats (fire rate, damage, move speed,
      max HP, etc. — small fixed catalogue).
- [ ] Multiple level-ups in quick succession queue correctly: pop → next
      level threshold met → push again immediately.

**Acceptance:**
- Full loop plays: kill enemies → XP fills → modal appears, gameplay frozen
  → choose upgrade → modal closes, gameplay resumes with new stats.
- Modal scale-in / out animates cleanly (phase 8 still works).
- HUD HP / XP bars update via bindings throughout (phase 6 still works).
- Triggering a level-up while another modal is animating in/out behaves
  deterministically (queued, not dropped, not interleaved).
- Phase evidence includes a ten-level-up scripted transcript with queued
  modal handling and command-only upgrade application.
- Phase evidence proves the queued modal payloads live in navigation/client
  state while upgrade application remains a deterministic gameplay command.

---

### Phase 10 — Death, restart, title shell

**Foundations:** lifecycle correctness under cancellation; cleanup.

**Goal:** clean boot, clean death, clean restart, no leaks.

**Scope:**
- [ ] HP ≤ 0 → simulation emits a death event; frame orchestration dispatches
      `NavigationCommands::PushScreen{GameOver}`.
- [ ] `GameOver` screen has Restart → dispatches
      `GameCommands::ResetSimulation` + `NavigationCommands::ReplaceScreen{Gameplay}`.
- [ ] `ResetSimulation` command: clear all simulation pools and reset player.
      Navigation reset is a separate `NavigationCommands` operation so
      `SimulationWorld` never owns the stack.
- [ ] Add Settings screen reachable from Pause (and from Title). Two
      settings: master volume, fullscreen toggle. Persist nothing for now.
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
  bound listeners (AddressSanitizer + thread sanitizer clean).
- Dying with a level-up modal already on the stack resolves to GameOver
  with the upgrade modal correctly torn down.
- Phase evidence includes reset/cancellation artifacts proving no stale actors,
  UI fibers, bindings, async fetches, or lifecycle listeners survive restart.

---

## Definition of done (whole plan)

All ten phases meet acceptance, and:

- [ ] All "Foundations under test" checkboxes ticked.
- [ ] No phase required a hack that violated the architecture rules
      (UI never mutates simulation state; UI never receives generic world
      access; simulation world never owns navigation/command/presentation
      state; simulation world never holds raw UI pointers; bindings never need
      a re-render to update; screen lifecycle is deterministic).
- [ ] Profiling: 1,000 enemies + 200 bullets + HUD bindings sustains ≥60fps
      on the target machine.
- [ ] Memory: 30-minute soak test shows no growth in RSS beyond pool caps.
- [ ] The level-up loop in Phase 9 plays cleanly back-to-back ten times
      without a single frame of jank or a missed lifecycle event.

When this is true, the foundation is real and the next game built on it does
not need to revisit any of these decisions.
