# React composition patterns, realized in this codebase

The `composition-patterns` skill is the generic React doctrine. This file is the bridge: each rule
→ how it's spelled in this retained-C++ repo, and where the model **diverges** from idiomatic
React. The single best worked example of all of these at once is `src/client/ui/screens/loadout/`.

## architecture-avoid-boolean-props

Mode/host-swapping booleans are the smell; explicit composition + enum variants replace them.
- **Here:** `loadout_screen.cppx` composes `LoadoutScreenFrame > LoadoutTitle/LoadoutTabs/LoadoutBody`
  as explicit children, and mounts a separate `<LoadoutConfirmDialog/>` sibling rather than a
  cascade of `is*` flags.
- **Sanctioned exception:** leaf host-*state* flags are fine — they're state, not mode. `disabled`
  and `default_focused` on `AppButtonProps` are wired through to the host; `selected` is a
  forward-compat placeholder `AppButton` currently ignores (live selection paint lives on
  `WeaponTile`/`EquipmentSlot`, which adapt `Button` directly). The *mode* is the `AppButtonVariant` enum.
- **Anti-pattern:** a host-swapping boolean like `bool interactive` that flips a component between a
  non-focusable `Box` and a focusable `Button` — one flag doubles the state space and couples
  element choice to focus/hover behavior. Split into two components (`Tag` vs `Chip`) or encode it
  in the variant; never a `bool interactive`/`clickable`/`selectable`. (See SKILL.md Recipe 2b.)

## architecture-compound-components

Sub-pieces compose under a parent and share state via context, not prop-drilling.
- **C++ idiom:** a `struct` with a `static` method, authored as `Parent.Child` in JSX:
  ```cpp
  struct LoadoutTabs { struct TabProps { /*…*/ }; static ::ui::UiElement Tab(const TabProps&); };
  ::ui::UiElement LoadoutTabs(const LoadoutTabsProps&);
  // <LoadoutTabs><LoadoutTabs.Tab label="Weapons" onSelect={...}/></LoadoutTabs>
  ```
- **Divergence:** no `const Composer = { Tab }` object-property assignment. Shared state is reached
  via the owning screen's `use_loadout()`/`use_server()`, not a Tabs-local context — the
  `on_select` closures (the deferred-write contract) live in the screen, not in `Tab`.

## patterns-children-over-render-props

Every container takes `::ui::UiChildren children = {}` (declared **last**) and renders
`{props.children}`. There are **no** `renderHeader`/`renderFooter` props in `src/client/ui`.
- **Divergence (the React-sanctioned "render prop for data/behavior" case):** when a child needs
  behavior rather than structure, this repo passes `std::function` callbacks (`on_press`,
  `on_compare_change`, `on_buy`), not render props returning UI.

## patterns-explicit-variants

A single component takes a closed `enum` variant and switches internally on host kind + paint;
none of that leaks as a caller knob.
- **Here:** `ScreenLayoutVariant { Menu, Game, Overlay, CenteredOverlay }` — `Menu`/`Game` emit a
  `Box`, `Overlay`/`CenteredOverlay` emit a modal `Dialog`. Same shape: `AppButtonVariant`,
  `PanelVariant`, `ScreenTitleVariant`.
- **Divergence:** React's rule leans toward separate named variant *components*
  (`ThreadComposer` vs `EditComposer`); this repo prefers **one component + an enum variant prop**
  when surfaces are "the same concept differing only by an app-approved bundle." Separate
  components are reserved for genuinely distinct *screens* (which are themselves components).

## state-context-interface (state / actions / meta)

A consumer reads a generic contract, decoupled from the implementation.
- **C++ convention:** a single **flat `Value` struct** returned by the consumer hook, mixing read
  fields (the "state") + `std::function` setters (the "actions"). The raw `use_state` pointers and
  the sink (the "meta"/storage) stay **private** in the provider `.cpp`. Examples: `LoadoutValue`,
  `AppValue`, `ServerValue` (which nests an `actions` struct).
- **Divergence:** there is no literal `{state, actions, meta}` object and no exported context type;
  the interface *is* the `Value` struct + the hook.
- **Consistency note:** `use_app`/`use_navigation` return flat shapes; `use_server` nests `.actions`.
  Pick one deliberately for a new hook rather than by accident.

## state-decouple-implementation

The provider is the only place that knows *how* state is stored/mutated.
- **What's hidden here:** (a) the `use_state` pointer storage, and (b) the **deferred-write sink** —
  UI never learns that mutations are queued-after-layout rather than synchronous.
  `make_loadout_context_value` is the only place that touches `internal::DeferredUiMutationSink` +
  `use_callback`/`callback_deps`; consumers just call the named setter.

## state-lift-state

State lives in a provider so siblings **outside the visual frame** can still read/act on it.
- **Here:** `LoadoutScreenView` renders `<LoadoutProvider><LoadoutScreenContent/></LoadoutProvider>`;
  the `use_state` slots live in `LoadoutProvider`, **not** as members on `LoadoutScreen`. Payoff:
  `LoadoutConfirmDialog` is a *sibling* of the frame yet reads `pending` via `use_loadout()`
  because both sit under the provider — the "provider boundary matters, not visual nesting" insight.
- App-wide lifts: `AppProvider`/`NavigationProvider`/`ServerProvider`. For **long-lived
  cross-screen** state, the POD is owned by `app::App` and the provider holds a raw pointer to it
  (mirrors `ServerProvider` → `ShooterGame`); a screen-local `use_state` can't outlive its screen.

## react19-no-forwardref — the biggest divergence

React 19 drops `forwardRef`, prefers `use()`. **In this repo there is no `forwardRef` and no
`ref`-forwarding at all** — components are plain free functions `UiElement Foo(const FooProps&)`.
- The analogue of `use(Context)` is the free function `use_context(&Ctx)`, wrapped by named hooks
  (`use_loadout`/`use_app`/`use_server`). Like React 19's `use()`, `use_context` **may** be called
  conditionally.
- **You never need `forwardRef`** because cross-component access is done by lifting state into a
  provider and reading it via a hook (state-lift-state) — that's *why* it's absent.
- `use_ref` exists but is a **per-fiber persistent pointer slot** (local storage, `useRef`-like),
  **not** a forwarded node handle.

## What this skill does NOT cover

Defer to the existing skills — don't re-explain them here:
- **`cppx-authoring`** — all `.cppx`/`.hx` syntax, JSX child rules, `<detail.Host>`, the
  line-oriented transpiler's limits (no JSX inside a C++ ternary/expression), the validate
  toolchain. The camelCase-JSX-attribute → snake_case-struct-field mapping (`controlId` →
  `control_id`, `onPress` → `on_press`) and the "`children` declared last to satisfy
  `-Werror=reorder-init-list`" rule are transpiler facts owned there.
- **`composition-patterns`** — the generic rules above, with full incorrect/correct examples.
