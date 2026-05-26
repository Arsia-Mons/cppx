# UI source architecture implementation plan

## Objective

Implement the active `docs/` UI architecture in `src/`.

This is not a demo polish pass. The end result is the complete documented UI
stack in source:

- the focus system;
- the interaction and primitive control system;
- retained screen stack ownership;
- `ClientUi` frame ownership;
- post-layout UI write draining;
- a runtime CLI/control harness that can drive input and capture visual proof;
- a real shooter-style example that proves the architecture under pressure.

The shooter example is a first-class deliverable. It must use the same hooks,
focus runtime, primitives, retained screens, `ClientUi`, and frame pipeline that
the docs prescribe.

## Source of Truth

Implement from the active architecture docs:

- `docs/client-ui-focus-navigation-architecture.md`
- `docs/ui-focus-interaction-plan.md`
- `docs/ui-focus-stress-test.md`
- `docs/engine-ui-boundary-plan.md`

The docs are the contract. If source and docs disagree, change source to match
the docs unless implementation proves a doc requirement impossible or wrong. If a
doc must change, update the doc and the code in the same correction pass.

## Authoritative Repo UI Skill

Build and maintain a repo-local UI skill under `.codex/skills/` as part of this
implementation. The skill is a first-class deliverable for this work, not a
nice-to-have summary after the fact.

The skill must make an agent expert in how this UI works from the game loop down
to the primitives. It should encode the current React-style hooks boundary,
game/UI pipeline, `ClientUi` frame order, retained screen stack, screen
component conventions, hook/provider data and write patterns, focus/runtime
invariants, primitive rules, shooter example constraints, verification gates,
and recurring failure modes.

Keep correcting, editing, and tightening the skill while implementing the plan.
When a hiccup exposes a better rule, a misleading instruction, a missing
boundary, or a reviewer finding that future agents must not repeat, update the
repo-local skill in the same correction pass as the code/docs change. Do not let
the skill drift behind the source or the active docs.

This skill does not replace the architecture docs. The docs remain the
architecture contract; the `.codex/skills/` UI skill is the authoritative
working procedure for applying that contract correctly in this repository.

## Runtime CLI and E2E Proof Harness

Build a repo-local CLI/control harness as a first-class deliverable for this
plan. The harness must let tests and agents interoperate with the running game
instead of relying only on unit tests or manual inspection.

Use `../Silencer/clients/cli` for inspiration only. Its command shape, structured
RPC style, wait commands, input commands, and screenshot command are useful
precedent, but Silencer is not authoritative for this repository's protocol,
runtime ownership, command names, implementation language, or transport.

Required capabilities:

- launch or attach to the local `hello`/game process in a deterministic test
  mode;
- drive keyboard, mouse, and gamepad-style input through the same
  platform-to-`UiInputFrame` path used by normal runtime input;
- step frames and wait for frame counts, UI state, focus ids, screen stack
  state, or game state predicates without sleeping blindly;
- inspect enough runtime state to debug failures, including current screens,
  focused element, focus source, selected game/shooter data, and pending writes;
- resize the surface and move/click/drag/release the pointer at coordinates or
  inspectable element targets;
- capture screenshots to image files from the rendered frame;
- capture a short frame sequence or video artifact for interaction regressions;
- return machine-readable JSON for assertions and stable shell scripting;
- fail loudly on wrong state, timeouts, missing targets, or rejected commands.

E2E tests must use this harness for runtime proof. They should drive real input,
wait for observable state, assert the state transition, and capture screenshots
or short videos for meaningful UI flows. When visual behavior is part of the
deliverable, send the resulting image/video proof to the user by Discord DM.
Those artifacts are evidence for review; they do not replace source-level tests
or architecture guardrail checks. The verification step must also open and
inspect representative captured frames. Non-empty screenshots, plausible
component layout numbers, or passing state assertions are not enough if the
rendered result looks wrong.

## Current Source Baseline

The current source tree is a React/Clay runtime sample:

- `src/react.h` and `src/react.cpp` provide component identity, hooks,
  providers, effects, refs, and shutdown cleanup.
- `src/ui/components/` contains demo components.
- `src/ui/providers/` contains input and theme providers.
- `src/main.cpp` owns the SDL/Clay loop and directly renders `App(&input)`.
- `tests/react_runtime_tests.cpp` covers hook/provider/runtime behavior.

That runtime is the foundation. Keep it React-style and extend it into the
documented UI architecture instead of replacing it with a separate framework.

## Non-Negotiables

