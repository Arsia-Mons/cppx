# Real Retained Reconciliation Goal

This PR replaces the current immediate retained UI authoring model with a real
React-style retained reconciler. This is a full architecture replacement, not a
partial migration, compatibility bridge, or syntax-only refactor.

The final app must author UI as returned element descriptions. Components do not
mutate `UiTree` directly. Components do not invoke child components while
building their own return value. The reconciler is the only code that invokes
component functions, owns hook fiber entry/exit, walks children, pushes provider
context, and commits host nodes into the retained tree.

## Final Component Contract

Components take one props object and return `UiElement`.

```cpp
UiElement ComponentName(const ComponentNameProps &props);
```

`children` is a prop, matching React's model.

```cpp
struct PanelProps {
    const char *key = nullptr;
    UiChildren children = {};
};

UiElement Panel(const PanelProps &props);
```

Parent components return descriptors, not rendered child results.

```cpp
return Panel({
    .key = "root",
    .children = Children({
        Component<WeaponTile>({ .key = "weapon-0", .index = 0 }),
        Text({ .value = "Loadout" }),
    }),
});
```

In that example, the parent does not call `WeaponTile`. It creates a
`ComponentElement` descriptor. The reconciler later enters the `WeaponTile`
fiber and invokes `WeaponTile(props)`.

## Non-Negotiables

- No final backwards compatibility layer for the old app authoring API.
- No mixed old/new subtrees in production app code.
- No child lambdas or render callbacks as the composition model.
- No immediate child component invocation during parent render.
- No `void Component(...)` app component contract.
- No `REACT_RETAINED_COMPONENT_BEGIN` or `REACT_RETAINED_COMPONENT_END` in
  authored app/client components.
- No direct `RetainedNodeScope` usage from app/client components.
- No direct `UiTree::begin_node` or `UiTree::end_node` outside reconciler
  internals and focused low-level tests.
- No `.cppx` output that lowers JSX-like syntax to nested immediate component
  calls.
- Hooks may only run while the reconciler is invoking a component.
- Providers are returned element boundaries with `props.children`, not scoped
  C++ blocks around already-invoked children.
- No required `NodeRole`-style behavior enum parallel to host kind.
- No accessibility or semantic role that drives layout, drawing, focusability,
  callback dispatch, or retained-node identity.
- No automation id used as a replacement for `key`.
- No button or scroll-container host kind. Button behavior and scrolling are
  props/component behavior over `Box`.
- No retained runtime abstraction that exists only because the old immediate API
  needed it. Every public concept must have explicit, load-bearing value in the
  returned-element reconciler model.

## What Stays

- `UiTree` remains the retained commit target.
- Yoga layout, retained focus, retained draw-list generation, SDL retained
  rendering, control-mailbox readback, and CLI smoke testing remain downstream
  of committed retained nodes.
- `ClientUi`, `ScreenStack`, and `UiPipeline` remain the shell/lifecycle owners,
  but screens return root `UiElement` values instead of declaring retained nodes
  by side effect.
- The hook storage concepts in `src/react.{h,cpp}` remain the state-storage
  baseline, but component invocation and fiber identity must be owned by the
  reconciler.

## Required Runtime Shape

Add an explicit element/reconciler layer with these concepts:

- `UiElement` tagged as empty, fragment, host, component, or provider.
- `UiChildren` as owned frame data, not a callback.
- `HostElement` for platform primitives only.
- `ComponentElement` with typed props storage and a typed render thunk.
- `ProviderElement` with a context pointer/value and `children`.
- Frame-owned arena or bounded storage for elements, child lists, copied props,
  strings, callbacks, and measurement data.
- A reconciler that walks `UiElement`, invokes component elements, pushes and
  pops provider context, and commits host elements into `UiTree`.

Component fiber identity and host node identity must be separated. A component
can return zero host nodes, one host node, many host nodes, a provider, a
fragment, or another component. The old `RetainedNodeScope` model fused
component entry with host node creation; the new model must not.

## Host Element Model

Host elements are the platform primitives of this runtime. App components and
compound components are built above them.

```cpp
enum class HostKind {
    Box,
    Text,
};
```

`HostKind` is the host tag. It is the equivalent of browser React's platform
tag. It drives the default platform behavior for layout, drawing, text
measurement, and input routing. Host props carry state, callbacks, styling,
identity, accessibility metadata, and children.

