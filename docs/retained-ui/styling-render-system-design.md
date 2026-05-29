# Retained UI: Styling + Render System Design

**Status:** Active design spec. **Supersedes** `docs/retained-ui/clay-parity-styling-render-system-design.md` (a Clay-parity draft, now retired). This is a **from-first-principles** design: there is **no old-pixel parity**, **no "zero fidelity loss" constraint**, and **no bit-for-bit golden** against any prior commit. Goldens are authored fresh against the new intended look. The retired Clay-parity draft is a parts donor only.

Target: C++20 / SDL3 / SDL3_ttf retained UI with a React-style hook runtime (`src/react.h`). No exceptions and no RTTI in `ui/`/runtime code; built `-Wall -Wextra`; fixed-size aggregates are house style; single-threaded UI; Yoga does layout.

---

## Design decisions (locked)

These are settled. Do not re-litigate; do not reintroduce anything in **Deferred / reserved** (Section 15).

1. **Component-resolved styling.** A component resolves its own final look (theme + variant + interaction state) and passes it as a **prop** on the returned `UiElement`, exactly like React. There is **no** imperative `set_visual_style` setter and **no** CSS-style cascade in the paint pass. `build_draw_list` is a **pure transcriber** that resolves nothing.
2. **Split the fused `Style`** (`src/ui/runtime/tree.h:236`) into `LayoutStyle` (the Yoga-mirrored fields) and `VisualStyle` (paint). Both are separate props on the host element.
3. **`VisualStyle` is a dense POD** — the resolved form the renderer reads. No optionals on the hot path.
4. **One optionality convention for authoring overlays:** a sparse `StylePatch` using an explicit per-field set-flag `Opt<T>{bool set; T value;}`. **No value-space sentinels anywhere** in authoring. The `{0,0,0,1}` "invisible black text" footgun must be impossible.
5. **Theme delivered through a normal `ThemeContext`** via `use_context` (the runtime's existing context/provider machinery, `src/react.h:182-195`). Nested themes = nested providers. Providers commit no node and the context stack is unwound before `build_draw_list`, so querying theme during the draw DFS is impossible by construction — the old "query theme from the snapshot during paint" is **deleted**.
6. **`resolve(role, variant, interaction) -> VisualStyle` runs in the component** at authoring time. Fixed precedence, low→high: `base` (dense) → variant patch → active interaction-state patches **hover, focus_visible, pressed, checked, active, disabled**.
7. **Inheritance via context, not an automatic cascade.** Text default color/font flow through a `TextStyleContext` that `Text` reads via `use_context`; a parent sets it for its subtree by providing it. There is **no** build-pass `INHERITED_SET` cascade.
8. **Interaction by every-frame read.** The runtime re-renders the whole tree each frame. A component reads its current interaction state during its normal render via `use_hovered()`, `use_pressed()`, `use_focused()`, `use_focus_visible()`, populated by the framework's existing per-frame hit-test/focus pass. One-frame lag is acceptable. **No** new `on_pointer_*` events. None of this lives in the paint pass.
9. **`DrawCommand` IR:** a tagged union of trivially-copyable POD arms; `static_assert(is_aggregate_v && is_trivially_copyable_v)`. The first union arm carries a default member initializer so `DrawCommand` stays an aggregate. Out-of-line arenas for variable data. **Premultiplied alpha end-to-end** (so `(0,0,0,0)` is the only transparent). Hierarchical z-resolution **during** the DFS (no global flat sort). Per-frame reset = cursors only. Overflow ⇒ `error_count` ⇒ failed frame; never silent truncation or clamp. Capacity is derived from the v1 used set, not a Clay worst case.
10. **Renderer = a dumb linear executor** over the command array, switching on `kind`; `Clip` and `Layer` manipulate fixed stacks. `ui/` stays SDL-free.

---

## 1. Architecture & ownership

### 1.1 The one-line data flow

Style is resolved exactly once per node per frame, *inside the component*, and is never touched again:

```text
component resolve()  →  VisualStyle (dense POD)  →  HostProps.visual  →  reconciler commits onto Node
   →  Yoga layout (LayoutStyle only)  →  build_draw_list (PURE transcribe)  →  DrawCommand[]  →  SDL executor
```

Every arrow is a phase boundary with a single owner:

1. **Resolve (component, authoring pass).** `Button`/`Text`/etc. read theme via `use_theme()` (wrapping `use_context(&ThemeContext)`), read live interaction signals via `use_hovered()/use_pressed()/use_focused()/use_focus_visible()`, and call `resolve(role, variant_patch, interaction) -> VisualStyle`. The result is a fully-baked, dense `VisualStyle`.
2. **Props.** The component returns a `UiElement` whose `HostProps` carry `LayoutStyle layout` and `VisualStyle visual` as two separate props (the fused `Style` at `src/ui/runtime/tree.h:236` is split — see §2).
3. **Reconcile/commit.** The reconciler (`src/ui/runtime/element.cpp`, driven from `src/client/ui/client_ui.cpp`) writes `visual` onto the retained `UiTree::Node` (today `Node::style`, `tree.h:475`) exactly as it commits style today — a memberwise copy of a POD, no interpretation.
4. **Layout.** The Yoga adapter (`src/ui/runtime/yoga_flex_layout.cpp`) feeds Yoga from `LayoutStyle` *only*. `VisualStyle` is invisible to Yoga; the lone exception is `hidden`, which never affects layout (it suppresses paint, not box flow).
5. **Transcribe.** `build_draw_list` (the rewrite of `src/ui/runtime/draw_list.cpp:226`) walks the tree and, per node, copies the already-resolved `VisualStyle` into `DrawCommand[]`. It computes nothing about appearance: no theme lookup, no role switch, no fill defaulting, no focus-ring injection. Geometry it *may* compute (rect math, z-bracketing, per-line text placement from the measurer) — appearance it copies.
6. **Execute.** The SDL renderer (`src/renderer/sdl_retained_renderer.{h,cpp}`) is a dumb linear loop over `DrawCommand[]`, switching on `DrawCommandKind`, manipulating fixed `Clip`/`Layer` stacks. `ui/` stays SDL-free; the only `ui/`→renderer seams are the `DrawCommand` list and the injected `MeasureTextFn`.

### 1.2 Ownership table

| Concern | Owner | Notes |
|---|---|---|
| Final appearance of a node (the dense `VisualStyle`) | **Component** | Computed by `resolve()` at authoring time, passed as the `.visual` prop. Nothing downstream may alter it. |
| Style *resolution* (base → variant → interaction patches) | **Component** (via `resolve()`) | Fixed precedence (§5). Runs once, in the render function. |
| Inherited text defaults (color/font for a subtree) | **Component** (via `TextStyleContext`) | A parent *provides* a `TextStyleContext`; `Text` reads it with `use_context`. No build-pass inheritance cascade. |
| Theme delivery | **Framework** (context/provider machinery) | `Theme` flows through `ThemeContext` (`src/react.h:182-195`). Nested themes = nested providers. The context stack is unwound before `build_draw_list`, so querying theme during the draw DFS is impossible by construction. |
| Interaction signals (`hovered/pressed/focused/focus_visible`) | **Framework** | Populated by the existing per-frame hit-test/focus pass. Surfaced to components as `use_hovered()` etc. No new `on_pointer_*` events. |
| Layout | **Framework** (Yoga adapter) | Consumes `LayoutStyle` only. |
| Transcription (tree → `DrawCommand[]`) | **Framework** (`build_draw_list`) | Pure: copies resolved `VisualStyle`, computes geometry/z-order, emits per-line `Text` from the measurer. Resolves nothing about appearance. |
| Rendering (`DrawCommand[]` → pixels) | **Framework** (SDL renderer) | Dumb linear executor; `Clip`/`Layer` push..pop over fixed stacks; premultiplied-alpha end to end. |
| Text measurement | **Framework** (injected `MeasureTextFn`) | One function pointer installed once; used as *both* the Yoga `MeasureFn` and the draw-time measurer so measure == draw. |

The shell contract is unchanged: deferred state writes during the declaration pass still go through `client::ui::ClientUi::queue_deferred_write` (per `CLAUDE.md`). Resolution reads context and interaction; it never mutates game/app state.

### 1.3 The inversion vs. the old design

The old `build_draw_list` (`src/ui/runtime/draw_list.cpp`) is a *resolver wearing a transcriber's name*. It:

- switches on `NodeRole` to pick fills from hardcoded constants `kButtonFill`/`kInputFill`/`kCheckedFill` (`draw_list.cpp:10-21,31-39`) — appearance decided in the paint pass;
- injects the focus ring during the DFS (`draw_list.cpp:59-66`) — interaction-state styling baked into the builder;
- recomputes a disabled cascade by threading `inherited_disabled` down the recursion (`draw_list.cpp:80-213`) — a build-pass inheritance cascade;
- defaults text color in-band via a `has_color` value-space sentinel (`color.a > 0`) — the exact `a==0 == "unset"` footgun this rewrite bans.

The new design **inverts all of this**: there is **no cascade, no resolution, and no theme/interaction lookup in the paint pass.** The rewrite deletes the role switch, the `k*Fill`/`k*Border` constants, the focus-ring injection, the `inherited_disabled` parameter threading, and `has_color`/all value-space sentinels. The focus ring becomes `VisualStyle::outline`, resolved by the focusable component from `Theme::focus_ring` + the `focus_visible` interaction patch. Disabled-dimming becomes a `disabled` `StylePatch` plus a dimmed `TextStyleContext`.

This makes a class of bug structurally impossible rather than merely fixed:

- **The invisible-black-text footgun** cannot occur: authoring overlays carry presence in `Opt<T>::set`, never in the value, and `Color` is premultiplied at the IR boundary so `(0,0,0,0)` is *unambiguously fully transparent*, not "unset."
- **The `strlen * kCharWidth = 8` caret bug** (`draw_list.cpp:120`) cannot occur: caret/selection geometry comes from the same injected `MeasureTextFn` that drove layout, so measure == draw by construction.
- **Theme-from-snapshot during paint** cannot occur: the context stack is unwound before `build_draw_list` runs, so the data is not reachable there.

### 1.4 Locked constraints (binding on every section)

- **Language:** C++20. **No exceptions** and **no RTTI** in `ui/` and the runtime; error paths use `error_count` / boolean returns, never `throw`/`dynamic_cast`/`typeid`.
- **Warnings:** built `-Wall -Wextra`. New POD aggregates initialize every member.
- **House data style:** fixed-size aggregates, no heap on the hot path. Capacities are explicit compile-time constants derived from the v1 used set. Variable-length data (text bytes, gradient stops) lives in out-of-line arenas, not inline caps.
- **Aggregate-ness is load-bearing:** `VisualStyle`, `StylePatch`, every `DrawCommand` union arm, and `DrawCommand` itself are aggregates so designated-initializer call sites compile; the IR additionally asserts `is_aggregate_v && is_trivially_copyable_v` (so per-frame reset is cursor-only, never a `memset` of the command array).
- **Single-threaded UI.** The runtime renders the whole tree every frame (`react_begin_frame`/`react_end_frame`); the one-frame lag in interaction signals is acceptable. No locking, no atomics, no cross-thread sharing.
- **Premultiplied alpha at the IR boundary.** Authored colors are straight alpha; conversion to premultiplied happens exactly once, at IR emit (§8.4). `(0,0,0,0)` is the only transparent.
- **Overflow is failure, never silent.** Any capacity exceeded (`DrawList`, arenas, line array, text budget) increments `error_count` and fails the frame. No truncation, no clamp, no `strncpy`-style silent cut.
- **Boundaries:** `ui/` stays SDL-free and game-vocabulary-free. New style/IR types live under `src/ui/` (`runtime/` or `src/ui/style/`). Renderer code lives under `src/renderer/`. Dependency direction holds: `client/ui/screens → client/ui → game → ui → react`.

---

## 2. Style model: LayoutStyle / VisualStyle split

The current runtime fuses layout and paint into a single `Style` aggregate (`src/ui/runtime/tree.h:236`), carried as one prop and committed onto the node. That fusion is the root cause of the layout-clobber footgun: any component that wants to change a color writes a whole `Style`, and unless it first copies every Yoga field forward it silently resets `width`, `flex_grow`, `padding`, etc. back to defaults. We split `Style` into two independent props so that **mutating paint cannot touch layout, by construction** — there is no shared field to clobber.

### 2.1 LayoutStyle — the Yoga-mirrored fields

`LayoutStyle` is exactly the layout half of today's `Style`, extracted verbatim. Every field is a 1:1 mirror of a Yoga input that the layout adapter already reads; no semantics change. The paint fields (`background`, `border` color, `text` color, `border_width`, `font_size`) are *removed* and move to `VisualStyle`. Note `border_widths` (the per-edge Yoga *layout* border) stays in `LayoutStyle` — it feeds box geometry, not paint — while the painted border color/width live in `VisualStyle::border`.

```cpp
// src/ui/runtime/tree.h — replaces the layout half of the old fused Style.
struct LayoutStyle {
    LayoutDirection layout_direction = LayoutDirection::Inherit;
    BoxSizing       box_sizing       = BoxSizing::BorderBox;
    Display         display          = Display::Flex;
    PositionType    position         = PositionType::Relative;
    Overflow        overflow         = Overflow::Visible;
    FlexDirection   direction        = FlexDirection::Column;
    FlexWrap        wrap             = FlexWrap::NoWrap;
    AlignItems      align_items      = AlignItems::Stretch;
    AlignItems      align_content    = AlignItems::Start;
    AlignItems      align_self       = AlignItems::Auto;
    JustifyContent  justify_content  = JustifyContent::Start;
    LayoutNodeType  node_type        = LayoutNodeType::Default;
    bool            is_reference_baseline         = false;
    bool            always_forms_containing_block = false;

    Length width      = Length::auto_size();
    Length height     = Length::auto_size();
    Length min_width  = Length::auto_size();
    Length min_height = Length::auto_size();
    Length max_width  = Length::auto_size();
    Length max_height = Length::auto_size();
    Length flex_basis = Length::auto_size();

    StyleFloat flex         = {};
    StyleFloat flex_grow    = {};
    StyleFloat flex_shrink  = {};
    StyleFloat aspect_ratio = {};

    EdgeSizes  margin         = {};
    EdgeSizes  padding        = {};
    EdgeSizes  position_inset = {};
    EdgeSizes  border_widths  = {};   // per-edge Yoga layout border (NOT paint)
    StyleValue gap            = {};
    StyleValue row_gap        = {};
    StyleValue column_gap     = {};
};
```

The layout adapter reads only `LayoutStyle`; it never sees a color or a font.

### 2.2 VisualStyle — the dense, resolved paint description

`VisualStyle` is the **dense POD the renderer reads** — the fully-resolved output of `resolve()` run in the component at authoring time. There are no optionals on this hot path: every field is a plain value, so `build_draw_list` branches on cheap presence checks (`stop_count==0`, `texture_id==0`, `color.a==0`, `width==0`) rather than unwrapping options. Authoring optionality lives only in `StylePatch`'s `Opt<T>` overlays (§3); by the time a `VisualStyle` reaches a node, every overlay has been folded in.

Used verbatim from the canonical signatures. **All `Color` values here are authored straight-alpha** — premultiplication happens once, at the IR boundary (§8.4):

```cpp
struct Color { uint8_t r=0,g=0,b=0,a=0; };              // straight alpha in authoring; premultiplied only inside the IR arenas
struct Vec2  { float x=0, y=0; };
struct SideWidths { float top=0,right=0,bottom=0,left=0; };
struct SideColors { Color top{},right{},bottom{},left{}; };
struct Border  { SideWidths width{}; SideColors color{}; };
struct Outline { float width=0; Color color{}; float offset=0; };   // signed offset: <0 inset, >0 outset
struct GradientStop { float t=0; Color color{}; };                  // t in [0,1]
constexpr int UI_MAX_GRADIENT_STOPS = 8;
struct Gradient { float angle_deg=0; uint8_t stop_count=0; GradientStop stops[UI_MAX_GRADIENT_STOPS]{}; };
struct BackgroundImage { uint32_t texture_id=0; Color tint{255,255,255,255}; SideWidths nine_slice{}; };
struct Shadow { Color color{}; Vec2 offset{}; float blur=0; float spread=0; };   // drop only in v1
enum class TextAlign : uint8_t { Left=0, Center, Right };
enum class TextWrap  : uint8_t { None=0, Words };
struct TextVisual { Color color{}; uint16_t font_id=0; uint16_t font_size=0;
                    TextAlign align=TextAlign::Left; TextWrap wrap=TextWrap::None; float line_height=0; };

struct VisualStyle {
    Color           background{};
    float           corner_radius=0;     // uniform in v1
    Border          border{};
    Outline         outline{};           // focus ring etc.
    Gradient        gradient{};          // stop_count==0 => no gradient
    BackgroundImage image{};             // texture_id==0 => no image
    Shadow          shadow{};            // color.a==0 => no shadow (resolved presence check)
    float           opacity=1;
    bool            hidden=false;        // skip paint, keep layout
    TextVisual      text{};
};
static_assert(std::is_aggregate_v<VisualStyle>);
static_assert(std::is_trivially_copyable_v<VisualStyle>);
```

### 2.3 Field-by-field meaning

- **`background` (`Color`)** — solid fill of the node's border-box. `a==0` means no fill is emitted. `{0,0,0,1}` is opaque black, not "unset" — presence is never inferred from a value an author might legitimately want.
- **`corner_radius` (`float`)** — uniform corner radius in points, applied to background, gradient, border, and clip. v1 is a single float; the field is positioned to widen to a per-corner struct later.
- **`border` (`Border`)** — per-side painted border: `width{top,right,bottom,left}` and `color{...}`. A side is painted only when its `width>0` and its `color.a>0`. v1 draws per-side stroking co-feathered with the fill in a single pass. Independent of `LayoutStyle::border_widths`.
- **`outline` (`Outline`)** — single-color stroke at a signed `offset` from the border-box: `offset<0` insets, `offset>0` outsets; default is small positive (outset) for the focus ring. Painted when `width>0` and `color.a>0`. The global ring color comes from `Theme::focus_ring`, resolved into here by the component.
- **`gradient` (`Gradient`)** — linear N-stop fill at `angle_deg`, capped at `UI_MAX_GRADIENT_STOPS=8`. `stop_count==0` ⇒ no gradient. Stops are inline for authoring; the IR copies variable-length stop data to an out-of-line arena at emit time.
- **`image` (`BackgroundImage`)** — textured rect: `texture_id==0` ⇒ no image. `tint` multiplies the sampled texel (default opaque white = untinted). `nine_slice{...}` gives scalable sprite chrome; all-zero ⇒ a plain stretched textured rect. Nine-slice + rounded corners is cut (radius ignored on nine-slice in v1).
- **`shadow` (`Shadow`)** — drop shadow only in v1: `color` (`color.a==0` ⇒ no shadow), `offset`, `blur` (feather radius), `spread`. This `color.a==0` test is a *resolved-output* presence cue, distinct from authoring sentinels.
- **`opacity` (`float`)** — group opacity in `[0,1]`. `1` is the common path (no layer). `<1` brackets the subtree in a `LayerPush`/`LayerPop` for a single naive render-to-target composite.
- **`hidden` (`bool`)** — when `true`, the node contributes nothing to paint but still participates in layout. Orthogonal to `LayoutStyle::display` (which removes the node from layout entirely).
- **`text` (`TextVisual`)** — resolved text paint for `Text` nodes: `color`, `font_id`, `font_size`, `align`, `wrap`, `line_height`. Non-text nodes leave this default (`color.a==0` ⇒ nothing to draw).

### 2.4 Two props on the host element

`HostProps` carries both as separate, independent props, replacing the single fused `Style style` at `src/ui/runtime/element.h:71`:

```cpp
struct HostProps {
    const char *key = nullptr;
    const char *id  = nullptr;
    int   id_offset = 0;
    LayoutStyle layout = {};   // Yoga-mirrored; consumed only by the layout adapter
    VisualStyle visual = {};   // dense resolved paint; consumed only by build_draw_list
    HostTextProps      text = {};
    NodeInteraction    interaction = {};
    TextEditMetadata   text_edit = {};
    AccessibilityProps accessibility = {};
    HostCallbacks      callbacks = {};
    UiChildren         children = {};
};
```

The reconciler commits `.visual` onto the `Node` exactly as it commits style today, and commits `.layout` to the layout-input fields. Because the two are disjoint structs, a component that returns `box({ .layout = parent_layout, .visual = resolved })` cannot accidentally reset a layout field by touching paint — the keystone property of this rewrite.

---

## 3. Optionality: StylePatch + one set-flag convention

`VisualStyle` (§2) is dense — every field always has a meaningful value. Authoring is the opposite problem: a variant or interaction-state layer wants to override *some* fields and leave the rest untouched. That sparse overlay is `StylePatch`, and it expresses presence with exactly one mechanism — an explicit per-field set-flag — never a value-space sentinel.

### 3.1 The one wrapper: `Opt<T>`

There is a single optionality type for authoring:

```cpp
template <class T> struct Opt { bool set = false; T value{}; };   // the ONE optionality wrapper for authoring
```

`Opt<T>` is a trivially-copyable aggregate: default-constructed it is `{set=false, value{}}`, i.e. "this layer does not touch this field." This is the generalization of the existing `StyleFloat{bool defined; float value;}` idiom at `src/ui/runtime/tree.h:75-87`, which already encodes "is this float meaningfully present?" as a discrete `bool` rather than overloading `0.0f`. `StyleFloat`/`StyleValue`/`Length` stay on the layout side; `Opt<T>` is its visual-side counterpart. We deliberately do **not** use `std::optional`: `Opt<T>` is a fixed-size aggregate, composes into the larger `StylePatch` without engaging optional's non-trivial machinery, and keeps `StylePatch` an aggregate.

The single helper for constructing a set field:

```cpp
template <class T> constexpr Opt<T> opt(T v) { return {true, v}; }
```

### 3.2 StylePatch: one `Opt<T>` per `VisualStyle` field

```cpp
// src/ui/style/style_patch.h
struct StylePatch {
    Opt<Color>           background;
    Opt<float>           corner_radius;
    Opt<Border>          border;
    Opt<Outline>         outline;
    Opt<Gradient>        gradient;
    Opt<BackgroundImage> image;
    Opt<Shadow>          shadow;
    Opt<float>           opacity;
    Opt<bool>            hidden;
    Opt<TextVisual>      text;
};
static_assert(std::is_aggregate_v<StylePatch>);
static_assert(std::is_trivially_copyable_v<StylePatch>);
```

Wrapping is at the granularity of the `VisualStyle` member. Compound members (`Border`, `Outline`, `Gradient`, `BackgroundImage`, `Shadow`, `TextVisual`) are wrapped whole, not per-sub-field: a patch that wants a different border supplies a complete `Border{}`. This keeps `StylePatch` a flat ten-member aggregate and `apply()` a ten-line copy. Sub-field granularity is explicitly **not** a v1 requirement.

### 3.3 The single rule: presence == `Opt::set`

> **Presence of an override is `Opt::set == true`. Nothing else. Ever.**

No field's *value* signals presence. `background.value == {0,0,0,0}` does not mean "no override" — it means "override the background to transparent." `corner_radius.value == 0` means "square this corner," not "unset." The flag and the value are orthogonal, and `apply()` reads only the flag.

This kills two footguns the retired sentinel approach carried:

- **The `{0,0,0,1}` invisible-black footgun is unrepresentable.** `{set=false}` and `{set=true, value={0,0,0,1}}` are different states. No value of `Color` means "absent," so there is no value an author must avoid.
- **"Explicitly transparent" is a first-class override.** A patch can force the background to nothing with `{.background = opt(Color{0,0,0,0})}`. A sentinel scheme literally cannot encode this, because there transparent *is* the "unset" signal.

### 3.4 `apply(VisualStyle&, const StylePatch&)`

`apply` is the only function that reads a `StylePatch`. It writes a field iff that field's `Opt::set` is true and copies the value verbatim — no clamping, no blending, no sentinel checks. It is `ui/`-pure, constexpr-friendly, branch-per-field, and last-write-wins when called repeatedly (which is how `resolve()` layers patches — §5).

```cpp
// src/ui/style/style_patch.h
constexpr void apply(VisualStyle& dst, const StylePatch& p) {
    if (p.background.set)    dst.background    = p.background.value;
    if (p.corner_radius.set) dst.corner_radius = p.corner_radius.value;
    if (p.border.set)        dst.border        = p.border.value;
    if (p.outline.set)       dst.outline       = p.outline.value;
    if (p.gradient.set)      dst.gradient      = p.gradient.value;
    if (p.image.set)         dst.image         = p.image.value;
    if (p.shadow.set)        dst.shadow        = p.shadow.value;
    if (p.opacity.set)       dst.opacity       = p.opacity.value;
    if (p.hidden.set)        dst.hidden        = p.hidden.value;
    if (p.text.set)          dst.text          = p.text.value;
}
```

`merge` is the patch-onto-patch counterpart, used for caller overrides (§5.5, §11.8). Its contract is fixed: **`merge(StylePatch& dst, const StylePatch& src)` copies each `src` field whose `set==true` into `dst`; `src` wins; `dst` is mutated in place.**

```cpp
constexpr void merge(StylePatch& dst, const StylePatch& src) {
    if (src.background.set)    dst.background    = src.background;
    if (src.corner_radius.set) dst.corner_radius = src.corner_radius;
    if (src.border.set)        dst.border        = src.border;
    if (src.outline.set)       dst.outline       = src.outline;
    if (src.gradient.set)      dst.gradient      = src.gradient;
    if (src.image.set)         dst.image         = src.image;
    if (src.shadow.set)        dst.shadow        = src.shadow;
    if (src.opacity.set)       dst.opacity       = src.opacity;
    if (src.hidden.set)        dst.hidden        = src.hidden;
    if (src.text.set)          dst.text          = src.text;
}
```

### 3.5 The fluent `patch()` builder

A small builder keeps authoring sites terse and makes the set-flag impossible to forget. `patch()` returns a `StylePatch` value; each setter operates on the materialized temporary and returns `StylePatch&` so chaining composes:

```cpp
// src/ui/style/style_patch.h — chaining operates on the materialized temporary.
struct StylePatchBuilder {
    StylePatch p{};
    StylePatchBuilder& background(Color c)    { p.background = opt(c); return *this; }
    StylePatchBuilder& corner_radius(float r) { p.corner_radius = opt(r); return *this; }
    StylePatchBuilder& border(Border b)       { p.border = opt(b); return *this; }
    StylePatchBuilder& outline(Outline o)     { p.outline = opt(o); return *this; }
    StylePatchBuilder& gradient(Gradient g)   { p.gradient = opt(g); return *this; }
    StylePatchBuilder& image(BackgroundImage i){ p.image = opt(i); return *this; }
    StylePatchBuilder& shadow(Shadow s)       { p.shadow = opt(s); return *this; }
    StylePatchBuilder& opacity(float o)       { p.opacity = opt(o); return *this; }
    StylePatchBuilder& hidden(bool h)         { p.hidden = opt(h); return *this; }
    StylePatchBuilder& text(TextVisual t)     { p.text = opt(t); return *this; }
    operator StylePatch() const { return p; }   // implicit collapse to a value
};
inline StylePatchBuilder patch() { return {}; }
// authoring: StylePatch hover = patch().background({20,20,28,255}).corner_radius(6.f);
```

`opt()` is the low-level primitive (used in theme-data tables); `patch()` is the authoring sugar for components. Use `patch()` in component/variant code and `opt()` in dense theme initializers.

### 3.6 Resolved-output presence vs. authoring presence

`VisualStyle` itself uses a small number of **value-as-presence** checks — but only as resolved-output cues the renderer reads, never as authoring sentinels: `gradient.stop_count == 0` ⇒ no gradient, `image.texture_id == 0` ⇒ no image, `shadow.color.a == 0` ⇒ no shadow, `background.a == 0` ⇒ no fill. These live exclusively in the *dense, post-`resolve()*` output and describe "the resolver produced nothing here," which is a legitimate render-time skip. An author never writes them, never reads them, and `StylePatch` never uses them. All authoring presence is `Opt::set`.

---

## 4. Theme + delivery via context

The theme is a single immutable POD owned by the app and handed to the subtree through a normal `ReactContext`. Components read it with `use_context`, resolve their own `VisualStyle` at authoring time (§5), and pass it as `.visual`. The theme is never consulted by the renderer, and never queryable during the draw DFS.

### 4.1 Canonical theme types

`RoleStyle` is a dense `VisualStyle base` plus one sparse `StylePatch` per interaction state. `resolve()` (§5) layers the active patches over `base` low→high. Use these signatures verbatim; they live under `src/ui/style/`:

```cpp
struct RoleStyle {
    VisualStyle base{};
    StylePatch  hover{}, focus_visible{}, pressed{}, checked{}, active{}, disabled{};
};

struct Theme {
    RoleStyle box, text, button, input, checkbox, checkbox_mark, dialog;
    // global tokens (dense, not patches), for components that compose their own subparts:
    Color focus_ring{}, text_default{}, text_disabled{}, caret{}, selection{};
};
```

`Theme` is a fixed-size aggregate of PODs (no pointers, no heap, trivially copyable). The seven `RoleStyle` members are the v1 role vocabulary; they map onto the components that ship in `src/ui/components/`. `checkbox_mark` is the inner mark a `Checkbox` composes (its fill flips on `st.checked` via the `checked` patch — see §11.7); it is a distinct role rather than a one-off `VisualStyle` so the mark participates in the same resolve path. Adding a role is adding a field; there is no string-keyed registry and no runtime lookup.

The five global tokens are *not* roles — they are loose colors a component reaches for when it composes a subpart that has no `RoleStyle`:
- `focus_ring` — the color a focusable control writes into its resolved `Outline` when `focus_visible` is active.
- `text_default` / `text_disabled` — the colors a parent pushes through `TextStyleContext` (§6) so `Text` inherits via context.
- `caret` / `selection` — consumed by `Text`/input editing geometry, driven by measured advances (§10).

**Variant differences are component-owned, not theme fields.** There is no per-variant seat (no `danger_fill`, no `button_primary`) on `Theme`. A component maps its `Variant` enum to a `StylePatch` (§5.4) using base/token values; this keeps `Theme` to the canonical shape above. Adding per-variant theme seats would be a deliberate spec change, not a silent field addition.

### 4.2 The context object

Exactly one `ReactContext` carries the theme, using the runtime's existing machinery (`src/react.h:182-195`):

```cpp
// src/ui/style/theme.h
namespace ui::style {
inline ReactContext ThemeContext = {};            // current = const Theme*

inline const Theme& use_theme() {
    const Theme* t = static_cast<const Theme*>(use_context(&ThemeContext));
    return t ? *t : default_theme();               // app installs one at root; never null
}
}
```

`ThemeContext.current` holds a `const Theme*` — `use_context` is `void*`-typed (`src/react.h:190`), so the pointer is stored directly with no allocation. **All theme reads go through `use_theme()`**, which wraps the `&`-address, `static_cast`, and null fallback. No call site writes the raw `use_context(&ThemeContext)` form.

### 4.3 Nested providers just work

A `ThemeProvider` is a transparent provider that commits **no layout node** — it brackets its body with `REACT_PROVIDER_ENTER/EXIT` (`src/react.h:102-108`) and pushes the pointer for the body's duration:

```cpp
UiElement ThemeProvider(const Theme* theme, const UiChildren& children) {
    REACT_PROVIDER_ENTER("ThemeProvider");
    UiElement out;
    PROVIDE(&ui::style::ThemeContext, const_cast<Theme*>(theme)) {
        out = fragment(children);
    }
    REACT_PROVIDER_EXIT();
    return out;
}
```

`PROVIDE` is the runtime's push/pop pair around a single-iteration loop (`src/react.h:193-195`); `ReactContext` keeps a 16-deep stack. Nesting a second `ThemeProvider` pushes onto the same stack; `use_theme()` inside the inner subtree reads the inner `Theme*`, and the outer value is restored on exit. A nested provider is a full replacement `Theme` — for a tweaked accent, author a derived `Theme` (copy + edit) and provide that; `Theme` is cheap to copy.

### 4.4 Snapshot-time theme query is architecturally impossible

The old renderer pulled theme colors out of the committed node snapshot during the paint DFS. That cannot happen here:

1. **Providers commit no node.** There is no host node carrying a theme pointer in the retained tree for the draw walk to read.
2. **The context stack is unwound before paint.** `react_end_frame()` completes the declaration pass; every `PROVIDE` for-loop has already run its `react_provider_pop`. `ThemeContext` is back to the root value (or null) before `build_draw_list` runs.
3. **`build_draw_list` is a pure transcriber.** It reads each node's already-committed `VisualStyle` and emits `DrawCommand`s. It has no `Theme` parameter, no `ThemeContext` access, and no resolve call.

There is no API to "get the theme during draw," no theme field on the draw-time snapshot, and no place to wire one back in.

### 4.5 `default_theme()`

The app installs a root `ThemeProvider(&default_theme(), ...)`. `default_theme()` returns a `static const Theme` seeded from the current `common.h` palette (`src/ui/components/common.h:46-50`) as the *starting point*. There is **no parity constraint** — these values are v1 defaults and freely re-chooseable; goldens are written fresh.

The old `control_style()` (`common.h:91-99`) used in-band sentinels (`background.a == 0` ⇒ "unset", `border_width <= 0` ⇒ "default"). Those are exactly the value-space sentinels this design bans; they do not survive the port. Presence is the `Opt::set` flag; the dense `base` always carries a real color.

Which patches default populated vs empty, **stated explicitly**:
- `base` — always populated (dense).
- `disabled` — populated for `button`/`input`/`checkbox` (dimmed fill + border).
- `checked` — populated for `checkbox`/`checkbox_mark`.
- `focus_visible` — populated for every focusable role (`button`/`input`/`checkbox`) with the focus-ring `Outline` from `focus_ring`. **This is the only wiring that produces the locked focus ring**, so it must be present in the default theme.
- `hover`/`pressed`/`active` — default empty in v1 (left for tuning); a control with no hover/pressed patch simply does not change on those states.

```cpp
// src/ui/style/default_theme.cpp  (illustrative; values freely chooseable, no parity)
const Theme& default_theme() {
    static const Theme t = []{
        using namespace ui::style;
        Theme th{};
        th.focus_ring   = {120, 170, 255, 255};
        th.text_default = {224, 228, 236, 255};
        th.text_disabled= {120, 128, 140, 255};
        th.caret        = {224, 228, 236, 255};
        th.selection    = { 44,  92, 128, 180};   // straight alpha; premultiplied at IR emit

        // Focus ring patch: ONLY outline is set; everything else inherits from base.
        const StylePatch focus_ring_patch =
            patch().outline(Outline{ .width = 2, .color = th.focus_ring, .offset = 2 });

        auto seed_control = [&](RoleStyle& r) {
            r.base.background   = {24, 28, 36, 255};                 // kControlFill
            r.base.border.width = {1, 1, 1, 1};
            r.base.border.color = {{78,88,104,255},{78,88,104,255},
                                   {78,88,104,255},{78,88,104,255}}; // kControlBorder
            r.disabled.background = opt(Color{30,34,42,255});        // kControlDisabledFill
            r.disabled.border     = opt(Border{
                {1,1,1,1}, {{62,68,78,255},{62,68,78,255},
                            {62,68,78,255},{62,68,78,255}}});        // kControlDisabledBorder
            r.focus_visible = focus_ring_patch;                      // the locked focus ring
        };
        seed_control(th.button);
        seed_control(th.input);
        seed_control(th.checkbox);
        th.checkbox.checked.background      = opt(Color{44,92,128,255});  // kCheckboxCheckedFill
        th.checkbox_mark.base.background    = {0,0,0,0};                  // hidden until checked
        th.checkbox_mark.checked.background = opt(Color{224,228,236,255});// the visible mark

        th.box.base       = VisualStyle{};                          // transparent layout box
        th.text.base.text = TextVisual{ .color = th.text_default, .font_size = 14 };
        th.dialog.base.background = {18, 21, 27, 245};
        return th;
    }();
    return t;
}
```

The exact numbers are placeholders for the new look. The contract is the *shape*: dense `base`, `Opt`-flagged patches, `focus_visible` wired for focusables, global tokens for composed subparts, delivered by one `ThemeContext`, with zero theme visibility in the paint pass.

---

## 5. `resolve()`: component-side resolution + variants

`resolve()` is the single function that turns a `RoleStyle`, a component's chosen variant patch, and the component's current `InteractionState` into one dense `VisualStyle`. It runs **inside the component** during its normal render, and its return value is committed as the `.visual` prop. Nothing downstream resolves anything.

### 5.1 Signature and precedence

```cpp
// src/ui/style/resolve.h  (ui/ — SDL-free, game-vocabulary-free)
namespace ui::style {

VisualStyle resolve(const RoleStyle& role,
                    const StylePatch& variant,
                    const InteractionState& st);

} // namespace ui::style
```

The *variant* is a `StylePatch` the component supplies (it maps its own `Variant` enum to a patch — §5.4); there is no single `Variant` type, since each role owns its own enum. Precedence is fixed, lowest to highest:

```
base (dense)
  ← variant            (one StylePatch)
  ← hover              (RoleStyle::hover,         applied iff st.hovered)
  ← focus_visible      (RoleStyle::focus_visible, applied iff st.focus_visible)
  ← pressed            (RoleStyle::pressed,       applied iff st.pressed)
  ← checked            (RoleStyle::checked,       applied iff st.checked)
  ← active             (RoleStyle::active,        applied iff st.active)
  ← disabled           (RoleStyle::disabled,      applied iff st.disabled)   ← wins
```

Each `←` overlays a `StylePatch` via `apply()`, copying only the fields whose `Opt<T>::set == true`. Disabled is last so a disabled control always reads dimmed; a held-down focused button shows pressed over focus_visible. Order is a hard contract — tests pin it.

`InteractionState` carries the `active` flag so this chain compiles (see §7.7 for its source):

```cpp
struct InteractionState {
    bool hovered=false, pressed=false, focused=false,
         focus_visible=false, checked=false, active=false, disabled=false;
};
```

### 5.2 The authoritative `resolve()` body

This is the single canonical listing; §11 references it rather than redefining it.

```cpp
// src/ui/style/resolve.cpp
namespace ui::style {

VisualStyle resolve(const RoleStyle& role, const StylePatch& variant,
                    const InteractionState& st) {
  VisualStyle vs = role.base;          // start dense
  apply(vs, variant);                  // variant first
  if (st.hovered)        apply(vs, role.hover);
  if (st.focus_visible)  apply(vs, role.focus_visible);
  if (st.pressed)        apply(vs, role.pressed);
  if (st.checked)        apply(vs, role.checked);
  if (st.active)         apply(vs, role.active);
  if (st.disabled)       apply(vs, role.disabled);   // disabled wins
  return vs;                            // dense, fully resolved
}

} // namespace ui::style
```

Whole-field granularity is deliberate: a patch that touches `border` replaces the whole `Border`. If a variant wants to keep three border sides and recolor one, it sets the entire `Border`. This keeps `apply` flat with no nested merges. `resolve()` is `ui/`-local, SDL-free, allocation-free, and trivially testable: feed PODs, assert on the returned POD.

### 5.3 Worked `Button` example

A `Button` reads its theme via context and its interaction flags via the per-frame hooks (§7), resolves once, and passes the dense result as `.visual`:

```cpp
// src/ui/components/button.cppx
::ui::UiElement Button(const ButtonProps& p) {
  const Theme& t = use_theme();
  InteractionState st{
      .hovered       = use_hovered(),
      .pressed       = use_pressed(),
      .focused       = use_focused(),
      .focus_visible = use_focus_visible(),
      .checked       = false,
      .active        = false,            // component-supplied; see §7.7
      .disabled      = p.disabled,
  };
  VisualStyle vs = resolve(t.button, button_variant_patch(t, p.variant), st);
  return box({
      .layout = { /* Yoga fields */ },
      .visual = vs,
      .interaction = { .focusable = true, .disabled = p.disabled },
      .children = p.children,
  });
}
```

Resolving a `Secondary` button under `st = { hovered=true, focus_visible=true, disabled=true }` (an unusual but legal combination that pins precedence):

1. `vs = base` → fill, no outline, full opacity.
2. `apply(variant)` → Secondary's patch sets only `border`.
3. `hovered` → `apply(hover)` lightens the fill (if `hover` is populated; default empty in v1).
4. `focus_visible` → `apply(focus_visible)` sets the outset focus ring.
5. `pressed`/`checked`/`active` skipped.
6. `disabled` (highest) → `apply(disabled)` overwrites fill/border, dims text, sets `opacity`.

Final dense `VisualStyle`: muted fill (disabled beat hover), the focus outline still present (disabled's patch did not touch `outline`, so it survives — if the design wanted disabled to suppress the ring, its patch would set `outline = {}`). The one-frame lag from reading last frame's hit-test is acceptable.

Because `Button` composes its own label, it can dim it via the resolved `vs.text` **or** provide a dimmed `TextStyleContext` to children (§6). Either is legal; both are component-side.

### 5.4 Variants are explicit, mapped to patches

Per composition-patterns, each role exposes an `enum class` variant and a small pure mapper from variant to `StylePatch`. The mapper reads theme tokens but returns only a sparse patch — it never returns a dense style, and it **never reads non-canonical theme fields**. `Default` is the empty patch:

```cpp
enum class ButtonVariant : uint8_t { Default = 0, Primary, Secondary, Ghost, Danger };

StylePatch button_variant_patch(const Theme& t, ButtonVariant v) {
  switch (v) {
    case ButtonVariant::Default:   return {};                                  // base, no overlay
    case ButtonVariant::Primary:   return patch().background(primary_fill(t)); // derived from tokens
    case ButtonVariant::Secondary: return patch().border(secondary_border(t));
    case ButtonVariant::Ghost:     return patch().background({0,0,0,0})        // transparent
                                                .border(no_border());
    case ButtonVariant::Danger:    return patch().background(danger_fill(t))   // helper, NOT a theme field
                                                .text(TextVisual{ .color = {255,255,255,255}, .font_size = 16 });
  }
  return {};   // unreachable; total switch over the enum
}
```

`primary_fill`/`danger_fill`/`secondary_border` are component-owned helpers that derive from `t.button.base` and global tokens — they are **not** members of `Theme`. `(0,0,0,0)` for Ghost is genuinely transparent (premultiplied end to end at the IR boundary), a real value, not a sentinel.

### 5.5 Caller-supplied overrides compose as one more patch

When a caller needs a one-off tweak, the override is a `StylePatch` prop, folded into the same precedence chain. It is merged **over** the variant and **before** interaction states, so theme interaction states still win:

```cpp
struct ButtonProps {
  // ... existing fields ...
  ButtonVariant variant = ButtonVariant::Default;
  StylePatch    style_override = {};     // sparse, optional; one merge, not N props
};

// In Button(), fold the caller override into the variant slot before resolve:
StylePatch v = button_variant_patch(t, p.variant);
merge(v, p.style_override);             // src (override) wins, mutates dst (v); see §3.4
VisualStyle vs = resolve(t.button, v, st);
```

This keeps the precedence story exactly one chain: `base ← (variant ⊕ override) ← interaction states`.

**Ergonomic cost, stated explicitly:** `style_override` is a **base/variant-layer-only** override. It **cannot touch interaction layers** (hover/focus_visible/pressed/checked/active/disabled) — those theme patches still apply on top. A caller that genuinely must change an interaction state (e.g. "make this one button's disabled state lighter") provides a nested `ThemeContext` with an adjusted `RoleStyle` for its subtree:

```cpp
// Escape hatch for per-interaction-state override: nest a tweaked RoleStyle.
Theme local = use_theme();              // copy
local.button.disabled = patch().background({40,44,52,255});  // lighter disabled
return ThemeProvider(&local, { /* the one button */ });
```

There is deliberately **no** per-interaction-state override prop, which would reintroduce boolean-prop soup and a second precedence axis.

---

## 6. Inheritance via context (not a cascade)

Text inherits two things and only two things in v1: a **default color** and a **default font** (face + size). Alignment, wrap, and line height are local to each `Text`. The old runtime delivered the color default by threading `inherited_disabled` down the draw DFS (`src/ui/runtime/draw_list.cpp:80-213`) and falling back to `kTextFill`/`kTextDisabledFill`. That cascade is **deleted**. The same dimming is produced the React way: a parent that wants its subtree's text dimmed *provides* a dimmed default, and `Text` *reads* the default it sees.

### 6.1 TextStyleContext

The inheritable text defaults are their own context value, distinct from `Theme`. `Theme` is the static design system; `TextStyleContext` is the *dynamic, position-dependent* default that flows down a subtree. It uses the **same optionality convention as everything else** — `Opt<T>` per field, never an in-band sentinel:

```cpp
// src/ui/style/text_style_context.h  — ui/, SDL-free, game-vocabulary-free
namespace ui {

// Inheritable text defaults for a subtree. NOT a full TextVisual:
// align / wrap / line_height are per-Text and never inherited.
// Optionality is Opt<T> (the §3 convention) — no value-space sentinels.
struct TextStyleValue {
    Opt<Color>    color{};       // set => this layer forces ink (including transparent)
    Opt<uint16_t> font_id{};     // set => this layer forces a face
    Opt<uint16_t> font_size{};   // set => this layer forces a size
};

extern ReactContext TextStyleContext;   // current = const TextStyleValue*

} // namespace ui
```

Because presence is `Opt::set`, a provider can override only color (leaving `font_id`/`font_size` unset to inherit further up), and can even force transparent text deliberately (`color = opt(Color{0,0,0,0})`) — something a sentinel scheme cannot express. This is the §3 convention applied uniformly; there is no `color.a==0`-means-unset reading anywhere.

### 6.2 Text reads the default; the host commits the resolved value

`Text` is an ordinary component. It reads `TextStyleContext`, folds in the theme's `text` role + any local props, and emits its host element with a fully-dense `VisualStyle.text`. Resolution precedence for text ink/face, low→high: **theme `text.base` → inherited `TextStyleValue` → explicit `Text` props**. If, after all layers, no color was ever set, it falls back to `t.text_default` (never `{0,0,0,0}` — see §11.7):

```cpp
UiElement Text(const TextProps& p) {
    const Theme&          t   = use_theme();
    const TextStyleValue* inh = static_cast<const TextStyleValue*>(use_context(&TextStyleContext));

    VisualStyle vs = t.text.base;              // dense base (color = text_default by default)
    if (inh) {
        if (inh->color.set)     vs.text.color     = inh->color.value;
        if (inh->font_id.set)   vs.text.font_id   = inh->font_id.value;
        if (inh->font_size.set) vs.text.font_size = inh->font_size.value;
    }
    // Per-Text, never inherited:
    vs.text.align       = p.align;
    vs.text.wrap        = p.wrap;
    vs.text.line_height = p.line_height;
    if (p.color.set)     vs.text.color     = p.color.value;     // explicit author override wins
    if (p.font_size.set) vs.text.font_size = p.font_size.value;

    if (vs.text.color.a == 0) vs.text.color = t.text_default;   // safety: never invisible-by-omission
    return text_node(p.value, /*visual=*/vs);
}
```

The final `a == 0` guard reads the *resolved dense field* (legitimate resolved-output presence check, §3.6), not an authoring sentinel: it converts "nobody ever supplied ink" into the theme default rather than silently-transparent text. An author who *explicitly* set `color = opt({0,0,0,0})` gets exactly that (the guard only fires when no layer set color at all and `text.base` somehow had `a==0`).

### 6.3 Worked example: reproducing `inherited_disabled` dimming, no cascade

A disabled control dims its descendant text by providing a dimmed `TextStyleValue` around its subtree:

```cpp
UiElement DisabledableControl(const ControlProps& p) {
    const Theme& t = use_theme();
    InteractionState st { .hovered = use_hovered(), .pressed = use_pressed(),
                          .focused = use_focused(), .focus_visible = use_focus_visible(),
                          .disabled = p.disabled };
    VisualStyle vs = resolve(t.button, p.variant_patch, st);   // disabled patch folded into the box

    const TextStyleValue child_text { .color = opt(p.disabled ? t.text_disabled : t.text_default) };

    return box({
        .layout      = p.layout,
        .visual      = vs,
        .interaction = { .focusable = true, .disabled = p.disabled },
        .children    = with_text_default(child_text, p.children),
    });
}
```

`with_text_default` is the existing provider-element helper bound to `TextStyleContext`. The value must outlive the build pass, so copy it into the runtime's frame storage exactly as providers do today (`ui::copy_value`):

```cpp
UiElement with_text_default(const TextStyleValue& v, UiChildren children) {
    const TextStyleValue* stored = ui::copy_value(v);
    if (!stored) return ui::empty();
    return ui::provider("TextDefault", &ui::TextStyleContext,
                        const_cast<TextStyleValue*>(stored), children);
}
```

Three differences from the old `inherited_disabled` thread that matter:

1. **The dimming is decided once, where state lives** (the control, which knows `p.disabled`), not re-derived at every draw node.
2. **It composes with nesting for free.** Nested disabled controls just nest two providers; the inner one wins. The old OR-cascade could only monotonically darken; providers can also *re-lighten* a subtree (an enabled region inside a disabled panel) by providing `text_default` again.
3. **A control that composes its own label needn't provide at all.** It can set the label's `VisualStyle.text.color` directly. Use the provider when the dimmed default must reach *arbitrary author-supplied `children`*.

By paint time the dimmed `color` is already a literal field on each `Text` node's committed `VisualStyle`. There is **no `inherited_disabled` parameter, no per-node OR, and no `INHERITED_SET` build pass** anywhere.

### 6.4 Why a single-app hook UI does not want CSS inheritance

CSS inheritance exists to style documents the stylesheet author has never seen. This codebase is the inverse: a single closed app where the same author writes both component and call site, the tree is re-declared every frame, and composition is explicit. A general cascade would re-introduce a second resolution pass, action-at-a-distance/specificity debugging, and the sentinel regime §3/§4 ban. So inheritance is reframed as **provider scope, not selector reach**: only text ink + face inherit, they inherit through `use_context` like any other React default, and the resolved result is a dense field on the node by the time the renderer sees it.

---

## 7. Interaction: every-frame read

The runtime re-renders the entire tree every frame between `react_begin_frame()` and `react_end_frame()`. Interaction is therefore a **read**, not an event stream. During its normal render a component asks "what is my current interaction state?", builds an `InteractionState`, calls `resolve()` (§5), and passes the result as `.visual`. By the time `build_draw_list` runs, `.visual` is already final.

### 7.1 Source of truth: the existing per-frame focus/hit-test pass

The framework already computes per-frame interaction in `focus_update` (`src/ui/runtime/focus.cpp`) and exposes results by `NodeId`:

- `focus_focused_id(runtime)` → `NodeId` (`focus.h:82`) — the focused node.
- `focus_source(runtime)` → `FocusSource` (`focus.h:86`) — `None/Keyboard/Gamepad/Mouse/Touch/Programmatic` (`focus.h:19`).

The hit-test that maps the pointer to a node already exists (`hovered_enabled` returns the top-most enabled node under the pointer), and the pressed-origin node is tracked as `runtime.pointer_press_origin` (`focus.h:72`). **No new hit-test and no `on_pointer_*` events are introduced** — hovered and pressed are already computed every frame; they are simply not yet *exposed*.

### 7.2 Framework addition: expose hovered/pressed, derive focus_visible

`FocusRuntime` already stores `pointer_press_origin` (`focus.h:72`) and `focused_id` (`focus.h:69`). Add exactly **one** stored field (the hovered node) and **two** getters; pressed is derived from the existing press origin, with **no new stored `pressed_id`**:

```cpp
// focus.h — add to FocusRuntime (alongside focused_id):
NodeId hovered_id = 0;        // top-most enabled node under the pointer this frame

// focus.h — add getters next to focus_source:
NodeId focus_hovered_id(const FocusRuntime &runtime);   // returns hovered_id
NodeId focus_pressed_id(const FocusRuntime &runtime);   // returns pointer_press_origin
```

In `focus_update`, populate `hovered_id` from the value the function already computes (`hovered_enabled` returns `0` when the pointer is invalid, so a hover read costs nothing extra). `focus_pressed_id` returns `pointer_press_origin` directly.

**`focus_visible` is derived, not stored.** It comes from the existing `FocusSource`: focus is "visible" when the last focus change came from a non-pointer source.

```cpp
inline bool focus_source_is_visible(FocusSource s) {
  return s == FocusSource::Keyboard || s == FocusSource::Gamepad ||
         s == FocusSource::Programmatic;   // Mouse/Touch/None => not visible
}
```

These results are published once per frame into a context the hooks read:

```cpp
// ui/runtime — committed once per frame from the previous frame's focus results
struct InteractionSnapshot {
  NodeId focused=0, hovered=0, pressed=0;
  FocusSource source=FocusSource::None;
};
extern ReactContext InteractionContext;   // provided at the tree root each frame
```

### 7.3 Mapping a component fiber to its host id

A hook runs inside a component fiber. The interaction snapshot is keyed by host `NodeId`, so the component must know the `NodeId` of the host element it returns. That `NodeId` is **deterministic**: `begin_node`/`begin_keyed_node` derive it from the parent id plus the host's type/key, stable across frames for a stable fiber path.

`use_host_id()` therefore **computes the id directly** from the active fiber's parent-hash + sibling index — the same derivation `begin_node` uses — rather than depending on a reconciler write-back into a hook-indexed `use_ref` slot (the reconciler runs outside the fiber's hook sequence and cannot know which slot a hook allocated). This removes the correlation gap entirely: the hook and the commit independently compute the same id from the same inputs.

```cpp
// ui/runtime — returns the NodeId this fiber's host will commit this frame,
// computed from the fiber's deterministic identity (parent-hash + sibling index),
// matching begin_node's derivation. No reconciler write-back dependency.
NodeId use_host_id();
```

On the first frame the id is well-defined (the derivation does not need prior commit). For controls that compose their own host (Button, Input, Checkbox), the host id is the focusable node `focus_update` records, so the ids line up.

### 7.4 The hooks

Each hook reads the `InteractionSnapshot` and compares its host id. All are pure reads:

```cpp
namespace ui {

bool use_hovered() {
  const auto *s = static_cast<const InteractionSnapshot *>(use_context(&InteractionContext));
  NodeId me = use_host_id();
  return s && me != 0 && s->hovered == me;
}
bool use_pressed() {
  const auto *s = static_cast<const InteractionSnapshot *>(use_context(&InteractionContext));
  NodeId me = use_host_id();
  return s && me != 0 && s->pressed == me;
}
bool use_focused() {
  const auto *s = static_cast<const InteractionSnapshot *>(use_context(&InteractionContext));
  NodeId me = use_host_id();
  return s && me != 0 && s->focused == me;
}
bool use_focus_visible() {
  const auto *s = static_cast<const InteractionSnapshot *>(use_context(&InteractionContext));
  NodeId me = use_host_id();
  return s && me != 0 && s->focused == me && focus_source_is_visible(s->source);
}

} // namespace ui
```

### 7.5 One-frame lag is acceptable

`InteractionContext` is committed from the *previous* frame's `focus_update` results, and `use_host_id()` is stable across frames. So a hover/press/focus change is reflected in `.visual` on the **next** frame. Because the tree re-renders unconditionally every frame at frame rate, the one-frame delay is imperceptible. This is the deliberate, locked trade: it removes any need for retroactive style mutation, any `set_visual_style` setter, any pointer event, and any work in the paint pass.

### 7.6 What this section explicitly does NOT add

- **No new pointer events.** Hovered/pressed come from the existing `focus_update` hit-test.
- **Nothing in the paint pass.** `build_draw_list` does not read interaction, resolve styles, or consult the theme. The focus ring is a resolved `Outline` already on `.visual`; even the old `focused_id` argument to the transcriber is removed.
- **No value-space sentinels and no build-pass cascade.** Interaction selects which `StylePatch` layers `resolve()` applies; it never mutates a committed node.

### 7.7 `checked` and `active` are component-supplied (no framework hook)

`checked`, `active`, and `disabled` are **not** framework hit-test results — they are component/domain state the caller already holds, so they have **no hook**. The component fills them directly when building `InteractionState`:

- `disabled` — from a prop (`st.disabled = p.disabled`).
- `checked` — from a prop (`st.checked = p.checked`), used by `Checkbox`/toggle controls.
- `active` — **component-supplied, mirroring `checked`**. There is deliberately no framework signal for it. A component sets `st.active` when *it* defines an "actively engaging" condition (e.g. a tab that is the current tab, a segmented-control segment that is selected-and-engaged, a menu item whose submenu is open). It sits just below `disabled` in precedence and exists so a component can carry one extra app-defined visual state through the same resolve chain without inventing a parallel mechanism. Example:

```cpp
// A tab marks itself active when it is the selected tab.
InteractionState st{
    .hovered = use_hovered(), .focused = use_focused(),
    .focus_visible = use_focus_visible(),
    .active = (p.index == p.selected_index),   // component decides; no hook
    .disabled = p.disabled,
};
VisualStyle vs = resolve(t.button, tab_variant_patch(t), st);
```

A component that has no "active" concept simply leaves `st.active = false`, and `role.active` (default empty in the theme) never applies.

**Files touched:** `src/ui/runtime/focus.h`/`focus.cpp` (add `hovered_id`, `focus_hovered_id`, `focus_pressed_id`, `focus_source_is_visible`), a new `src/ui/runtime/interaction.{h,cpp}` (or `src/ui/style/`) for `InteractionSnapshot`, `InteractionContext`, `use_host_id`, and the four hooks, and `src/client/ui/client_ui.cpp` (publish the snapshot from the focus getters). All stay SDL-free and game-vocabulary-free.

---

## 8. DrawCommand IR

The IR is the only artifact that crosses the `ui/`→`renderer/` seam besides the injected `MeasureTextFn`. It replaces the two-arm `DrawCommand` and the `UI_RETAINED_MAX_DRAW_COMMANDS = UI_RETAINED_MAX_NODES * 2` array (`src/ui/runtime/draw_list.h:10`), and the inline text payload. `build_draw_list` becomes a pure transcriber: every byte it writes already exists on a node snapshot (its resolved `VisualStyle`) or in the runtime string arena.

### 8.1 Header + tagged-union shape

One `DrawCommand` = a fixed header followed by a union of POD arms. The header carries `kind`, `node_id`, and an **IR-local** geometry rect `DrawRect` (deliberately *not* the heavyweight layout `Rect` at `src/ui/runtime/tree.h:215`, which drags `ComputedEdgeSizes`). The IR rect is the resolved paint box already computed during layout.

```cpp
// src/ui/runtime/draw_list.h
struct DrawRect { float x=0, y=0, w=0, h=0; };   // IR-local; NOT tree.h Rect

enum class DrawCommandKind : uint8_t {
  None=0, Rect, Border, Text, Image, Gradient, Shadow,
  ClipPush, ClipPop, LayerPush, LayerPop,
  Custom /* reserved seat: no payload, no renderer path */
};

// --- POD arms. Each is an aggregate; none owns memory. ---
struct RectData     { Color fill{}; float corner_radius=0; };           // first arm: DMI present
struct BorderData   { Border border{}; Outline outline{}; Color fill{}; // fused fill+border+outline
                      float corner_radius=0; bool has_fill=false; bool has_outline=false; };
struct TextData     { uint32_t text_off=0; uint16_t text_len=0;         // slice into DrawList::text_arena
                      Color color{}; uint16_t font_id=0; uint16_t font_size=0;
                      uint16_t line_index=0; TextAlign align=TextAlign::Left; };
struct ImageData    { uint32_t texture_id=0; Color tint{255,255,255,255};
                      SideWidths nine_slice{}; float corner_radius=0; };
struct GradientData { uint16_t stop_off=0; uint8_t stop_count=0;        // slice into DrawList::grad_arena
                      float angle_deg=0; float corner_radius=0; };
struct ShadowData   { Color color{}; Vec2 offset{}; float blur=0;
                      float spread=0; float corner_radius=0; };
struct ClipData     { float corner_radius=0; };                         // clip rect = header.rect
struct LayerData    { float opacity=1; };
// Custom: NO payload arm. The enum seat exists; the union has no Custom member.

union DrawPayload {
  RectData     rect{};       // active member of the default ctor; keeps DrawPayload an aggregate
  BorderData   border;
  TextData     text;
  ImageData    image;
  GradientData gradient;
  ShadowData   shadow;
  ClipData     clip;
  LayerData    layer;
};

struct DrawCommand {
  DrawCommandKind kind = DrawCommandKind::None;   // header
  NodeId          node_id = 0;
  DrawRect        rect{};
  DrawPayload     payload{};                       // first union member default-initialized
};
```

**Why the first arm carries a default member initializer** (decision 9): a union is an aggregate only if it has no user-provided/deleted constructors; giving its *first* member a default member initializer (`RectData rect{}`) supplies the implicit default constructor without making the union non-aggregate, so designated-initializer call sites keep compiling:

```cpp
list.push({ .kind = DrawCommandKind::Rect, .node_id = node.id, .rect = paint_box,
            .payload = { .rect = { .fill = vs.background, .corner_radius = vs.corner_radius } } });
```

**The reserved `Custom` seat has no payload arm.** Per decision 9 ("reserved enum seats only, no payloads"), `Custom` is an enum value with no union member and no renderer path. This keeps the cut typed-custom arena out of the IR entirely.

### 8.2 Static asserts and the size budget

The IR is a value type the renderer memcpy-iterates; it must never run a destructor or copy ctor, and aggregate-init is the only construction path:

```cpp
static_assert(std::is_aggregate_v<DrawCommand>,         "DrawCommand must stay an aggregate");
static_assert(std::is_trivially_copyable_v<DrawCommand>,"DrawCommand must be trivially copyable");
```

These hold because every type involved is an aggregate of scalars/aggregates with default member initializers only — no `std::function`, no owning buffers. They also forbid silently smuggling a heap pointer into an arm later.

**Sizeof is asserted exactly, not approximated.** The largest union arm bounds `DrawPayload`; `BorderData` (a `Border` = 4 floats + 4 `Color` = 16 + 16 = 32 bytes, plus `Outline` = float+Color+float = 12, plus `Color fill` = 4, plus float + 2 bools = 6, padded) is the largest. The header is `DrawCommandKind`(1) + padding + `NodeId`(`uint64_t`, 8 — `tree.h:11` — forcing 8-byte struct alignment) + `DrawRect`(16), i.e. a 32-byte header, so `sizeof(DrawCommand)` ≈ 32 + 56 = 88. Rather than hand-computing, pin it with a static_assert so the budget rests on a real number, not an estimate:

```cpp
static_assert(sizeof(DrawPayload) <= 56, "largest union arm grew; re-check the budget");
static_assert(sizeof(DrawCommand) <= 96, "DrawCommand size budget; update UI_DRAWLIST_BUDGET if intentional");
```

(The exact values are fixed at first build by tightening the bounds to the measured `sizeof`; the asserts then guard against silent growth.)

### 8.3 Out-of-line arenas (offsets, never pointers)

Variable-length data lives in two side arenas on the `DrawList`; arms hold integer *handles*, never pointers — this keeps `DrawCommand` trivially copyable and the list relocatable.

- **Text bytes** — `text_arena`: the renderer slices `text_arena[text_off .. text_off+text_len)`. Bytes are *copied from the runtime's shared string arena* (§10.3) at transcribe time. This deletes the old `copy_text`/`strncpy` clamp.
- **Gradient stops** — `grad_arena`: sliced `[stop_off .. stop_off+stop_count)`. Inlining was rejected — a `GradientStop stops[8]` per command would bloat every non-gradient command.

```cpp
constexpr int UI_DRAW_TEXT_ARENA_BYTES = 8 * 1024;     // shared line-text budget for one frame
constexpr int UI_DRAW_GRAD_STOPS       = 256;          // total stops across all gradients this frame
```

Per-line `Text` commands reference disjoint slices: the injected `MeasureTextFn` returns `LineRun{slice_offset, slice_len, x, y, w, h}` with alignment pre-baked (§10), and the transcriber copies each line's bytes once, emitting one `Text` command per `LineRun` carrying that slice plus `line_index`. Caret/selection rects (formerly `kCharWidth=8.0f`, `draw_list.cpp:120`) become plain `Rect` commands positioned from measured advances.

### 8.4 Premultiplied-alpha emit rule (the single conversion point)

**All authored `Color` values — in `Theme`, `StylePatch`, props, and the resolved `VisualStyle` — are straight alpha.** Premultiplication happens **exactly once**, at the IR `push` path, the moment a color is copied into a command arm or arena. There is no other conversion point, so double-premultiply is structurally impossible.

`(0,0,0,0)` is therefore the only transparent color in the IR; there is no `a==0`-as-"unset" reading. The renderer composites premultiplied end-to-end, so co-feathered fill+border land in one pass and `LayerPush/Pop` composites stay correct.

```cpp
// Private to the transcriber TU. Applied at every emit; never anywhere else.
constexpr Color premul(Color c) {
  return { uint8_t(c.r * c.a / 255), uint8_t(c.g * c.a / 255), uint8_t(c.b * c.a / 255), c.a };
}
```

### 8.5 Hierarchical z-resolution DURING the DFS (no flat sort)

There is **no global flat sort**. Ordering is the emission order of a single depth-first walk of the snapshot tree. Painter's algorithm = document order. Within one node the transcriber emits a fixed back-to-front order so a node's own pixels are self-consistent regardless of siblings:

```
Shadow → fill/Gradient/Image → [children…] → Border+Outline (one fused frame command)
```

(Background paints before children; the frame — border and focus-ring outline — paints *after* children so it frames content. Fill and frame are separate commands; the frame is one `Border`-kind command carrying both border and outline so the renderer feathers them with the fill in a single pass, §9.4.)

A node opens a stacking context when it clips (`Overflow::Hidden`) or when `visual.opacity < 1` (group opacity → render-to-target). Because resolution happens *during* the DFS, the push and its matching pop wrap exactly that node's subtree emission — brackets are contiguous and balanced by construction, with zero bookkeeping:

```cpp
bool transcribe(const UiTree& t, DrawList& dl, NodeId id, const DrawRect& box) {
  NodeSnapshot n; if (!t.snapshot(id, &n)) return false;
  if (n.visual.hidden) return true;                       // skip paint, keep layout
  const bool clip  = n.layout.overflow == Overflow::Hidden;
  const bool layer = n.visual.opacity < 1.0f;
  if (layer && !dl.push({ .kind=DrawCommandKind::LayerPush, .node_id=n.id, .rect=box,
                          .payload={ .layer={ .opacity=n.visual.opacity } } })) return false;
  if (clip  && !dl.push({ .kind=DrawCommandKind::ClipPush,  .node_id=n.id, .rect=box,
                          .payload={ .clip ={ .corner_radius=n.visual.corner_radius } } })) return false;

  emit_background(dl, n, box);                             // Shadow → Rect/Gradient/Image
  for (int i = 0; i < t.child_count(id); ++i)
    if (!transcribe(t, dl, t.child_at(id, i), child_box(...))) return false;
  emit_frame(dl, n, box);                                 // one Border cmd (border + outline)

  if (clip  && !dl.push({ .kind=DrawCommandKind::ClipPop,  .node_id=n.id })) return false;
  if (layer && !dl.push({ .kind=DrawCommandKind::LayerPop, .node_id=n.id })) return false;
  return true;
}
```

The renderer (§9) is a dumb linear executor: `ClipPush/Pop` and `LayerPush/Pop` drive fixed stacks; their balance is guaranteed upstream, so it asserts depth rather than recovering. The signed-offset focus ring is part of the fused frame command, sourced from `n.visual.outline` — the runtime no longer overrides border to a `kFocusBorder` constant.

### 8.6 Per-frame reset = cursors only

The new reset zeroes **cursors only**; the backing arrays are overwritten in place up to the new high-water mark, never memset:

```cpp
void DrawList::reset() { count = 0; text_len_used = 0; grad_count = 0; error_count = 0; }
```

Stale bytes past `count`/`text_len_used`/`grad_count` are unreachable because the renderer iterates `[0,count)` and every arm reads only its own arena slice. Correctness must not rely on zeroed slots.

### 8.7 Derived capacity + the byte budget

Capacity is **derived from the v1 used set, not a Clay worst case**, and crucially **not** by assuming every node is simultaneously a full box *and* an 8-line text node. A node is either a box-like node (fill/gradient/image + frame, plus optional shadow and clip/layer brackets) **or** a text node (up to `UI_TEXT_LINE_CAP` line commands). We budget per node as the **max** of those two shapes, not their sum.

```cpp
constexpr int UI_TEXT_LINE_CAP = 8;     // small line cap; not 64-line machinery

// Box-like node worst case, by actual emitted command KINDS:
//   Shadow(1) + fill-or-gradient-or-image(1) + frame[border+outline fused](1)
//   + ClipPush/ClipPop(2) + LayerPush/LayerPop(2) = 7.
// (Rect and Gradient are mutually exclusive fills; border+outline are ONE fused
//  command; Outline has no separate enum seat — so the count is 7, not 10.)
constexpr int UI_MAX_CMDS_PER_BOX_NODE = 7;

// Text node worst case: up to UI_TEXT_LINE_CAP Text commands + optional clip brackets.
constexpr int UI_MAX_CMDS_PER_TEXT_NODE = UI_TEXT_LINE_CAP + 2;   // 10

// Per node we budget the larger shape, not the sum.
constexpr int UI_MAX_CMDS_PER_NODE = (UI_MAX_CMDS_PER_BOX_NODE > UI_MAX_CMDS_PER_TEXT_NODE)
                                   ? UI_MAX_CMDS_PER_BOX_NODE : UI_MAX_CMDS_PER_TEXT_NODE;  // 10

// Not every node is a max-text node. A realistic v1 frame is mostly box nodes with a
// minority of multi-line text. Budget the whole tree at the per-node max anyway (256
// nodes is already small) but compute it from the real per-node max (10), not 18.
constexpr int UI_MAX_DRAW_COMMANDS = UI_RETAINED_MAX_NODES * UI_MAX_CMDS_PER_NODE;  // 256 * 10 = 2560
```

`UI_RETAINED_MAX_NODES = 256` (`src/ui/runtime/tree.h:13`). The `text_arena` (8 KiB) and `grad_arena` (256 stops) are one shared budget each, sized from a realistic frame, overflow-surfaced (§8.8), never a per-node inline array.

```cpp
struct DrawList {
  std::array<DrawCommand, UI_MAX_DRAW_COMMANDS> commands{};
  std::array<char,         UI_DRAW_TEXT_ARENA_BYTES> text_arena{};
  std::array<GradientStop, UI_DRAW_GRAD_STOPS>       grad_arena{};
  int count = 0, text_len_used = 0, grad_count = 0, error_count = 0;

  bool push(const DrawCommand& c);                                       // bounds-checks count
  bool push_text(const char* bytes, uint16_t len, uint32_t* out_off);    // bounds-checks text_arena
  bool push_stops(const GradientStop* s, uint8_t n, uint16_t* out_off);  // bounds-checks grad_arena
  void reset();
};

// 2560 cmds * ~88 B ≈ 220 KiB + 8 KiB text + ~3 KiB stops; budget gives headroom
// without inviting a Clay-scale array.
constexpr std::size_t UI_DRAWLIST_BUDGET = 256 * 1024;
static_assert(sizeof(DrawList) < UI_DRAWLIST_BUDGET, "DrawList exceeds its byte budget");
```

### 8.8 Overflow → error_count → failed frame (never silent)

Every variable-capacity write is bounds-checked and bumps `error_count` on overflow, mirroring the existing `DrawList::push` contract (`draw_list.cpp:217-224`) and propagating exactly like today: `build_draw_list` returns success only when `error_count == 0` (`draw_list.cpp:230-231`), making overflow a **test-visible failed frame**.

```cpp
bool DrawList::push(const DrawCommand& c) {
  if (count >= UI_MAX_DRAW_COMMANDS) { ++error_count; return false; }
  commands[count++] = c; return true;
}
bool DrawList::push_text(const char* bytes, uint16_t len, uint32_t* out_off) {
  if (text_len_used + len > UI_DRAW_TEXT_ARENA_BYTES) { ++error_count; return false; }
  *out_off = uint32_t(text_len_used);
  for (uint16_t i = 0; i < len; ++i) text_arena[text_len_used++] = bytes[i];
  return true;
}
```

Command-array overflow, text-arena overflow, gradient-stop overflow, and a per-text-node line count exceeding `UI_TEXT_LINE_CAP` all resolve to the same outcome: `++error_count`, return `false`, fail the frame. No truncation, no `strncpy` clamp, no value clamp.

---

## 9. Renderer (SDL executor)

The renderer is a **dumb linear executor** over the resolved `DrawCommand` array. It owns no styling logic, no theme, no resolution: it switches on `DrawCommandKind` and emits SDL calls. `ui/` stays SDL-free; the renderer is the only place SDL geometry is touched. This replaces `SdlRetainedRenderer::render` and its helpers (`src/renderer/sdl_retained_renderer.cpp:39-114`).

### 9.1 Executor shape

`render()` walks `draw_list.commands[0..count)` once, in emission order. There is **no global flat sort**: z-resolution already happened hierarchically during the DFS, so `ClipPush..ClipPop` and `LayerPush..LayerPop` brackets arrive contiguous and balanced. The executor maintains two fixed stacks (clip, layer). It never allocates.

```cpp
// src/renderer/sdl_retained_renderer.h
class SdlRetainedRenderer {
public:
  bool initialize(SDL_Renderer *renderer, FontRegistry &fonts);
  void clear(::ui::Color background);
  void render(const ::ui::DrawList &draw_list);
  void present();
private:
  void exec_rect(const ::ui::DrawCommand &c);
  void exec_border(const ::ui::DrawCommand &c);     // fused border+outline, co-feathered
  void exec_text(const ::ui::DrawCommand &c, const ::ui::DrawList &dl);
  void exec_image(const ::ui::DrawCommand &c);
  void exec_gradient(const ::ui::DrawCommand &c);
  void exec_shadow(const ::ui::DrawCommand &c);
  void clip_push(const ::ui::DrawCommand &c);
  void clip_pop();
  void layer_push(const ::ui::DrawCommand &c);
  void layer_pop();

  SDL_Renderer *renderer_ = nullptr;
  FontRegistry *fonts_    = nullptr;
  static constexpr int kClipStackMax  = 16;
  static constexpr int kLayerStackMax = 8;
  SDL_Rect clip_stack_[kClipStackMax]{};
  int      clip_depth_ = 0;
  struct LayerSlot { SDL_Texture *target; float opacity; SDL_Rect bounds; };
  LayerSlot layer_stack_[kLayerStackMax]{};
  int       layer_depth_ = 0;
};
```

`Custom` and the reserved gradient/shadow sub-modes have **enum seats only** — they fall through to no-ops. Stack overflow is a programmer error guarded by `SDL_assert` in debug and a hard no-op push in release; the *capacity* side of overflow is already an `error_count` failed frame upstream.

### 9.2 Premultiplied alpha end-to-end

The whole pipeline is premultiplied at the IR boundary (§8.4), so by the time a color reaches the renderer it is already premultiplied. `(0,0,0,0)` is the only transparent color. The renderer sets premultiplied blending once at init:

```cpp
bool SdlRetainedRenderer::initialize(SDL_Renderer *r, FontRegistry &f) {
  if (!r || !f.default_font()) return false;
  renderer_ = r; fonts_ = &f;
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
  return true;
}
```

Geometry colors are fed verbatim — no straight-alpha modulation. Two *external* alpha sources are straight-alpha and must be converted **once, at upload/cache**, never per-frame:

- **TTF glyph surfaces.** `TTF_RenderText_Blended` produces straight-alpha. After creating the glyph/line texture, multiply RGB by A (or `SDL_PremultiplyAlpha`) before `SDL_CreateTextureFromSurface`, then set `SDL_BLENDMODE_BLEND_PREMULTIPLIED`. The font registry owns this at cache insert.
- **stb_image textures.** Convert to premultiplied at upload in the texture cache. `texture_id==0` means "no image."

### 9.3 Rounded fill — uniform radius only

`exec_rect` paints the fill (and, when `gradient.stop_count != 0`, defers to `exec_gradient` — gradient and solid fill are distinct commands). v1 supports **uniform** `corner_radius` only. When `corner_radius <= 0.5f` the fill is a single `SDL_RenderFillRect`. Otherwise the fill is tessellated as **center quad + 4 corner fans + 4 edge quads** into one `SDL_RenderGeometry` call:

```cpp
int seg = std::max(16, (int)std::ceil(r * 0.5f));   // segments per 90° corner
```

All vertices carry the premultiplied fill color (or per-vertex gradient colors, §9.6). This replaces the `SDL_RenderFillRect` at `sdl_retained_renderer.cpp:65`.

### 9.4 Frame: border + outline, co-feathered with fill, single pass

The old per-inset `SDL_RenderRect` loop (`sdl_retained_renderer.cpp:75-85`) is deleted. The fused frame command (`Border` kind, carrying `Border border`, `Outline outline`, and the fill for co-feathering) is emitted as **geometry co-feathered with the fill in one `SDL_RenderGeometry` pass**, giving a single shared AA fringe on the outer edge instead of stair-stepped nested rects.

Construction: build the fill ring (§9.3) and, for each side with `width.<side> > 0`, append a border band quad-strip inset from the border-box edge, colored with `color.<side>`. Corners use the same `seg`-tessellated arc, split radially between adjacent side colors at the 45° bisector. Because there is one outer silhouette, the AA fringe is generated once around the union and submitted in the same batch — no double-blended seam. Unequal-width CSS miter `atan2` math is cut; the radial 45° split is the v1 join. A border on a node with no fill still emits just the band geometry. The signed-offset outline (§9.11) is appended to the same geometry batch.

### 9.5 (Border covered by §9.4)

### 9.6 Linear gradient — per-vertex color

`exec_gradient` reuses the **same fill tessellation as §9.3** but computes each vertex's color by projecting the vertex onto the gradient axis: axis direction `d = (cos(angle), sin(angle))`, project to get `t ∈ [0,1]`, then interpolate between bracketing `GradientStop`s (pre-sorted by `t`). Color is interpolated in **premultiplied space**. One `SDL_RenderGeometry` call, `texture=nullptr`, colors per-vertex. `stop_count == 0` ⇒ no gradient command. Radial gradient is a reserved seat only.

### 9.7 Image — plain / nine-slice / rounded

`exec_image` resolves `texture_id` against the renderer-side texture cache (premultiplied, §9.2). Three paths:

- **Plain** (`nine_slice` all zero, `corner_radius <= 0.5`): one `SDL_RenderTexture`. Tint via `SDL_SetTextureColorMod`/`AlphaMod`, **restored to white immediately after** so cached textures aren't left modulated.
- **Nine-slice** (`nine_slice` non-zero): **9× `SDL_RenderTexture`** — 4 corners 1:1, 4 edges stretched along one axis, 1 center stretched both. Source/dest rects derived from `nine_slice` widths. Corners never scaled. Nine-slice + rounded corners is **cut** (radius ignored).
- **Rounded** (`corner_radius > 0.5`, no nine-slice): the §9.3 tessellation with `texture=tex`, per-vertex UVs, tint folded into per-vertex color. One `SDL_RenderGeometry` call.

### 9.8 Drop shadow — feathered quads

`exec_shadow` (drop only; inset is a reserved seat) runs *before* the node's fill. `color.a == 0` ⇒ no shadow command. The shadow box is the border-box translated by `offset` and expanded by `spread`. Construction: a solid inner quad at full `color`, surrounded by a `blur`-wide skirt fading to premultiplied-transparent. One `SDL_RenderGeometry` call. When `corner_radius > 0`, the skirt corners reuse the `seg`-arc tessellation. No gaussian sprite cache (cut).

### 9.9 Clip — integer round-out stack

`ClipPush` rounds its rect **out** to integers (floor min, ceil max — never clip a wanted sub-pixel edge), intersects with the current top, pushes, and calls `SDL_SetRenderClipRect`:

```cpp
void SdlRetainedRenderer::clip_push(const ::ui::DrawCommand &c) {
  SDL_Rect r = round_out(c.rect);
  if (clip_depth_ > 0) SDL_GetRectIntersection(&clip_stack_[clip_depth_-1], &r, &r);
  clip_stack_[clip_depth_++] = r;
  SDL_SetRenderClipRect(renderer_, &r);
}
void SdlRetainedRenderer::clip_pop() {
  if (--clip_depth_ <= 0) { clip_depth_ = 0; SDL_SetRenderClipRect(renderer_, nullptr); }
  else                    { SDL_SetRenderClipRect(renderer_, &clip_stack_[clip_depth_-1]); }
}
```

Every executor path runs under the active clip, so fills/borders/images/text are clipped for free by SDL.

### 9.10 Group opacity — single naive render-to-target

`VisualStyle.opacity < 1` emits a `LayerPush`/`LayerPop` bracket. v1 is the **single naive render-to-target composite** — explicitly **no** pool, **no** size-bucket hysteresis, **no** VRAM budget, **no** per-frame switch cap, **no** degraded fallback (all cut).

**Layer target sizing rule:** the layer target is sized to the **`LayerPush` header rect** (`DrawRect`), which is the node's border-box. Children that draw outside that rect are clipped to it by the target's bounds — the layer is a stacking context, and content overflowing a group-opacity node's box is clipped to that box (consistent with how opacity groups behave as composited units). The target is a **per-frame transient**, created within the `LayerPush`/`LayerPop` bracket and released at `LayerPop` (or one reused full-viewport-size scratch indexed by `layer_depth_`); it is explicitly **not** a size-bucketed retained pool.

```cpp
void SdlRetainedRenderer::layer_push(const ::ui::DrawCommand &c) {
  SDL_Rect bounds = round_out(c.rect);                       // node border-box
  SDL_Texture *target = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
      SDL_TEXTUREACCESS_TARGET, bounds.w, bounds.h);         // transient; freed at pop
  SDL_SetTextureBlendMode(target, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
  layer_stack_[layer_depth_++] = { target, c.payload.layer.opacity, bounds };
  SDL_SetRenderTarget(renderer_, target);
  SDL_SetRenderClipRect(renderer_, nullptr);
  SDL_SetRenderDrawColor(renderer_, 0,0,0,0); SDL_RenderClear(renderer_);  // (0,0,0,0)
}
void SdlRetainedRenderer::layer_pop() {
  LayerSlot s = layer_stack_[--layer_depth_];
  SDL_SetRenderTarget(renderer_, layer_depth_ ? layer_stack_[layer_depth_-1].target : nullptr);
  SDL_SetRenderClipRect(renderer_, clip_depth_ ? &clip_stack_[clip_depth_-1] : nullptr);
  SDL_SetTextureBlendMode(s.target, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
  SDL_SetTextureAlphaMod(s.target, (Uint8)(s.opacity * 255.0f + 0.5f));
  SDL_FRect dst = to_frect(s.bounds);
  SDL_RenderTexture(renderer_, s.target, nullptr, &dst);
  SDL_DestroyTexture(s.target);                              // transient: no retention
}
```

Because everything is premultiplied, scaling the whole layer's alpha is a correct, clean composite. All SDL state (render target, clip, texture mod) is saved before and restored after so layers nest correctly.

### 9.11 Signed-offset focus ring (Outline)

The `Outline` carried in the fused frame command (§9.4) has a **signed offset**: `offset < 0` insets, `offset > 0` outsets; the default focus ring is small positive (outset). `width <= 0` or `color.a == 0` ⇒ no ring. The ring's outer rect is the border-box grown by `offset` on every side (negative grows inward); the ring is a `width`-thick band on that rect, with corner radius `corner_radius + offset` (clamped at 0) so an outset ring's curvature tracks the box. It is emitted after the node's fill/border so it sits on top. It is the resolved visual of focus (`InteractionState.focus_visible`), not a special renderer mode; the ring color token `Theme.focus_ring` was already resolved into `VisualStyle.outline` at authoring time.

### 9.12 What the executor does NOT do

No resolution, no theme lookup, no cascade, no hit-testing, no interaction state, no global sort, no per-frame `memset`. Presence is `stop_count`/`texture_id`/`a==0` on the *resolved* `VisualStyle` only — a resolved-output presence check, distinct from authoring `Opt<T>::set`. The renderer is a pure transcription of the `DrawCommand` list into SDL geometry, and nothing more.

---

## 10. Text model + measure seam

### 10.1 One measurer, two callers

There is exactly **one** text measurement function, a free function pointer installed once at startup and invoked from two places that must agree to the pixel:

1. **Layout** — the Yoga `MeasureFn` adapter (`src/ui/runtime/yoga_flex_layout.cpp`, via `tree.measure` → the per-node `MeasureFn = Size(*)(MeasureInput, void*)`, `tree.h:378`). For text nodes the per-node `MeasureFn` is a thin shim that builds a `TextMetricsQuery` and calls the global measurer, returning `{width,height}` to Yoga. Wrapping at the Yoga-provided `AtMost` width drives node height.
2. **Paint** — `build_draw_list` calls the *same* measurer with the *same* query (keyed to the laid-out content width) for per-line geometry, then emits one `Text` command per line.

Because both callers run identical code on identical inputs, **measure == draw** by construction — the structural fix for the old `strlen*kCharWidth` caret bug. `ui/` stays SDL-free: the measurer is the single function-pointer seam; the renderer (which owns SDL_ttf via `FontRegistry`) supplies the implementation.

### 10.2 The seam types (live in `ui/`, SDL-free)

```cpp
// src/ui/style/text_measure.h  (ui/ owns these, no SDL)
namespace ui {

constexpr int UI_MAX_TEXT_LINES = 8;   // small cap

struct LineRun {
    uint32_t slice_offset = 0;   // byte offset into TextMetricsQuery::utf8
    uint32_t slice_len    = 0;   // byte length of this line's visible glyphs
    float    x = 0, y = 0;       // top-left pen origin, alignment applied
    float    w = 0, h = 0;       // measured advance width, line box height
};

struct TextMetricsQuery {
    const char* utf8 = nullptr;  // borrowed; points into the shared string arena
    uint32_t    len  = 0;        // byte length
    uint16_t    font_id   = 0;
    uint16_t    font_size = 0;
    TextAlign   align = TextAlign::Left;
    TextWrap    wrap  = TextWrap::None;
    float       line_height = 0; // 0 => face's natural line skip
    float       wrap_width  = 0; // <=0 or wrap==None => single line / no wrap
};

struct TextMetricsResult {
    float    width  = 0;         // max line w (the node's content width)
    float    height = 0;         // sum of line h
    uint8_t  line_count = 0;     // <= UI_MAX_TEXT_LINES
    LineRun  lines[UI_MAX_TEXT_LINES] = {};
    bool     overflowed = false; // wrapped output needed > UI_MAX_TEXT_LINES lines
};

using MeasureTextFn = TextMetricsResult (*)(const TextMetricsQuery&);

void          set_text_measurer(MeasureTextFn fn);   // install once at startup
MeasureTextFn text_measurer();                        // null until installed

} // namespace ui
```

`TextMetricsResult` is a fixed-size aggregate. `LineRun::slice_*` index back into the caller-owned `utf8`, so the result carries no text bytes.

### 10.3 Source text in the shared string arena (cap lifted)

Today node text lives in `char value[UI_RETAINED_VALUE_CAP]` (`UI_RETAINED_VALUE_CAP = 96`, `tree.h:17,462`) via a `strncpy`-style clamp. That 95-byte ceiling is removed for renderable text content:

- The runtime gains a **shared string arena** — one contiguous byte buffer, per-frame bump-allocated. A `Node` stores text as `(uint32_t text_offset, uint32_t text_len)` into that arena instead of `value[96]`.
- Its capacity is an explicit named constant, derived the same way as the draw-list arenas (a shared whole-frame budget, not a per-node cap):

```cpp
// src/ui/runtime/tree.h — runtime source-text arena (replaces value[96] for renderable text).
// Derivation: a realistic frame's total renderable text. 256 nodes * ~64 avg bytes ≈ 16 KiB.
constexpr int UI_RETAINED_TEXT_ARENA_BYTES = 16 * 1024;
```

- `TextMetricsQuery::utf8 = arena.base + node.text_offset`, `len = node.text_len`. The measurer only sees `(ptr,len)`.
- **Overflow is a failed frame, never a clamp.** If a node's text does not fit the remaining arena budget, the runtime calls `report_error()` (the existing `error_count_` path, `tree.h:494,503`). The old silent `strncpy` truncation is deleted; CLI smoke tests assert `error_count()==0`.

The fixed-cap accessibility/control fields (`accessibility_label`, control `value`, `composition`, `tree.h:460-463`) keep their `*_CAP` arrays — only **renderable text content** moves to the arena.

### 10.4 Per-line emission in `build_draw_list`

For a text node `build_draw_list` (a pure transcriber):

1. Reads the node's resolved `TextVisual` (committed as part of `VisualStyle::text`) and its laid-out rect.
2. Builds a `TextMetricsQuery` with `wrap_width = content_width(rect)` (border-box minus padding/border).
3. Calls `text_measurer()(query)` once.
4. If `result.overflowed`, calls the draw list's error path and emits the lines that fit — the frame is marked failed; there is no silent truncation beyond `UI_MAX_TEXT_LINES`.
5. For each `LineRun`, emits **one `Text` `DrawCommand`** whose `rect = {line.x, line.y, line.w, line.h}` and whose bytes are the slice copied into the draw list's text arena (`text_off/text_len`). `node_id` is propagated so clip/z bracketing wraps the lines.

The renderer draws each `Text` command at its own pre-aligned rect — no layout, alignment, or wrapping.

### 10.5 Caret / selection from measured advances

Caret/selection geometry comes from measured advances, **never** a fixed char width. The text-edit metadata carries byte indices (`TextEditMetadata{caret, selection_start, selection_end}`, `tree.h:319`). To turn a byte index into x:

- Find the `LineRun` whose `[slice_offset, slice_offset+slice_len)` contains the caret byte index.
- Issue a **sub-slice query**: same query but `utf8 = line_base`, `len = caret - line.slice_offset`, `wrap = None`. The returned `width` is the exact pen advance; caret x = `line.x + that_width`.
- Selection rectangles are the advance spans between start and end, clipped per line.

**UTF-8 / boundary handling:** caret and selection byte indices are required to fall on **codepoint boundaries** (the text-edit logic that maintains them already advances by whole codepoints; v1 does not split inside a multi-byte sequence). The measurer measures the byte sub-slice up to that boundary, so a multi-byte codepoint contributes its full glyph advance. v1 treats one codepoint as one cluster for caret placement (no combining-mark/grapheme-cluster merging); the real shaper (§10.6) still shapes the whole line correctly for rendering, but caret stops are per-codepoint. The deterministic test mock (§13) uses a fixed per-glyph advance and 1-byte ASCII fixtures, so its byte==codepoint==advance assumption is fixture-scoped, not a runtime guarantee. An index that does not fall on a codepoint boundary is a caller bug; v1 measures to the nearest preceding boundary rather than splitting a sequence.

### 10.6 Renderer-side implementation: retained `TTF_Text` cache

The renderer maintains a **retained `TTF_Text` cache** so it neither re-shapes every frame nor leaks. The current code creates a fresh surface+texture every draw (`sdl_retained_renderer.cpp:97-113`); that is replaced.

- **Cache key:** `(node_id, line_index, face_id, font_size, style_flags)`. **The arena offset is NOT part of the key** (it churns every frame as the bump allocator resets).
- **Change-gate by content fingerprint.** Each entry stores `fnv1a(utf8 slice) ^ mix(font_id, font_size, align, wrap, wrap_width_quantized)`. On lookup, if key and fingerprint match, the retained `TTF_Text` is reused (only its draw position changes). If the fingerprint differs, the entry is rebuilt (`TTF_DestroyText` + re-create).
- **Font-size change = rebuild, not `SetFontSize` on a shared font.** The current path mutates one shared `default_font()` via `TTF_SetFontSize` (`sdl_retained_renderer.cpp:93-95`) — a shared-mutable-state bug. The new model keys fonts by `(face_id, font_size)` in `FontRegistry` (a small `TTF_Font*` map, open-on-demand, replacing the single `default_font_`). A size change selects a different `TTF_Font*` and rebuilds the `TTF_Text`.
- **Mark-and-sweep eviction.** Clear `used` at frame start; set `used` on hit/build; at frame end destroy entries still `used==false`. Nodes that unmount (tracked via `unmounted_at`, `tree.h`) drop their lines naturally. This bounds VRAM without any pool/budget/hysteresis machinery (cut).

The renderer-side `MeasureTextFn` shapes a `TTF_Text` (or measures via `TTF_GetStringSize`) to fill `LineRun`s. `TextWrap::Words` breaks at `wrap_width`; `None` returns a single line. Shaping done during measure is stashed in the same cache, so layout-time and paint-time measure share the work — expensive shaping happens once per content-change, not twice per frame.

### 10.7 Clip honored by text

Text respects the active clip rect set by `ClipPush` (§9.9): `SDL_SetRenderClipRect` clips the rasterized glyphs; partial glyphs at the boundary are pixel-clipped. **This is the only text clip mechanism in v1.** There is no text-only offscreen path — render-to-target exists *solely* for group opacity (`opacity < 1`), not for clipping. (If a clipped text node also happens to sit inside an opacity layer, it is drawn into that layer's target and clipped by the active clip rect there, but no extra offscreen apparatus is introduced for clipping itself.)

**Verification:** a dedicated test lays a multi-line text node inside an `Overflow::Hidden` parent shorter than the text, captures the BMP, and asserts pixels below the clip boundary are background-colored (no glyph bleed), and `error_count()==0`. Goldens are fresh against the new look.

### 10.8 Invariants summary

- One `MeasureTextFn`, used by both the Yoga `MeasureFn` shim and `build_draw_list`. Measure == draw.
- `ui/` never touches SDL_ttf; the function pointer is the only seam (plus the `DrawCommand` list).
- Text content lives in the shared string arena as `(offset,len)` (`UI_RETAINED_TEXT_ARENA_BYTES = 16 KiB`); the 95-byte inline cap is gone; arena overflow ⇒ `error_count()` (failed frame).
- `UI_MAX_TEXT_LINES = 8`; exceeding it sets `overflowed`/`error_count` — never a silent line drop.
- Retained `TTF_Text` cache keyed `(node_id, line_index, face, size, style)` with a content-fingerprint change-gate; arena offset never in the key; size change rebuilds against a per-size `TTF_Font*`; mark-and-sweep eviction.
- Caret/selection x from sub-slice measured advances; caret indices are codepoint boundaries.

---

## 11. Component authoring API

A component reads its theme and live interaction state, calls `resolve()` to produce a dense `VisualStyle`, and hands that style to the host element as a **prop**. `build_draw_list` resolves nothing. There is no `set_visual_style` setter, no paint-pass cascade, no theme lookup during the draw DFS.

### 11.1 HostProps: split `.style` into `.layout` and `.visual`

Today `HostProps` (`src/ui/runtime/element.h:67`) carries one fused `Style style` (`tree.h:236`). The rewrite splits that into `LayoutStyle layout` and `VisualStyle visual` (§2.4). The reconciler commits `.visual` onto the `Node` beside `.layout`, exactly as it commits `Style` onto `Node::style` today (`tree.h:475`). `UiTree::Node` gains a `VisualStyle visual` member; `NodeSnapshot` carries it so the transcriber reads `snapshot.visual` directly.

The convenience factories lose the `Style`-typed overloads: `text(const char *value, const char *key, const Style &style)` becomes `text(const char *value, const char *key, const TextVisual &text = {})` — text is paint, and its measured size flows from the injected `MeasureTextFn`.

### 11.2 `resolve()` lives in `src/ui/style/`

The split earns a folder; types and the resolver land under `src/ui/style/`:

```
src/ui/style/visual_style.h   // VisualStyle, Color, Border, Outline, Gradient, ... LineRun
src/ui/style/style_patch.h    // Opt<T>, opt(), StylePatch, apply(), merge(), patch()
src/ui/style/interaction.h    // InteractionState
src/ui/style/theme.h          // RoleStyle, Theme, ThemeContext, TextStyleContext, use_theme()
src/ui/style/resolve.{h,cpp}  // VisualStyle resolve(const RoleStyle&, const StylePatch& variant, const InteractionState&)
src/ui/style/text_measure.h   // TextMetricsQuery/Result, MeasureTextFn, set_text_measurer
```

There is **one** `resolve()` — the authoritative listing in §5.2, with the full precedence chain including `active`. This section does not redefine it. Presence is **always** the `Opt::set` flag; the `control_style()` helper at `common.h:91` (keyed on `style.background.a == 0`) is **deleted**.

### 11.3 Interaction hooks (every-frame read)

```cpp
// src/ui/style/interaction_hooks.h  (declared in ui/, populated by the runtime — §7)
bool use_hovered();        // this fiber's host == per-frame hovered id
bool use_pressed();        // == pressed id (pointer_press_origin)
bool use_focused();        // == focus_focused_id
bool use_focus_visible();  // focused AND focus_source ∈ {Keyboard, Gamepad, Programmatic}
```

`checked`, `active`, and `disabled` have no hooks — they are component-supplied (§7.7).

### 11.4 Theme + text inheritance via context

Theme is read with `use_theme()` (§4.2); text inheritance flows through `TextStyleContext` (§6). All theme reads go through the `use_theme()` helper, never a raw `use_context(&ThemeContext)`.

### 11.5 Box

`Box` is layout plus an optional caller-supplied `VisualStyle` — the simplest correct primitive, no role and no interaction by default:

```cpp
struct BoxProps {
  const char *key = nullptr; const char *id = nullptr; int id_offset = 0;
  LayoutStyle layout = {};
  VisualStyle visual = {};        // dense; caller-resolved, default = nothing painted
  AccessibilityProps accessibility = {};
  ::ui::UiChildren children = {};
};
```

A default `VisualStyle` paints nothing (`background.a == 0`, `stop_count == 0`, etc.) — resolved-output presence checks, distinct from authoring sentinels.

### 11.6 Explicit variants (no boolean-prop soup)

Each role exposes a `Variant` enum; the component maps it to a `StylePatch` via a pure mapper (§5.4). We do **not** add `bool primary`/`bool danger` props.

```cpp
enum class ButtonVariant : uint8_t { Default = 0, Primary, Secondary, Ghost, Danger };

struct ButtonProps {
  const char *key = nullptr; const char *id = nullptr; int id_offset = 0;
  ButtonVariant variant = ButtonVariant::Default;
  bool disabled = false;          // a state, not a style flag
  bool autofocus = false;
  const char *label = nullptr;
  StylePatch style_override = {};  // caller escape hatch (§11.8); usually empty
  AccessibilityProps accessibility = {};
  std::function<void(const ::ui::ActivationEvent &)> on_activate = {};
  LayoutStyle layout = {
      .direction = ::ui::FlexDirection::Row,
      .align_items = ::ui::AlignItems::Center,
      .justify_content = ::ui::JustifyContent::Center,
      .width = ::ui::Length::points(132.0f),
      .height = ::ui::Length::points(38.0f),
      .padding = {14.0f, 14.0f, 8.0f, 8.0f},
  };
  ::ui::UiChildren children = {};
};
```

Variant differences are sourced from the component-owned mapper using `Theme` tokens and `t.button.base`, **not** from per-variant `Theme` fields.

### 11.7 Button, Input, Checkbox, Text — after

**Button** — reads theme + live state, resolves a dense `VisualStyle`; the focus ring arrives **only** because `theme.button.focus_visible.outline` is set (§4.5) and `st.focus_visible` is true. Nothing injects it:

```cpp
// src/ui/components/button.cppx
::ui::UiElement Button(const ButtonProps &props) {
  const Theme &t = use_theme();
  InteractionState st {
      .hovered = use_hovered(), .pressed = use_pressed(),
      .focused = use_focused(), .focus_visible = use_focus_visible(),
      .active = false,                       // component-supplied (§7.7)
      .disabled = props.disabled,
  };
  StylePatch v = button_variant_patch(t, props.variant);
  merge(v, props.style_override);            // src wins, mutates v (§3.4)
  VisualStyle vs = resolve(t.button, v, st);

  TextVisual label = vs.text;                // Button composes its own label
  if (props.disabled) label.color = t.text_disabled;

  return /* host: layout=props.layout, visual=vs, text=label,
            interaction.focusable=true, callbacks=... */;
}
```

**Input** — same pattern; it owns its edit-state hook and caret/selection geometry comes from measured `LineRun` advances (§10.5):

```cpp
const Theme &t = use_theme();
InteractionState st { .hovered = use_hovered(), .focused = use_focused(),
                      .focus_visible = use_focus_visible(), .disabled = props.disabled };
VisualStyle vs = resolve(t.input, props.style_override, st);
if (props.disabled) vs.text.color = t.text_disabled;
// host: visual=vs, caret token = t.caret, selection token = t.selection
```

**Checkbox** — compound: the box composes a child mark. `checked` is component-supplied interaction state (§7.7). The mark's fill is a *second* resolved `VisualStyle` from the canonical `t.checkbox_mark` role (§4.1), **not** a `kCheckboxCheckedFill` constant:

```cpp
const Theme &t = use_theme();
InteractionState st { .hovered = use_hovered(), .focused = use_focused(),
                      .focus_visible = use_focus_visible(),
                      .checked = props.checked, .disabled = props.disabled };
VisualStyle box  = resolve(t.checkbox,      props.style_override, st);   // body + focus ring
VisualStyle mark = resolve(t.checkbox_mark, /*variant=*/{},       st);   // fill flips on st.checked
// host(Checkbox){ visual=box, layout=row, interaction.checked=props.checked,
//   children = { Box{ key="mark", layout=18x18, visual=mark }, props.label } }
```

**Text** — reads `TextStyleContext` for the ambient default, then overlays its own props (§6.2). Color falls back to `t.text_default` when nothing supplied it, so an omitted color renders the theme default, **not** invisible-by-omission:

```cpp
// src/ui/components/text.cppx — see §6.2 for the full body. Key guarantees:
//   precedence: theme text.base -> inherited TextStyleValue -> explicit props
//   final fallback: if no layer set ink, color = t.text_default (never {0,0,0,0})
//   an EXPLICIT author Opt color (including transparent) is honored as-is
```

A parent that wants a subtree default provides it once via `with_text_default(...)` (§6.3). There is no build-pass `INHERITED_SET`.

### 11.8 Caller overriding a variant via a StylePatch merge

A one-off tweak is a `StylePatch` passed as `style_override` and `merge`d **over** the variant patch (§3.4 ordering: src wins, mutates dst). It wins against base/variant but interaction-state patches still layer on top:

```cpp
// caller site
using ui::style::patch;
StylePatch wide_amber = patch()
    .border(Border{ {2,2,2,2}, {{255,176,32,255},{255,176,32,255},
                                {255,176,32,255},{255,176,32,255}} })
    .corner_radius(6.0f);

return ::ui::component("Button", ButtonProps{
    .variant = ButtonVariant::Primary,
    .style_override = wide_amber,          // merged OVER the primary patch
    .label = "EQUIP",
    .on_activate = on_equip,
}, ui::components::Button);
```

Inside `Button`: `merge(button_variant_patch(t, Primary), wide_amber)` then `resolve(t.button, ..., st)`: base → Primary → `wide_amber` (only border + corner_radius move) → then `st`'s interaction patches. The caller cannot null out the focus ring or fill — unset `Opt`s leave the base/variant untouched, and `disabled` still wins last. To change an interaction state, nest a tweaked `RoleStyle` via `ThemeProvider` (§5.5).

---

## 12. Boundaries & file layout

This pins where every new type lives, the dependency direction, the single `ui/`→`renderer/` seam, and the exact file delta.

### 12.1 The two hard boundaries (enforced)

`ui/` MUST stay (a) SDL-free and (b) game-vocabulary-free. Both are machine-checked:

- **No game vocab in `ui/`** (`src/ui/CLAUDE.md`). All new style/IR types are generic — `VisualStyle`, `StylePatch`, `Theme`, `RoleStyle`, `DrawCommand` name no shooter concept. `RoleStyle` fields are *widget roles* (`box, text, button, input, checkbox, checkbox_mark, dialog`), already generic `HostKind`/`NodeRole`, not game roles.
- **No SDL in `ui/`** (`src/ui/CLAUDE.md`). The `runtime_dependency_guard.py` test (`tests/runtime_dependency_guard.py`, registered in `CMakeLists.txt`) scans `src/**` for blocked terms; extend its `BLOCKED` tuple to reject `sdl`/`SDL`/`TTF_` under `src/ui/`. The new headers contain only POD aggregates and `uint32_t texture_id` — no SDL handle crosses into `ui/`. The renderer maps `texture_id` → `SDL_Texture*` on its side.

`renderer/` may use SDL3/SDL3_ttf and depend on the generic `ui/runtime` draw-command types, but MUST NOT depend on `client/`, `game/`, or `app/`.

### 12.2 Exact placement of new files

`src/ui/style/` earns a folder (multiple files, distinct concern from the Yoga-mirrored `Style` in `tree.h`):

```text
src/ui/style/                       NEW FOLDER — generic, SDL-free, game-free
  visual_style.h                    Color, Vec2, SideWidths, SideColors, Border, Outline,
                                    GradientStop, Gradient, BackgroundImage, Shadow,
                                    TextVisual, TextAlign, TextWrap, VisualStyle, LineRun
  style_patch.h                     Opt<T>, opt(), StylePatch, apply(), merge(), patch() builder
  interaction.h                     InteractionState
  theme.h                           RoleStyle, Theme, ThemeContext / TextStyleContext, use_theme()
  resolve.h / resolve.cpp           VisualStyle resolve(const RoleStyle&, const StylePatch& variant,
                                                        const InteractionState&)
  text_measure.h                    TextMetricsQuery/Result, MeasureTextFn, set_text_measurer
```

`Color` currently lives at `tree.h:229`. Move the definition to `src/ui/style/visual_style.h` and have `tree.h` include it, so there is one `Color` shared by `LayoutStyle`'s neighbors, `VisualStyle`, and `DrawCommand` — no duplicate definition.

The `Style` split lands in `tree.h`: keep the Yoga-mirrored fields as `struct LayoutStyle` and delete the paint tail. The host element carries both as separate props (§2.4): `HostProps.layout` (was `HostProps.style`, `element.h:71`) and a new `HostProps.visual`. The reconciler commits `.visual` exactly as it commits style today.

The `DrawCommand` IR replaces the current 2-kind enum (`draw_list.h:12-26`) **in place** (it is the runtime↔renderer contract and already lives in `runtime/`; the dependency guard and test targets already reference `src/ui/runtime/draw_list.cpp`):

```text
src/ui/runtime/draw_list.h          REWRITTEN — tagged-union POD IR (§8); DrawRect header rect;
                                      static_assert is_aggregate_v && is_trivially_copyable_v
src/ui/runtime/draw_list.cpp        REWRITTEN — pure transcriber DFS; hierarchical z; cursor-only
                                      reset; out-of-line text/stop arenas
```

### 12.3 The single `ui/`→`renderer/` seam

Exactly two things cross between `ui/` and `renderer/`:

1. **Outbound (ui→renderer): the `DrawCommand` list + arenas.** The renderer is a dumb linear executor over `DrawList::commands`, reading the out-of-line text/stop arenas by `(offset,len)`. It resolves nothing. `texture_id` is an opaque `uint32_t` the renderer maps to its `SDL_Texture*` registry — the only place an SDL handle meets a `ui/` id, on the renderer side.
2. **Inbound (renderer→ui): the injected `MeasureTextFn`.** The ONE callback `ui/` invokes that reaches SDL_ttf. It reuses the existing `MeasureFn` seam shape (`Size (*)(MeasureInput, void*)`, `tree.h:378`, installed via `UiTree::set_measure`). Today `measure_text_node` fakes width with `strlen(text)*8.0f` — the caret bug (also `draw_list.cpp:120`). The rewrite installs *one* real `MeasureTextFn` once (provided by `renderer/`, which owns `TTF_TextEngine`) and uses it for BOTH the Yoga `MeasureFn` and the draw-time per-line emitter. `MeasureTextFn` returns a bounded `LineRun` array — pure POD declared in `src/ui/style/text_measure.h`. `ui/` never `#include`s anything SDL.

The injection point: `app/` (which composes everything) wires the renderer-owned implementation into an `ui::set_text_measurer(MeasureTextFn)` at startup.

### 12.4 Dependency direction (unchanged)

```text
client/ui/screens → client/ui → game → ui → react
app/ composes everything; platform/ and renderer/ are siblings consumed by app/
game/ must not include SDL or UI headers
```

- `src/ui/style/*` and the `DrawCommand` IR sit at the `ui/` layer, depending only downward on `react` (for `ReactContext` used by `ThemeContext`/`TextStyleContext`/`InteractionContext`). They MUST NOT include `client/`, `game/`, `renderer/`, or SDL.
- A component calls `resolve()` at authoring time, reads `use_theme()`, and passes the resulting `VisualStyle` as `.visual`. `Theme` is delivered ONLY through `use_context` over the provider machinery; providers commit no node and the context stack unwinds before `build_draw_list`, so the deleted-cascade decision is structurally enforced.
- `renderer/` consumes the IR but never `Theme`/`StylePatch`/`resolve()` — those have collapsed to dense `VisualStyle` then flat `DrawCommand`s by the time they reach the seam.

### 12.5 Deferred writes are unchanged

Any state a component reads to resolve its style (interaction flags via the hooks) is read-only during render; anything that *mutates* game/UI state during the declaration pass still routes through `client::ui::ClientUi::queue_deferred_write`. `resolve()` is pure and side-effect-free.

### 12.6 `architecture.md` deliverable

`CLAUDE.md`, `src/CLAUDE.md`, and `src/ui/CLAUDE.md` point at `architecture.md` as the canonical boundary doc, but the file does not currently exist:

```text
$ ls /Users/hv/repos/ui/architecture.md
ls: /Users/hv/repos/ui/architecture.md: No such file or directory
```

**This spec commits to creating `/Users/hv/repos/ui/architecture.md`** as an explicit deliverable of the rewrite (not a conditional fallback), with this required contents outline:

1. Layer diagram + dependency direction (`client/ui/screens → client/ui → game → ui → react`; `app/` composes; `renderer/`/`platform/` siblings).
2. The two hard `ui/` invariants (SDL-free, game-vocabulary-free) and the `runtime_dependency_guard.py` enforcement.
3. The single `ui/`→`renderer/` seam: the `DrawCommand` outbound contract and the inbound `MeasureTextFn`.
4. Where new code goes (the `src/ui/style/` folder; the IR in `runtime/`).
5. The component-resolved-styling model in one paragraph, pointing at this design doc for detail.

The three `CLAUDE.md` references resolve to that file. The rewrite does not ship leaving them dangling.

### 12.7 New / renamed files (delta summary)

| Path | Status | Contents |
|------|--------|----------|
| `src/ui/style/visual_style.h` | **new** | `Color` (moved from `tree.h:229`), `Vec2`, `SideWidths`, `SideColors`, `Border`, `Outline`, `GradientStop`, `Gradient`, `BackgroundImage`, `Shadow`, `TextAlign`, `TextWrap`, `TextVisual`, `VisualStyle`, `LineRun` |
| `src/ui/style/style_patch.h` | **new** | `Opt<T>`, `opt()`, `StylePatch`, `apply()`, `merge()`, `patch()` builder |
| `src/ui/style/interaction.h` | **new** | `InteractionState` (incl. `active`) |
| `src/ui/style/theme.h` | **new** | `RoleStyle`, `Theme`, `ThemeContext`, `TextStyleContext`, `use_theme()` |
| `src/ui/style/resolve.{h,cpp}` | **new** | `VisualStyle resolve(const RoleStyle&, const StylePatch& variant, const InteractionState&)` |
| `src/ui/style/text_measure.h` | **new** | `TextMetricsQuery/Result`, `MeasureTextFn`, `set_text_measurer` |
| `src/ui/runtime/tree.h` | **edited** | `Style` → `LayoutStyle` (drop paint tail); include `style/visual_style.h`; add `Node::visual`; `UI_RETAINED_TEXT_ARENA_BYTES`; text as `(offset,len)` |
| `src/ui/runtime/element.h` | **edited** | `HostProps.style` → `HostProps.layout`; add `HostProps.visual` (`element.h:71`) |
| `src/ui/runtime/element.cpp` | **edited** | commit `.visual`; replace fake `measure_text_node` with the injected `MeasureTextFn`; store `value` as `(offset,len)` |
| `src/ui/runtime/focus.{h,cpp}` | **edited** | add `hovered_id`, `focus_hovered_id`, `focus_pressed_id`, `focus_source_is_visible` |
| `src/ui/runtime/interaction.{h,cpp}` | **new** | `InteractionSnapshot`, `InteractionContext`, `use_host_id`, the four hooks |
| `src/ui/runtime/draw_list.h` | **rewritten** | tagged-union POD `DrawCommand` IR (§8) |
| `src/ui/runtime/draw_list.cpp` | **rewritten** | pure-transcriber DFS; hierarchical z; cursor-only reset; arenas; delete `k*Fill`/`kCharWidth` |
| `src/renderer/sdl_retained_renderer.{h,cpp}` | **edited** | linear executor over the new IR; `texture_id`→`SDL_Texture*` map; provide the real `MeasureTextFn` |
| `src/renderer/font_registry.{h,cpp}` | **edited** | per-`(face,size)` `TTF_Font*` map; back `MeasureTextFn` (returns `LineRun` array) |
| `src/client/ui/client_ui.cpp` | **edited** | publish `InteractionSnapshot` from the focus getters |
| `tests/runtime_dependency_guard.py` | **edited** | add `sdl`/`SDL`/`TTF_`-under-`src/ui/` to the boundary scan |
| `architecture.md` | **new** | canonical boundary doc (§12.6) |

New `src/ui/style/*.cpp` (i.e. `resolve.cpp`) are added to the existing `retained_ui_*`/pipeline test target source lists and to a focused `ui_style_tests` target (needs only `react.cpp` for context types, not SDL or Yoga).

---

## 13. Testing

Testing is layered to match the two seams. The **hermetic layer** (no SDL) covers everything `ui/` computes alone: `resolve()`, the `StylePatch`/`Opt<T>` machinery, `TextStyleContext` inheritance, and `build_draw_list` driven by a mock `MeasureTextFn`. The **SDL layer** covers the executor against fresh golden BMPs of the *new* look. The **Python CLI layer** is unchanged. There are **no parity goldens** — every BMP is authored against the new design.

Hermetic targets follow the existing `retained_ui_*_tests` mold: a single `int main()` returning nonzero on first failure, the `CHECK(expr)` macro (`tests/retained_ui_draw_list_tests.cpp:9`), registered with `add_test`, linking only the minimal source set.

### 13.1 New test targets

| Target | Layer | Sources (beyond the test .cpp) | SDL? |
|---|---|---|---|
| `ui_style_resolve_tests` | hermetic | `src/ui/style/resolve.cpp` | no |
| `ui_draw_list_emit_tests` | hermetic | `resolve.cpp`, `draw_list.cpp`, `element.cpp`, `tree.cpp`, `${UI_COMPONENT_SOURCES}`, `react.cpp` | no |
| `ui_theme_context_tests` | hermetic | `resolve.cpp`, `react.cpp`, `element.cpp`, `tree.cpp` | no |
| `renderer_golden_tests` | SDL | `sdl_retained_renderer.cpp`, `font_registry.cpp` + draw-list sources | yes |
| `renderer_text_cache_tests` | SDL | same | yes |
| `ui_style_invariants_guard` | python | `tests/ui_style_invariants_guard.py` | n/a |

`retained_ui_draw_list_tests` stays as legacy element/layout coverage; new emission semantics go into `ui_draw_list_emit_tests`.

### 13.2 Hermetic — `resolve()` (`ui_style_resolve_tests`)

Pin the locked precedence (§5.1). All inputs are plain aggregates; no theme provider needed since `resolve()` takes a `const RoleStyle&`.

Cases:
- **Precedence low→high.** A `pressed` patch beats a `hover` patch on the same field; an unset higher layer does NOT clobber a set lower layer.
- **`active` layer applies between checked and disabled.** With `st.active` and a `role.active` patch, the active patch lands above checked and below disabled.
- **Variant before state.**
- **Set-flag survival (no-sentinel guarantee).** `opt(Color{0,0,0,0})` applies (resolved field is exactly `{0,0,0,0}`); `Opt<Color>{false, {255,255,255,255}}` does not apply despite a non-zero value. Same pair for `corner_radius` (`opt(0.0f)` applies; `{false, 8.0f}` does not).
- **Disabled wins last.** With `disabled` plus `hovered/pressed`, the disabled patch lands on top.
- **Empty resolve is the base.** `resolve(role, {}, {})` returns `role.base` byte-identical (memcmp).
- **focus_visible vs focused.** `focused && !focus_visible` does NOT apply the `focus_visible` patch.
- **Dense output.** `static_assert(is_aggregate_v && is_trivially_copyable_v<VisualStyle>)`; assert `sizeof(VisualStyle)` is stable (catch `Opt<>` creep on the hot path).

### 13.3 Hermetic — theme + inheritance (`ui_theme_context_tests`)

Run the real hook runtime, reconcile a small tree, read the committed `Node.visual`.

- **Theme reaches a leaf.** A `Button` under a `ThemeContext` provider resolves `theme.button.base.background`.
- **Nested providers override.** Inner subtree resolves the inner theme; outer button the outer; nothing leaks.
- **Disabled-dim via `TextStyleContext`.** A disabled container provides a dimmed `TextStyleValue`; a child `Text` resolves the dimmed color; a sibling outside keeps `text_default`. No `INHERITED_SET` cascade test exists (that path is deleted).
- **Context unwound before paint.** After `react_end_frame`, assert context stack depth is zero, then `build_draw_list` succeeds reading only committed `Node.visual`.

### 13.4 Hermetic — draw-list emission with a mock `MeasureTextFn` (`ui_draw_list_emit_tests`)

A deterministic mock (fixed monospace advance, fixed line height, alignment pre-baked) makes per-line geometry and caret offsets exact and platform-independent. The mock uses 1-byte ASCII fixtures so byte==advance holds *for the fixtures* (not a runtime guarantee — §10.5).

- **measure==draw caret.** A test `caret_geometry_comes_from_measured_advances` asserts caret `Rect.x == text_origin.x + kAdvance * caret_column`, never `8 * caret_column`.
- **Per-line counts.** A 3-line wrapped paragraph emits exactly 3 `Text` commands in ascending `y`; concatenated slices reconstruct the source.
- **Alignment pre-baked.** Left/Center/Right: each line's `x` matches the mock's pre-baked alignment; `build_draw_list` does no alignment math.
- **Line cap.** A paragraph measuring to 9 lines (> `UI_MAX_TEXT_LINES=8`) drives `error_count > 0` and a failed frame — never a silent drop.
- **Selection from advances.** Selection over columns `[2,5)` emits a `Rect` with `x = origin.x + 2*kAdvance`, `width = 3*kAdvance`.
- **Bracket integrity.** With nested clips and an opacity layer, assert every `ClipPush`/`LayerPush` has a matching pop, brackets are contiguous and balanced (a stack walk never goes negative, ends at zero), and a child's commands fall strictly between its parent's push/pop. Helper `assert_balanced_brackets(list)` returns max depth.
- **Premultiplied at emit.** A 50%-alpha red resolves to premultiplied `{128,0,0,128}`-shaped values (±1); `{0,0,0,0}` is the only fully-transparent output.
- **Presence checks, not sentinels.** `stop_count==0` ⇒ no `Gradient`; `texture_id==0` ⇒ no `Image`; `shadow.color.a==0` ⇒ no `Shadow`; `hidden==true` ⇒ no paint but node still occupies layout. Test names flag these as resolved-output rules, distinct from authoring `Opt::set`.
- **Arena/capacity overflow ⇒ failed frame.** (a) text exceeding `UI_RETAINED_TEXT_ARENA_BYTES` sets `error_count` (no `strncpy` clamp); (b) a tree exceeding `UI_MAX_DRAW_COMMANDS` sets `error_count`. Both assert `build_draw_list` returns false and the partial array is not consumed.
- **Cursor-only reset.** Two frames through one `DrawList`; correctness must not rely on zeroed slots past `count`.

### 13.5 SDL — fresh golden BMPs (`renderer_golden_tests`)

Run with `SDL_VIDEODRIVER=dummy`, `SDL_RENDER_DRIVER=software` (as `tests/ui_cli_smoke.py` does). Each case renders a hand-authored draw list offscreen, reads pixels, compares to a committed golden under `tests/golden/` with a small per-channel tolerance (±2) to absorb software-rasterizer rounding. No exact bit match, no historical comparison. First run with `UI_GOLDEN_REGEN=1` writes goldens; CI compares. `compare_bmp(actual, golden, tol, &report)` dumps `*_actual.bmp`/`*_diff.bmp` on failure.

One golden per v1 capability:
- **Rounded-rect topology** (corner pixels are background; arc present).
- **Co-feathered fill+border, no seam** (no transparent/background pixels between fill and border).
- **Linear gradient** (2-stop and 8-stop, `angle 0`/`90`; monotonicity probe).
- **Nine-slice image** (corners unscaled, center stretched). Nine-slice + rounded corners NOT tested (cut).
- **Drop shadow** (feathered; falls on the offset side, fades outward). Inset shadow NOT tested (reserved seat).
- **Group opacity composite** (overlapping translucent children flatten once; no double-darkening).
- **Signed-offset focus ring, both directions** (inset `offset<0`, outset `offset>0`, plus the default small-positive ring).
- **Clipped text** — hard assertion (independent of tolerance) that every pixel outside the clip rect equals the backdrop (no glyph bleed).

### 13.6 SDL — text cache (`renderer_text_cache_tests`)

Instrument the font/text path with counters.
- **Steady state.** Same content for N frames: exactly 1 create, 0 `SetTextString` after frame 1.
- **Reflow leaks zero.** Width-change re-wrap and back: balanced create/destroy; live count returns to baseline.
- **Font-size change rebuilds once.** One destroy + one create, not per-frame churn.
- **measured==drawn (real measurer).** Install the real `MeasureTextFn` as both the Yoga `MeasureFn` and draw-time measurer; assert caret/selection geometry matches rasterized advances (±1px).

### 13.7 Python guard — no value-space sentinels (`ui_style_invariants_guard`)

A source-scanner (mold of `runtime_dependency_guard.py`) fails the build if banned authoring patterns reappear in `src/ui/`:
- `.a == 0`/`.a != 0` as "unset" in authoring/resolve code (the `VisualStyle` resolved-output presence checks in `draw_list.cpp`/`text.cppx` are path-whitelisted);
- `corner_radius == 0`/`font_size == 0`/`size == 0` as "unset" in patch/authoring code;
- any `Opt<` field read without consulting `.set` (heuristic);
- reintroduction of an inline `char value[` text cap in `tree.h`, or any `INHERITED_SET`/build-pass-cascade identifier.

### 13.8 Python CLI smoke — unchanged

`ui_cli_smoke`/`ui_cli_commands`/`ui_cli_dm` keep their current shape (drive `hello` over the control dir, capture BMPs with dummy/software drivers, assert artifacts non-empty). `tests/ui_cli_smoke.py` is not modified — it remains a liveness check, not a styling oracle. Styling correctness lives in `renderer_golden_tests`.

---

## 14. Migration / phasing

This replaces the old parity migration (the retired "Renderer replacement" slice and its golden-vs-commit anchor). There is **no parity golden** and no commit anchor. Each phase ends green: compiles under `-Wall -Wextra` and `./build.sh --tests` passes. Phases are ordered so type/IR work lands inert, resolution is unit-tested before wiring, and the visual look changes only at explicitly-goldened phases (P3, P5, P6).

### Phasing invariants

- **Inert-first.** A phase adding types/IR does not change output. New `DrawCommandKind` seats land before any path uses them.
- **One seam crossing per phase.** Only P3 (renderer reads new IR) and P4 (injected `MeasureTextFn`) change the `ui/`→`renderer/` seam.
- **Resolution is pure and unit-tested before wired.** `resolve()` is verified with no `UiTree`/renderer/SDL before any component calls it.
- **Overflow is always a failed frame.** Every phase preserves the `error_count`→failed-frame contract.

### P0 — Land the IR + style types, inert

- Define all style types and `Theme`/`RoleStyle`/`InteractionState` (with `active`) verbatim. `Color` supersedes `tree.h:229` (now straight-alpha authoring, premultiplied only at IR emit).
- Replace the 2-arm `DrawCommandKind` (`draw_list.h:12`) with the full enum (§8.1), `Custom` a reserved seat with no payload arm.
- Define `DrawCommand` as header + tagged union of POD arms, first arm with a default member initializer; `DrawRect` IR-local header rect (NOT `tree.h` `Rect`). All `static_assert`s land here, including the `sizeof` bounds (§8.2).
- Out-of-line arenas declared, cursors not yet written. Reset stays as-is in P0 (behavior-preserving); switch to cursor-only in P3.
- `UI_RETAINED_MAX_DRAW_COMMANDS` keeps its current value; re-derive to `UI_MAX_DRAW_COMMANDS` (§8.7) in P5/P6.
- Existing emitters keep compiling by mapping old fields onto the new `RectData`/`TextData` arms — mechanical, behavior-preserving. The caret bug is NOT fixed here (fixed in P4).

**Gate:** a `*_style_types_tests` compiles, the `static_assert` set passes, `Opt<Color>{}.set == false`. All existing tests green.

### P1 — `resolve()` + `Theme` + `ThemeContext` + hooks, unit-tested in isolation

- Implement `resolve()` (§5.2) with the full precedence chain (incl. `active`).
- Create `ThemeContext` via the existing context machinery; providers commit no node; stack unwound before paint.
- Add the interaction hooks (§7). `use_focused` reuses `focus_focused_id`; `use_focus_visible` derives from `FocusSource`. Add stored `hovered_id` + getters `focus_hovered_id`/`focus_pressed_id` (pressed derived from existing `pointer_press_origin` — **no new stored `pressed_id`**). `use_host_id` computes the deterministic id (§7.3). One-frame lag accepted; no pointer events.
- Author `default_theme()` (§4.5), including the `focus_visible` outline patch for focusables. The constants currently inlined in `draw_list.cpp:10-21` move here and are re-tuned.

**Gate:** `ui_style_resolve_tests` (no `UiTree`/renderer): empty patches → base; precedence ordering; `active` between checked and disabled; `disabled` wins; `Opt::set==false` never overwrites. A hook test asserts `use_focus_visible()` tracks `FocusSource` transitions.

### P2 — Split `HostProps`; components resolve-and-pass; builder becomes a transcriber

- Split `Style` (`tree.h:236`) into `LayoutStyle` + `VisualStyle`; `HostProps` carries both. Reconciler commits `.visual`; `NodeSnapshot` exposes `node.visual`.
- Migrate every component in `src/ui/components/` to the authoring shape: `use_theme()`, build `InteractionState`, `resolve()`, pass `.visual`. `Text` reads `TextStyleContext`; a parent dims children by providing a dimmed `TextStyleValue`. The `inherited_disabled` thread (`draw_list.cpp:80-213`) is deleted.
- **Delete the focus-ring injection** (`draw_list.cpp:59-66`) and `kFocusBorder*`. Focus ring is now a resolved `Outline`. `build_draw_list` no longer takes `focused_id`.
- `build_draw_list` becomes a **pure transcriber**: emit from `node.visual`/`node.layout`; resolve nothing. Hierarchical z during the DFS (only `Rect`/`Border`/`Text` in P2, so brackets are present but trivial).

**Gate:** component tests assert components produce the expected `.visual` (a focused `Button` resolves a non-zero `outline`); transcriber tests assert one fused frame command for a bordered box and nothing for an unstyled box. CLI smoke green; rebaseline if BMPs are captured (P2 output is the new baseline).

### P3 — Renderer rewrite behind NEW goldens

First visual phase. Renderer rewritten as a dumb linear executor (§9).
- Premultiplied compositing at init; geometry colors fed verbatim.
- v1 paint: solid `Rect`, fused frame (border+outline co-feathered with fill, single pass), uniform `corner_radius`, signed-offset focus ring. `ClipPush`/`ClipPop` fixed stack.
- Per-frame reset switches to **cursors only**.
- **Re-golden** fresh against the new look (rounded corners, feathered borders, premultiplied composite). First real visual goldens. No HiDPI apparatus.

**Gate:** renderer golden tests pass against fresh BMPs; CLI smoke green; balanced `ClipPush`/`ClipPop` asserted.

### P4 — Text measure seam + per-line emission + `TTF_Text` cache + lifted text storage

- Install one `MeasureTextFn` used as BOTH the Yoga `MeasureFn` and draw-time measurer (measure==draw).
- Measurer returns a bounded `LineRun` array (alignment pre-baked); transcriber emits one `Text` command per `LineRun`. Line cap 8. Caret/selection from measured advances — **kills the `kCharWidth=8.0f` caret bug** (`draw_list.cpp:120`).
- Lift text storage to the shared string arena `(offset,len)` (`UI_RETAINED_TEXT_ARENA_BYTES = 16 KiB`); delete `copy_text`'s 95-byte clamp; overflow ⇒ `error_count`.
- Renderer caches `TTF_Text` keyed by content/font; per-`(face,size)` `TTF_Font*` map.
- **Re-golden** text-bearing screens against measured layout.

**Gate:** caret-position test asserts caret x == measured advance; multi-line wrap test asserts emitted `Text` count == measured `LineRun` count; over-budget text asserts `build_draw_list` returns false.

### P5 — Images, gradients, nine-slice, drop shadow

- `Image` arm: textured rect + tint + nine-slice. `texture_id==0` ⇒ none. No nine-slice + rounded corners (cut).
- `Gradient` arm: linear N-stop (8 max); stops in the gradient arena. `stop_count==0` ⇒ none. Radial = reserved seat only.
- `Shadow` arm: drop only, feathered quads. `color.a==0` ⇒ none. Inset = reserved seat. No gaussian sprite cache (cut).
- Re-derive `UI_MAX_DRAW_COMMANDS` (§8.7) now that a node can emit shadow + fill + frame.

**Gate:** golden tests for a tinted nine-slice panel, a 3-stop gradient, a drop shadow; arena-overflow tests assert failed frames.

### P6 — Group opacity

- `LayerPush`/`LayerPop` arms: a node with `visual.opacity < 1` brackets its subtree; composite via a single naive render-to-target (transient target sized to the `LayerPush` header rect, §9.10). No pool, no hysteresis, no budget, no switch caps, no degraded fallback (all cut). Hierarchical-z keeps brackets contiguous.
- Re-golden screens using group opacity.

**Gate:** a group-opacity golden (a half-opaque subtree composites uniformly); transcriber test asserts balanced `LayerPush`/`LayerPop`.

### Phase dependency summary

| Phase | Depends on | Visual change? | Re-golden? |
|------|-----------|----------------|-----------|
| P0 IR + types | — | no | no |
| P1 resolve + Theme + hooks | P0 | no | no |
| P2 split props + transcriber | P1 | equivalence (constants moved) | rebaseline if BMPs captured |
| P3 renderer rewrite | P2 | **yes** | **yes (first real goldens)** |
| P4 text measure seam | P3 | yes (measured layout) | **yes** |
| P5 image/gradient/nine-slice/shadow | P3 | yes | **yes** |
| P6 group opacity | P3 | yes | **yes** |

P4–P6 depend only on P3, not on each other, so they may land in any order or in parallel; each re-goldens only the screens it touches.

---

## 15. Deferred / reserved

Recorded so future readers know these omissions are deliberate, not oversights.

### Reserved enum seats (names only — no payloads, no renderer paths in v1)

- `DrawCommandKind::Custom` — escape hatch. **No union arm, no `CustomData`** (the cut typed-custom payload arena is not reintroduced). The renderer no-ops on it.
- Radial gradient — `Gradient` carries linear data only; no radial mode/payload.
- Inset shadow — `Shadow` is drop-only; the inset variant is a name reserved for later.

### Cut gold-plating (do NOT spec or implement in v1)

- Per-corner **elliptical** radii (v1 is one uniform `corner_radius` float).
- Unequal-width **CSS miter `atan2`** border math (v1 uses a radial 45° corner split).
- Nine-slice **combined with rounded corners** (deferred; radius ignored on nine-slice images).
- Gaussian nine-slice **shadow sprite cache** (v1 shadows are feathered quads).
- VRAM **offscreen-layer pool** with size-bucket hysteresis + total-VRAM budget + per-frame switch caps + degraded fallback (v1 group opacity is one naive transient render-to-target per `LayerPush`/`LayerPop`).
- **64-line** paragraph machinery (v1 cap is `UI_MAX_TEXT_LINES = 8`).
- **HiDPI golden** apparatus.
- `user_data` on every command.
- The **typed custom payload arena** (only the `Custom` enum seat is reserved).
- The old build-pass **`inherited_disabled` cascade** / any `INHERITED_SET` mechanism (replaced by `TextStyleContext` providers).
- **Value-space sentinels** of any kind in authoring (`Color.a==0`/`size==0`/`corner_radius==0` as "unset") — replaced by the `Opt<T>::set` convention. Value-as-presence survives only as resolved-output cues the renderer reads (`stop_count==0`, `texture_id==0`, `shadow.color.a==0`, `background.a==0`), never in authoring.
- The fused `Style` carrying both layout and paint — split into `LayoutStyle` + `VisualStyle`.
- Theme query from the node snapshot during the paint DFS — structurally impossible (context unwound before paint).