- This is React-style architecture. Use hooks.
- Hooks return values and functions.
- Local UI state stays in hook state inside the component tree.
- Do not introduce MVC.
- Do not introduce view models.
- Do not introduce route-table renderers.
- Do not pass screen-wide state bundles through component trees.
- Do not prop drill when a hook is the right boundary.
- Clay owns layout and render commands only.
- Screen authors declare components, not sibling navigation edges.
- Directional navigation is derived from harvested Clay rectangles.
- Stack and game writes are requested through hook-returned functions and are
  applied after Clay declaration.

## Required Source Shape

The implementation should land these layers in source. Exact filenames may vary,
but the ownership boundaries must stay intact.

```text
src/react.*
  React-style component, hook, provider, effect, and ref runtime

src/ui/focus/
  generic focus scopes, focusable registration, layout harvest, spatial nav

src/ui/primitives/
  Focusable, Button, Toggle, Selectable, TextInput/Stepper as needed

src/client/ui/
  screen components, UiScreen, ScreenStack, ScreenNavigator hook, ClientUi

src/game/ui/
  GameUiPipeline, platform/game input adaptation, presentation providers

src/shooter/
  shooter sample state, presentation builders, and UI hook adapters
```

The folder names are less important than the dependency direction:

```text
shooter/game code
  -> game/ui presentation and write adapters
  -> client/ui hooks, screens, ClientUi
  -> ui/primitives
  -> ui/focus
  -> Clay
```

`ui/` must not know shooter rules. Shooter code must not own the generic focus
runtime, primitive visual state, or retained screen stack.

## Workflow

Use an implementation/review loop for this plan. Do not wait until the end of
the whole architecture build to review it.

The outer loop is:

```text
implement one coherent slice
commit that slice
run the review/address loop until clean
move to the next slice
```

The review/address loop is:

```text
read-only reviewer pass
address every high-confidence finding
commit the fixes
repeat review/address/commit until the reviewer is clean
```

Rules:

- This document is an instruction contract, not a progress tracker. Do not mark
  items complete here as the work proceeds unless the plan itself is wrong and
  needs a real correction.
- Each implementation slice should leave the repo in a coherent state with tests
  or focused verification for that slice.
- Commit the implementation slice before asking for review, so the reviewer can
  inspect a stable diff. The commit message should be detailed enough to explain
  what changed, what docs requirement it satisfies, and what verification was
  run.
- Reviewers are read-only. They report findings; they do not edit files.
- Address every reviewer finding with either a code/doc fix or a clear written
  reason the finding is not valid.
- Commit reviewer fixes separately from the original implementation slice. The
  fix commit message should say what the reviewer found and how it was
  addressed.
- Do not call a phase complete while its reviewer still has blocking findings.
- Do not batch unrelated phases into one giant review if smaller slices can be
  reviewed cleanly.
- Preserve user or parallel-agent work in the tree. Do not revert unrelated
  changes to make a slice look clean.

## Phase 1: Source Gap Audit

Before implementation, produce a short gap audit against the four active docs.

The audit must identify:

- every documented layer missing from `src/`;
- every current demo shortcut that conflicts with the target architecture;
- which current runtime pieces stay as foundations;
- the first vertical slice that will prove the new path.

This is a working audit, not a new architecture proposal. Do not rewrite the
docs into a smaller target.

Also create or update the repo-local `.codex/skills/` UI skill with the
implementation rules learned from this audit. The skill should capture the first
vertical slice, the existing demo shortcuts that must not become architecture,
and the React-style hook/provider mechanics that remain the foundation.

Verification:

```sh
rg -n "ScreenRoute|GridStrategy|use_local_state|OptionsState|view model|MVC" docs src tests
git diff --check
```

## Phase 2: Focus Runtime

Add the generic focus runtime under `src/ui/focus/`.

Required behavior:

- focus scopes with stable ids;
- modal focus scopes;
- initial focus requests;
- focusable registration during component declaration;
- bounded storage with explicit overflow diagnostics;
- layout harvest after `Clay_EndLayout()` through `Clay_GetElementData()`;
- frame N directional navigation from frame N-1 harvested rectangles;
- disabled controls skipped for focus movement and confirm;
- focus source tracking for keyboard, gamepad, mouse, touch, and programmatic
  focus;
- spatial resolver with stable tie-breaks;
- rare local boundary rules such as stop, wrap, or explicit target.

Required tests:

- stacked buttons navigate up/down from harvested rectangles;
- grid cells navigate from harvested geometry, not row/column neighbor tables;
- disabled controls do not receive confirm;
- initial focus chooses the requested enabled element when present;
- modal scope traps navigation and parent focus resumes when it closes;
- focus state remains stable across resize/reflow after the next harvest.

## Phase 3: Interaction and Primitives

Add the generic interaction layer under `src/ui/primitives/`.

Required primitives:

- `Focusable`;
- `Button`;
- `Toggle`;
- `Selectable`;
- at least one richer focusable tile primitive used by the shooter example;
- stepper or numeric control if the shooter loadout flow needs quantity/count
  editing.