The retained runtime must not add a required `NodeRole`-style behavior enum in
parallel with `HostKind`. A node does not need both a host kind and a duplicate
role such as `Button`, `Text`, `Toggle`, or `Selectable`. Behavior that changes
per instance belongs in props; behavior that changes the platform primitive
belongs in `HostKind`; higher-level concepts belong in components.

The host/reconciler surface must stay minimal. Concepts that do not directly
carry layout, text measurement, drawing, input routing, callback dispatch,
automation readback, provider/context flow, hook identity, or owned lifetime do
not belong in the generic runtime. If a behavior can be expressed as a component
that returns `Box`/`Text` with configured props, it is a component rather than a
runtime primitive.

`Box` is the generic layout/container primitive. It replaces panel-like boxes,
focusable wrappers, control surfaces, and scroll containers. A `Box` can become
focusable or confirmable through interaction props and callbacks.

`Text` is the text primitive. It stores copied text and text style on the
committed host node and owns text measurement through layout.

Scrolling is not a host kind. It is overflow behavior on a bounded `Box`, the
same way browser layouts scroll a `div` with constrained size and
`overflow: auto` or `overflow: scroll`.

`Button`, `Toggle`, `Selectable`, `ScrollView`, tabs, menu items, weapon tiles,
and other domain or compound controls are components. They return host elements
and configure focusability, confirm callbacks, overflow, checked/selected state,
accessibility semantics, style, and children through props.

## Host Props

Host props are copied or retained into committed host nodes by the reconciler.
The committed node data is the source consumed by Yoga, focus, drawing,
automation readback, and callback dispatch.

```cpp
struct HostProps {
    const char *key = nullptr;
    Style style = {};
    VisualStyle visual = {};
    TextProps text = {};
    InteractionProps interaction = {};
    AutomationProps automation = {};
    AccessibilityProps accessibility = {};
    HostCallbacks callbacks = {};
    UiChildren children = {};
};
```

`key` is reconciliation identity among siblings. It is not a control id,
automation id, or visible label.

`style` maps to flex/layout input, including overflow and scroll offset for
bounded scrolling boxes. Flex layout computes the `Box`'s viewport rect; when
children exceed that rect, overflow style controls clipping and scrollability.
`visual` maps to retained draw-list input.

`text` is meaningful for `HostKind::Text`. Text strings and measurement data
must have owned lifetime through commit and layout. Measurement reads committed
host-node text/style, not borrowed component props.

`interaction` carries runtime state such as focusable, disabled, checked,
selected, modal, and initial focus. Focus routing reads interaction props and
layout boxes. It does not read accessibility role to decide whether a node is
focusable.

`automation` carries deterministic control readback data such as test id and
index/offset. Automation ids are for CLI/control-mailbox targeting and tests.
They do not participate in reconciliation identity.

`accessibility` carries optional semantic metadata:

```cpp
enum class SemanticRole {
    Auto,
    Button,
    Checkbox,
    Switch,
    Tab,
    Dialog,
};
```

`SemanticRole::Auto` derives semantics from `HostKind`. Any explicit semantic
role is an accessibility/readback override only. It must not drive layout,
drawing, focusability, callback dispatch, or retained-node identity.

`callbacks` stores copied callbacks such as `on_focus` and `on_confirm`. These
callbacks must still queue deferred UI mutations when they change app or screen
state, and they must still dispatch at the existing frame boundary after focus
and confirm resolution.

## Web React Mapping

The C++ retained runtime follows browser React's ownership model without
importing the DOM or JavaScript APIs.

- Browser host tags map to the minimal runtime host set: generic layout elements
  such as `div` map to `Box`, and text nodes map to `Text`.
- Browser controls such as `button`, checkbox inputs, tabs, and selectables map
  to components that return `Box`/`Text` host elements with interaction,
  callback, accessibility, and visual props.
- Browser overflow styles map to retained style props. Scrollable containers are
  bounded `Box` host nodes with overflow style, not a separate host kind.
- Browser props/attributes map to typed C++ prop structs.
- `children` maps to `UiChildren`, owned frame data passed as a prop.
- Function components map to `UiElement Component(const Props &props)`.
- Context providers map to `ProviderElement` boundaries with `props.children`.
- React render/reconcile maps to walking `UiElement` descriptions, invoking
  component/provider boundaries, and committing host nodes into `UiTree`.
