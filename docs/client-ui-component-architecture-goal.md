# Client UI Component Architecture Goal

This document defines the intended end state for `src/client/ui` component
architecture. It is not a description of the current tree and it should not be
watered down to preserve current folder names, current "chrome" terminology, or
current convenience shortcuts.

The target is a modern shadcn-style application component system translated into
this C++20 / SDL retained UI runtime:

- low-level host/runtime primitives stay in `src/ui/*`;
- `src/client/ui/*` becomes a semantic app UI layer;
- screen code composes semantic components instead of configuring host widgets;
- variant props express design-system choices;
- files aspire to one exported component or one tightly scoped public concept.

`src/client/ui/CLAUDE.md` is the authoritative operational guidance for agents
working in this tree. This document gives the longer-form design rationale; when
there is tension, update this document to match the scoped guidance rather than
preserving older migration-era patterns.

## Core Boundary

`src/ui/*` is the primitive runtime layer. It may expose host details:

- raw `Style` and `VisualStyle`;
- focus, activation, key, text-input event structs;
- host IDs and `id_offset`;
- runtime interaction flags;
- low-level callbacks;
- layout dimensions and paint values.

`src/client/ui/*` is the semantic app layer. It should reduce those mechanics to
domain and product intent:

- `variant`, `size`, `tone`, `density`, `placement`, `selected`, `disabled`;
- `title`, `description`, `label`, `value`, `children`;
- `on_press`, `on_select`, `on_back`, `on_confirm`, `on_cancel`;
- screen/action identifiers that mean something to the app;
- app layout concepts such as shell, page, surface, panel, toolbar, tab, and
  action row.

Only the implementation of client components should touch host/runtime details.
Screen declarations should mostly consume semantic client components.

## Naming Direction

Avoid `chrome` as the default name for app UI structure. It is vague and
browser-era terminology. Prefer names that describe the app-level responsibility:

- `app_shell` for persistent around-content structure;
- `screen_layout` for screen/page framing components;
- `page_header`, `page_content`, `page_toolbar` for page structure;
- `surface`, `panel`, `toolbar`, `tab_list`, `action_row` for reusable app
  building blocks;
- `blocks` only for larger feature-composed chunks, not for primitive widgets.

Current `screen_chrome.*` should be treated as a transitional smell. Its useful
pieces should be split into semantically named modules such as screen layout,
surfaces, titles, actions, and buttons.

## File And Export Shape

Aspire to one exported component per file.

Good:

```text
src/client/ui/components/screen_layout/menu_screen_frame.hx
src/client/ui/components/screen_layout/menu_screen_frame.cppx
src/client/ui/components/surfaces/panel.hx
src/client/ui/components/surfaces/panel.cppx
src/client/ui/components/actions/menu_button.hx
src/client/ui/components/actions/menu_button.cppx
```

Acceptable exceptions:

- a provider implementation file may define its matching consumer hook so the
  `ReactContext` stays private with the provider, but public headers should stay
  role-shaped: provider headers in `providers/`, hook headers in `hooks/`;
- a small `index` / umbrella header that only re-exports component headers;
- a private implementation helper file with no public component API;
- a tightly coupled compound component namespace where the exported public
  concept is one component family.

Avoid broad catch-all files that export unrelated components, tokens, style
builders, and helpers together.

## Semantic Component APIs

Client component props should read like product/UI intent, not runtime
configuration.

Preferred:

```cpp
struct AppButtonProps {
  const char *key = nullptr;
  const char *control_id = nullptr;
  AppButtonVariant variant = AppButtonVariant::Primary;
  AppButtonSize size = AppButtonSize::Md;
  bool disabled = false;
  bool selected = false;
  bool default_focused = false;
  const char *label = nullptr;
  std::function<void()> on_press = {};
  ::ui::UiChildren children = {};
};
```

Avoid in `src/client/ui` public props:

```cpp
struct LeakyButtonProps {
  int id_offset = 0;
  bool autofocus = false;
  ::ui::Style style = {};
  ::ui::VisualStyle visual = {};
  std::function<void(const ::ui::ActivationEvent &)> on_activate = {};
};
```

Those leaky details are acceptable inside the component implementation when it
adapts to `src/ui/components::Button`.

## Variant Discipline

Variants are the main client-layer styling API. A client component should not ask
callers for explicit numbers or color values when those values are simply
selecting a known design-system treatment.

Good:

```cpp
<Panel variant={PanelVariant::Hero}>
<ScreenTitle variant={ScreenTitleVariant::Dialog} value="Paused" />
<AppButton variant={AppButtonVariant::Danger} size={AppButtonSize::Sm} />
```

Bad:

```cpp
<Box style={...width = Length::points(340)} visual={fill_visual({18, 27, 32, 245})}>
<Text visual={text_visual({236, 246, 242, 255}, 28)} />
<Button style={...height = Length::points(34)} />
```

