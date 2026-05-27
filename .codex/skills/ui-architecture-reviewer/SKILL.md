---
name: ui-architecture-reviewer
description: Use when reviewing this repo's UI architecture docs for drift, stale focus/navigation concepts, wrong screen-stack ownership, or examples that train readers toward toy/demo UI architecture instead of the canonical ClientUi/ScreenStack/UiScreen/ScreenView model.
---

# UI Architecture Reviewer

## Scope

Review the repo's active UI architecture docs:

- `docs/client-ui-focus-navigation-architecture.md` is canonical for intra-UI
  architecture.
- `docs/ui-focus-interaction-plan.md` covers focus, interaction, control, and
  visual-state primitives.
- `docs/ui-focus-stress-test.md` stress-tests the contracts with a complex
  Loadout screen.
- `docs/engine-ui-boundary-plan.md` covers game/engine ownership around the UI
  frame, not component internals.

## Canonical Stack

The docs should agree on this stack:

```text
Clay
  layout + render commands only

ui/focus
  focus scopes, focusable registry, spatial navigation from Clay rectangles

ui/primitives
  Focusable, Button, Toggle, TextInput, etc.

client/ui components
  dialogs, panels, rows, item tiles, menus

{Name}ScreenView
  root component tree for one screen

UiScreen
  retained stack/lifetime shell for one top-level surface

ScreenStack
  push/pop/replace, overlays, visible-screen ordering

ClientUi
  UI frame owner: focus runtime, action queue, input routing, ScreenStack

GameUiPipeline
  adapts game/platform state into one UI frame and renders Clay commands

Game tick
  polls input, ticks world, draws world, invokes GameUiPipeline, presents
```

## Correct Principles

- Clay owns layout and render commands, not app state, focus policy, or
  navigation ownership.
- Screen authors declare components, not sibling navigation edges.
- Focus navigation is implicit from laid-out focusable rectangles; explicit nav
  rules are rare boundary exceptions.
- `{Name}ScreenView` is the first component-style UI entry point.
- `{Name}Screen` is the retained stack object that calls `{Name}ScreenView`.
- `ScreenStack` stores retained `UiScreen` objects, not enums or route tables.
- Hooks expose generic UI services such as `use_screen_navigator()`.
- Props pass narrow child-specific behavior.
- UI writes become typed actions and are applied after layout.
- Game/world/stack mutation does not happen during Clay declaration.
- Engine/UI boundaries stay separate from intra-UI component architecture.

## Failure Modes To Flag

Flag these as high-priority drift:

- Manual sibling navigation in ordinary screen code, including required
  `left/right/up/down` edges or `GridStrategy`.
- "Teaching" examples that encode wrong architecture, such as enum/switch
  screen renderers, fake route tables, demo state, or shortcut-only examples.
- `ScreenRoute` objects that become callback adapters or know child-specific
  behavior such as `on_discard_and_pop`.
- Invented state concepts such as `OptionsState`, `use_local_state`, or extra
  containers that only hide ordinary screen-local hook state.
- Blurred lifetime/component boundaries, especially treating `{Name}ScreenView`
  as the retained stack object or putting component concerns into `UiScreen`.
- Un-grokkable C++ examples: `void *user`, opaque render-body helper names,
  function-pointer-shaped examples where a capturing lambda is clearer.
- Stack/game mutation during Clay declaration instead of post-layout action
  dispatch.
- Historical or draft-language in active docs: "previous", "current draft",
  "version A/B", "teaching version", or commentary about discarded designs.
- Narrative order that skips levels. Examples should climb:
  `Focusable -> Button -> Dialog -> ScreenView -> UiScreen -> ScreenStack ->
  ClientUi -> GameUiPipeline -> game tick`.
- Generic-sounding boundaries that are actually game-specific, or game-specific
  behavior pushed into generic `ui/` primitives.

## Review Workflow

1. Read `docs/client-ui-focus-navigation-architecture.md` first.
2. Review the other three active docs against it.
3. Search for common drift markers:

```sh
rg -n "focus-version-a|focus-version-b|ScreenRoute|GridStrategy|use_local_state|OptionsState|teaching version|previous|version A|version B" docs
```

4. Return only high-confidence findings. Each finding must include:
   - file and line;
   - the conflicting text or concept;
   - why it violates the canonical stack/principles;
   - the structural correction.

Do not edit files when acting as a reviewer.