Required behavior:

- keyboard/gamepad confirm and cancel;
- pointer hover, press, drag-off cancellation, release-to-confirm;
- focus-visible behavior by input source;
- caller-owned control state;
- visual state derived from focus state plus control state;
- frame-local callback functions;
- no primitive-owned game state;
- no primitive-owned navigation graph.

Required tests:

- button confirms once on keyboard/gamepad confirm;
- pointer press outside or drag-off does not confirm;
- pointer release on the same target confirms once;
- toggles call hook-returned setter functions and do not own shared state;
- visual state changes only through focus/control state.

## Phase 4: Client UI Ownership

Add retained client UI ownership under `src/client/ui/`.

Required pieces:

- `UiScreen` retained stack entry;
- `{Name}ScreenView` component root convention;
- `ScreenStack`;
- overlay/opaque visible-screen ordering;
- `ScreenNavigator` hook;
- `ClientUi`;
- bounded UI write queue;
- post-layout drain point for queued writes.

Rules:

- `ScreenStack` stores retained `UiScreen` objects.
- `{Name}ScreenView` is only a component root.
- Screen components use hooks for services, values, functions, and local state.
- Children receive narrow props only for their own behavior.
- The game loop does not own the screen stack.
- Stack mutation is requested through hook-returned functions and applied after
  Clay declaration.

Required tests:

- push, pop, replace if implemented, and overlay ordering;
- visible screen ordering with opaque and overlay screens;
- `use_screen_navigator()` routes operations to the current retained screen
  entry;
- local hook state survives normal rerenders and resets on unmount;
- screen stack writes requested during UI declaration are not applied until the
  post-layout drain.

## Phase 5: Game/UI Frame Pipeline

Replace the direct sample `App(&input)` frame path with the documented boundary.

Required flow:

```text
platform input
  -> UiInputFrame
  -> GameUiPipeline
  -> ClientUi::begin_frame
  -> React/Clay declaration
  -> Clay_EndLayout
  -> ClientUi layout harvest and input dispatch
  -> render Clay commands
  -> drain UI writes
  -> present
```

Required behavior:

- platform code owns raw SDL polling;
- game/sample state owns shooter simulation;
- `GameUiPipeline` adapts game state into presentation providers;
- `ClientUi` owns focus, input routing, retained screens, and write draining;
- component code never calls Clay lifecycle functions;
- component code never ticks shooter simulation;
- shooter/game writes are applied after UI declaration.

Required tests:

- Clay lifecycle calls stay in the frame owner/pipeline;
- ordinary UI components do not read raw shooter state directly;
- UI writes requested by controls are drained after layout;
- runtime teardown still runs hook/effect cleanup and worker cleanup correctly.

## Phase 6: CLI and E2E Harness

Add the runtime control harness before treating the shooter example as proved.

Required pieces:

- a CLI entrypoint under this repository, with documented commands;
- a game-side control server or equivalent local control channel owned by the
  platform/runtime layer, not by UI components;
- deterministic test mode suitable for CI/local e2e runs;
- commands for `state`, `inspect`, `wait`, `step`, `resize`, key/button input,
  pointer input, screenshot capture, and short video/frame-sequence capture;
- artifact paths that are predictable enough for tests and agent handoff;
- focused tests for command parsing/protocol behavior and at least one
  end-to-end smoke test that launches the game, drives input, captures an image,
  and exits cleanly.

Rules:

- CLI-driven input must enter through the same input adaptation path as SDL
  input. Do not add a test-only shortcut that mutates focus, screen state, or
  shooter state behind the UI pipeline.
- The control protocol may be simpler than Silencer's and should fit this repo,
  but it must be structured, inspectable, and deterministic.
- Screenshots and videos must capture the actual rendered frame, not a synthetic
  component snapshot.
- E2E visual proof must include a rendered-frame inspection/readback step.
  Component layout numbers can pass while the final image is visually broken.
- E2E tests should prefer waits on real state over arbitrary sleeps.

Required tests:

- CLI can launch or attach to the game in test mode;
- key/gamepad-style input moves focus through the real focus runtime;
- pointer input can hover, press, drag off, release, and be observed through the
  real primitive/focus path;
- screenshot capture writes a non-empty image for the current rendered frame,
  and the verification process opens representative captures to check for
  unexpected visual output;
- a short video or frame-sequence capture can be produced for an interaction;
- CLI failures report actionable wrong-state or timeout errors.

## Phase 7: Shooter Example

Build a real shooter-style example that exercises the architecture end to end.

The example should include enough game-shaped state to make the UI meaningful:

- player health, armor, ammo, credits, and selected weapon;
- inventory/loadout items with disabled requirements;
- buy or loadout screen with tabs, grid, details panel, equipment slots, compare
  toggle, quantity/stepper where useful, and modal confirmation;