If the caller chooses from a fixed set of app-approved appearances, that choice
is a variant. If the caller passes a number or color because the component has no
semantic API yet, the component API is incomplete.

Escape hatches should be rare, visibly named, and kept out of ordinary screen
authoring paths. If a low-level override is needed, prefer an explicit
implementation-only adapter or a clearly marked advanced prop over making
`style` and `visual` normal client component props.

## Composition Over Boolean Modes

Do not grow broad client components by adding mode booleans. Prefer explicit
variant components, compound parts, or composition.

Avoid:

```cpp
<LoadoutPanel show_tabs={true} show_details={true} compact={false} />
```

Prefer:

```cpp
<LoadoutProvider>
  <LoadoutScreenFrame>
    <LoadoutTabs />
    <LoadoutBody>
      <LoadoutWeaponGrid />
      <LoadoutDetails />
    </LoadoutBody>
  </LoadoutScreenFrame>
</LoadoutProvider>
```

The goal is not to remove domain logic from client components. Domain-specific
components are allowed. The goal is to make their APIs semantic and composable.

## Folder Hierarchy Target

The exact folder names may evolve, but the hierarchy must communicate the
boundary between primitives, semantic shared client components, and screen-local
feature code.

Target shape:

```text
src/ui/components/                    # low-level primitives; host/runtime API ok

src/client/ui/app_shell/              # app-level shell/providers/navigation frame
src/client/ui/components/             # shared semantic app components
  actions/
  layout/
  surfaces/
  text/
  navigation/

src/client/ui/screens/<screen>/       # screen feature module
  <screen>_screen.*
  <screen>_state.*                    # provider + hooks for screen-local state
  components/                         # one semantic component per file
  hooks/                              # screen-local hooks, only when needed
  providers/                          # screen-local providers, only when needed
  lib/                                # feature-local helpers, only when needed
```

This target is not dogma about the names above. It is a separation rule:

- shared semantic app components do not live in screen files;
- screen-local blocks do not masquerade as generic app components;
- caller-specific navigation hooks do not live in destination screen modules;
- navigation is composed at the calling screen with `use_navigation()`;
- tokens/style recipes do not share a file with many unrelated component exports.

Co-location wins by default. Bubble up only when there is a real second
consumer: reusable components go to `client/ui/components/`, reusable hooks go
to `client/ui/hooks/`, and reusable providers go to `client/ui/providers/`.

## Screen Code End State

Screen declarations should be mostly semantic composition. Treat screens as
components: do not split `Screen` and `View` just to make the wrapper look thin.
Add a separate view component only when it is reused or independently meaningful.

Good:

```cpp
return <MenuScreenFrame>
  <HeroPanel>
    <ScreenTitle variant={ScreenTitleVariant::Hero} value="Reference Shooter" />
    <ScreenSubtitle value="SDL3 / retained UI flow" />
    <MenuButton controlId="StartMatchButton" onPress={start_match}>
      Start Match
    </MenuButton>
  </HeroPanel>
</MenuScreenFrame>
```

Bad:

```cpp
return ::ui::components::elements::Button({
  .id = "StartMatchButton",
  .style = {...},
  .visual = theme::panel_visual(...),
  .on_activate = [](const ::ui::ActivationEvent &) { ... },
});
```

Raw primitive usage in screen code is acceptable only while implementing a new
semantic component or during a temporary migration slice. It should not be the
final authored shape.

## State And Actions

Provider/hook state is still the right mechanism, but the API should be a clear
contract rather than a grab bag of pointers and callbacks.

For screen-local state:

- the screen owns the provider;
- descendants read through named hooks;
- descendants write through named setter/capability hooks;
- host-safe deferred mutation remains an implementation detail;
- screen components do not receive or forward context bags.

For navigation and cross-cutting capabilities:

- app-shell capabilities are consumed through `use_app()`;
- screen-stack capabilities are consumed through `use_navigation()`;
- shooter/domain capabilities are consumed through `use_server()`;
- destination screen modules export screens/components or factories, not
  caller-specific hooks such as `use_push_options_screen()`;
- avoid `*_actions.{h,cpp}` files; model reusable behavior as hooks, providers,
  components, or private `lib/` helpers based on its role and scope.

## Review Standard

A migration slice is not done if it merely renames files while preserving the old
API shape. It must move the code toward the semantic boundary.

Review questions:

- Does this client component hide `src/ui` host details from screen authors?
- Does it expose variants instead of raw dimensions, colors, or paint structs?
- Is each public file centered on one component or one cohesive public concept?
- Are broad files like `screen_chrome` being split rather than rebranded?
- Does screen code compose semantic components instead of configuring primitives?
- Are domain hooks and state providers named around the feature contract?
- Are boolean modes avoided unless they represent true binary state such as
  `disabled`, `selected`, or `checked`?

If the answer to any of these is no, the slice should either keep going or mark
the remaining drift explicitly.