- React commit maps to updating copied host-node props, callbacks, layout input,
  measurement input, automation metadata, and unmount cleanup in `UiTree`.
- React `key` maps to sibling reconciliation identity. Component fiber identity
  and host node identity remain separate.
- ARIA/accessibility role maps to optional `SemanticRole`; it is not a host
  behavior enum.

## Work Order

1. Introduce the element model and reconciler internals under the generic UI
   runtime. Keep this below app/client code.
2. Convert retained primitives to element factories that return `UiElement`.
   They must not mutate `UiTree` directly.
3. Move all `react_enter` and `react_leave` responsibility into the reconciler.
4. Implement host commit into `UiTree` from `HostKind` plus `HostProps`,
   including copied strings, callbacks, interaction props, automation metadata,
   text measurement, keyed host children, and unmount cleanup.
5. Rework providers so screen/frame providers are `ProviderElement` values with
   `children` in props.
6. Change `UiScreen`/screen view entrypoints so visible screens return root
   elements and `ClientUi` asks the reconciler to commit those roots.
7. Convert every app/client screen and component to the new returned-element
   contract in this PR.
8. Rewrite `tools/cppx_transpile.py` so JSX-like syntax lowers to element
   construction with `children` props.
9. Delete or make private the old immediate authoring API after conversion.
10. Add guard tests that prevent the old API from re-entering app/client code.

## Required Guard Tests

Add tests or static guards that fail after the refactor if these appear outside
explicitly allowed backend tests and private reconciler internals:

- `REACT_RETAINED_COMPONENT_BEGIN`
- `REACT_RETAINED_COMPONENT_END`
- direct `RetainedNodeScope`
- direct `UiTree::begin_node`
- direct `UiTree::begin_keyed_node`
- direct `UiTree::end_node`
- `.cppx` generated child lambdas for JSX children
- primitive child-lambda overloads in the app-facing component API
- `NodeRole` or any equivalent required behavior enum in the app-facing or
  generic host model
- draw, focus, or callback code that branches on accessibility/semantic role
- a button host kind in the generic host model
- a scroll host kind in the generic host model
- public retained runtime abstractions without explicit load-bearing value in the
  returned-element reconciler model

Low-level tests for `UiTree` itself may still call `begin_node` and `end_node`.
The guard must distinguish backend tests from app/client authoring.

## Required Semantic Tests

The PR is not complete without tests proving the real reconciler semantics:

- A parent returns a `ComponentElement`; the reconciler invokes the child.
- Child components are not invoked during parent element construction.
- Hook state survives keyed child reorder.
- Hook cleanup runs when a conditional component unmounts.
- Providers work as returned element boundaries and descendants can read
  context through hooks.
- `children` flows through props and can be composed by compound components.
- Repeated host children preserve retained node identity by key.
- `HostKind` plus host props is the only host behavior source of truth.
- Buttons, toggles, selectables, tabs, and scroll containers are components over
  `Box`/`Text`, not generic host kinds.
- Obsolete immediate-authoring helpers and abstractions that no longer carry
  load-bearing runtime value are removed or made private.
- Scrollable content is represented as a bounded `Box` with overflow style,
  not a distinct host node kind.
- Focus routing uses interaction props and layout boxes, not semantic role.
- Draw-list generation uses `HostKind` and visual/interaction props, not
  semantic role.
- Automation id/index readback works and remains independent from `key`.
- Confirm/focus callbacks survive commit and still queue deferred UI mutations at
  the existing frame boundary.
- Text and measurement data have owned lifetime through commit/layout, including
  when original component prop storage is no longer valid after commit.
- Retained focus, draw list generation, control-mailbox readback, and CLI smoke
  tests still observe the committed retained tree.

## Completion Criteria

This PR is done only when:

- all shooter sample screens use returned `UiElement` components;
- `.cppx` emits returned element construction with `children` props;
- no app/client component uses the old immediate retained API;
- old public authoring helpers are removed or made reconciler-private;
- guard tests enforce the new contract;
- semantic reconciler tests cover hooks, providers, children, keyed reorder,
  callbacks, unmount cleanup, host kind/props, semantic-role isolation,
  automation-id readback, text/measure lifetime, focus, draw, and CLI readback;
- `architecture.md` and root contributor guidance describe the new reconciler
  contract instead of the old slot-callback contract;
- `./build.sh --tests` passes.
