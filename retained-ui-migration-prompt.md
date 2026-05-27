# Retained UI Migration Prompt

Migrate this repo's UI from Clay-backed immediate/declarative layout to a truly
retained-mode UI runtime with flexbox layout, JSX-like C++ syntax, and
React-style composition conventions.

Start by reading the repo contract: `CLAUDE.md`, `src/CLAUDE.md`,
`src/client/ui/CLAUDE.md`, `src/react.h`, and any architecture docs that exist.
The root instructions mention `architecture.md`; if it is missing, document
that in the plan and create or restore an architecture doc early instead of
silently inventing new boundaries.

## Goal

- Remove Clay as the UI layout/runtime dependency.
- Replace it with an app-owned retained UI tree: stable node identity, keyed
  children, lifecycle/unmount behavior, event routing, focus, and deferred
  mutation semantics.
- Use a real flexbox layout model. Evaluate maintained C/C++-friendly layout
  options before hand-rolling full flexbox.
- Add a transpiler so UI files can use true JSX-like syntax in C++ source. Use
  `.cppx` and `.hx` extensions for authored files, generating normal C++ for
  the build.
- Bring component APIs closer to contemporary React conventions, using
  `$vercel-composition-patterns` as guidance: compound components,
  provider-owned state, explicit variants, children slots, and no boolean-prop
  sprawl.
- Keep the end state React-like: components should still compose through hooks,
  providers/context, keyed children, and predictable lifecycle semantics. The
  current `src/react.{h,cpp}` implementation is fair game to refactor or
  replace when doing so makes retained window state, component lifecycle, or
  authored `.cppx`/`.hx` code cleaner and more idiomatic. Preserve the
  React-like programming model, not the current implementation details.

## Constraints

- Preserve the repo's layer boundaries: `src/ui` remains generic and
  game-vocabulary-free; game-specific UI stays under `src/client/ui`; game
  rules remain in `src/game`.
- Keep C++20, SDL3, no exceptions in UI/runtime code, and no RTTI assumptions.
- Do not keep Clay semantics behind a renamed facade. The end state should not
  require Clay IDs, Clay layout calls, or Clay render command arrays.
- Do not import React, DOM, Next.js, or JavaScript runtime assumptions. Adapt
  the architecture patterns to native C++/SDL.
- Do not preserve current hook/runtime APIs solely for compatibility if they
  make the retained-mode window state model awkward. Keep public concepts
  React-like, clear, and idiomatic for the new runtime, then migrate callers
  deliberately.
- Mutations triggered during UI declaration/rendering must remain deferred and
  drained at the correct frame boundary.
- Preserve deterministic headless CLI testing through `tools/ui_cli.py`.

## Execution Plan

1. Create a durable migration plan in-repo, including current architecture
   findings, missing docs, selected flexbox strategy, transpiler design, risk
   list, and migration slices.
2. Open a draft PR immediately after the first plan/skeleton commit. Keep
   pushing focused commits frequently.
3. Build a thin retained UI runtime skeleton first: retained node tree, keyed
   identity, style model, flex layout adapter, renderer boundary, input/focus
   boundary, and lifecycle cleanup.
4. Add the `.cppx`/`.hx` transpiler as an independently tested tool with golden
   tests, CMake integration, deterministic generated output, and useful
   diagnostics with source line mapping.
5. Port primitives before screens: text, button, toggle, selectable/focusable
   containers, panels, scroll containers, and common visual state.
6. Port screens one coherent slice at a time: main menu, options, pause,
   in-game HUD, loadout, dialogs, and shared providers/hooks.
7. Keep each slice buildable and testable. After every meaningful slice, run the
   narrowest relevant tests, commit, push, and update the draft PR description.
8. Once parity is reached, remove Clay dependencies, Clay renderer glue,
   Clay-specific tests, and stale docs. Add guard tests or grep checks to
   prevent Clay from re-entering UI code.

## Verification

- Run `./build.sh --tests` before major handoffs.
- Keep focused tests green as modules move: runtime tests, focus tests,
  primitive tests, client UI tests, pipeline tests, shooter UI tests, and Python
  CLI smoke tests.
- Add new tests for retained lifecycle, keyed reconciliation, flex layout
  behavior, transpiler output, event/focus routing, and deferred mutation
  ordering.
- Use CLI screenshots or BMP captures for visual regressions when migrating
  screens.

## PR Cadence

- Commit and push after each complete slice, even if the full migration is not
  done.
- Keep the draft PR description current with completed slices, remaining work,
  verification results, and known risks.
- Prefer small, reviewable commits over one large rewrite.