- pause screen and options screen;
- HUD overlay visible over gameplay;
- nested modal dialog for confirm/discard flows;
- local hook state for dialogs and scratch UI state;
- hooks returning values and functions for shooter presentation and writes.

Required screens/components:

- `HudScreenView` or HUD overlay component;
- `PauseScreenView`;
- `OptionsScreenView`;
- `LoadoutScreenView` or `BuyMenuScreenView`;
- retained `UiScreen` entries for top-level screens;
- item tile using `Focusable` directly;
- buttons, toggles, and dialog controls using primitives;
- details panel reading selected/preview data through hooks.

The shooter example must not be a separate hand-coded UI path. It is the proof
that the documented architecture works.

Required runtime scenarios:

- keyboard navigation through pause/options;
- gamepad navigation through loadout grid and equipment slots;
- mouse hover/press/drag-off/release on item tiles;
- disabled item explains itself and does not confirm;
- modal confirm traps focus and restores parent focus when closed;
- changing tabs or resizing reflows the grid and navigation follows rectangles;
- buying/equipping requests a real shooter-state write after layout;
- closing options returns to pause without prop-threaded state.

These scenarios must be covered by CLI-driven E2E tests where feasible. Capture
screenshots or short videos for the flows whose correctness is visual: focus
movement, modal trapping/restoration, grid reflow, disabled item explanation,
and buy/equip confirmation. Send representative proof artifacts to the user by
Discord DM during completion/review handoff. Before treating any visual flow as
verified, open at least one representative capture from that flow and inspect it
for unexpected layout, clipping, occlusion, focus, or rendering artifacts.

## Phase 8: Integration Hardening

Once the shooter example works, remove demo-only shortcuts that no longer match
the architecture.

Expected cleanup:

- direct `App(&input)` as the primary app path is replaced by `ClientUi`;
- demo counter/theme components either move behind the new screen model or are
  archived/deleted;
- global UI state is eliminated except for platform/service globals that have an
  explicit owner;
- broad context bags are not introduced;
- source comments describe the final architecture, not transitional intent.

## Verification Gates

Run these before every completion claim:

```sh
ctest --test-dir build --output-on-failure
git diff --check
rg -n "ScreenRoute|GridStrategy|use_local_state|OptionsState|view model|MVC" docs src tests
```

Add focused tests as implementation lands. Completion requires coverage for:

- React runtime identity and hook invariants;
- focus resolver behavior;
- modal scope behavior;
- pointer and confirm interaction behavior;
- primitive visual state derivation;
- screen stack ownership and visible ordering;
- post-layout write draining;
- CLI/control harness input, waits, screenshot capture, and video/frame capture;
- rendered screenshot/video visual readback for flows where appearance is part
  of the claim;
- shooter example integration scenarios.

Before every completion claim, inspect the repo-local `.codex/skills/` UI skill
and confirm it reflects the current source/docs/reviewer findings. If it is
stale or incomplete, update it before claiming the slice or full plan is done.

If a check is too broad and reports archive or legacy docs, scope it to active
docs and current source before treating it as a blocker.

## Reviewer Gate

Completion requires at least one read-only reviewer pass after implementation.

The reviewer should check:

- source matches the active docs;
- hooks are the UI boundary for values and functions;
- no MVC/view-model/state-bundle architecture was introduced;
- `ui/` remains generic and shooter-agnostic;
- `client/ui/` owns retained screens and `ClientUi`;
- shooter-specific code stays behind shooter hooks/adapters;
- navigation derives from Clay rectangles instead of sibling edge tables;
- writes drain after Clay declaration.
- CLI-driven tests use the real runtime input/pipeline path and capture actual
  rendered frames rather than synthetic UI snapshots.

Any high-confidence reviewer finding must be fixed before the plan is complete.

## Done Means

The implementation is complete only when `src/` demonstrates the full documented
stack end to end:

```text
SDL/platform input
  -> UiInputFrame
  -> GameUiPipeline
  -> ClientUi
  -> ScreenStack
  -> UiScreen
  -> {Name}ScreenView
  -> hooks returning values and functions
  -> primitives and Focusable
  -> Clay layout/render commands
  -> focus layout harvest
  -> post-layout UI write drain
  -> shooter state update
  -> CLI-driven E2E proof with screenshot/video artifacts
```

The shooter example must use that path for HUD, pause/options, and loadout/buy
flows. A working UI that bypasses this stack does not satisfy the plan. Runtime
proof must be reproducible through the CLI/control harness, with representative
images or videos captured and sent to the user when visual behavior is part of
the claim. Do not call visual proof complete until a representative screenshot
or video frame has actually been inspected for unexpected rendered output.
