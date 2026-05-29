# Clay-Parity Styling & Render System — Final Design

Status: Final / implementation-ready. Supersedes the draft.
Locked decisions (not re-litigated): (1) zero fidelity loss vs the old Clay-based renderer; (2) a **data** `DrawCommand` list with a tagged union mirroring `Clay_RenderData`; (3) CSS-like styling where the **component** is the sole source of truth (no behind-the-back injection of focus rings or file-level theme constants); (4) C++20 / SDL3 / SDL3_ttf, no exceptions, no RTTI in UI/runtime, `-Wall -Wextra`, fixed-size aggregates as house style.

---

## Resolved review findings (changes from the draft)

The draft was revised to resolve every blocker/major and to address or justify every minor finding. Highlights:

- **DrawCommand is a true aggregate, not a class with a user-declared ctor** (blocker). The anonymous union takes a default member initializer on its first arm (`union { RectData rect{}; … };`), so `DrawCommand` stays an aggregate (designated-init at every call site keeps compiling) **and** trivially copyable. We `static_assert` both `is_aggregate_v` and `is_trivially_copyable_v` in Phase 0. See §A.5.
- **z-index no longer flat-sorts the command list** (two blockers). Paint order is resolved **hierarchically during the DFS**: within each stacking context the direct children are stable-sorted by `(z, sibling_index)` and each child's **entire bracketed command range** (its Clip/Layer push…pop and everything between) is emitted contiguously. No global `stable_sort`, so Clip/Layer brackets can never be split and nested contexts can never escape. See §A.8 + §D.5.
- **Per-node interaction is computed by point-in-rect during the DFS, not by id-equality** (major). This restores CSS ancestor-chain `:hover`/`:active`. `:focus`/`:focus-visible` remain id-based (focus is genuinely single-node). See §D.2.
- **The cascade operates over a `VisualStyle` (paint-only) subset, not the full `Style`** (blocker + major). This eliminates the layout-clobber hazard and makes the dense-base inheritance problem solvable. Inheritance uses explicit per-field **sentinels** (`Color.a==0 ⇒ unset`, `font_id==0 ⇒ unset`, `font_size==0 ⇒ unset`, etc.); the base layer is applied **field-wise conditionally**, never as a whole-struct copy. The exact algorithm is in §B.5 with a worked nested-inheritance example and a worked disabled-control example.
- **Text seam fully specified** (2 blockers + majors). One text model is chosen: **per-line emission**, driven by a `MeasureTextFn` that returns a bounded fixed array of `LineRun{slice_offset, slice_len, x, y, w, h}` with alignment pre-baked. The **same measurer is installed as the Yoga `MeasureFn`** so wrapping drives layout height, and reused in `build_draw_list`. See §D.4 + §D.6.
- **TTF_Text cache fully specified** (3 majors). Cache identity = `(node_id, line_index, face, size, style)`; change-gate = a content **fingerprint** (FNV-1a of the slice), color, and wrap-width — never the per-frame arena offset. A resolved-font change is a **rebuild** (`TTF_DestroyText`+`TTF_CreateText`), never `TTF_SetFontSize` on a live shared font. Mark-and-sweep eviction handles reflow and id churn. See §E.5.
- **SDL clip honored by the text engine** is now a **verified fact gated by a test**, with a documented offscreen-layer fallback if the verification fails. See §E.5.
- **Alpha convention fixed pipeline-wide** (major): **premultiplied-alpha end-to-end**. Co-feathered fill+border+single fringe in one `SDL_RenderGeometry` pass (no double-blend seam). See §E.0 + §E.3.
- **Exact rounded-rect topology** reproduces Clay's (center rect + 4 corner fans + 4 edge quads, `segments = max(16, …)`), plus the elliptical/per-corner arc-sampling algorithm. See §E.3.
- **`between_children` dividers, scroll `child_offset`, image-tint mapping, visibility:hidden subtree+outline gating, focus-ring inset-vs-outline geometry, multi-color/multi-width rounded corners, per-target SDL state save/restore, layer bounds = descendant-ink union, layer pool hysteresis + budget, vertex scratch budget, complete theme-constant table** are all now specified. See the relevant sections.
- **Migration**: Phase 4 split into 4a–4e; focus-ring de-injection happens in the **same** phase that adds `theme.focus_visible` with a "exactly one ring" test; input caret/selection reclassified as an **intentional** change excluded from the zero-loss golden, with a deterministic mock measurer for hermetic unit tests. See §G + §H.

**Locked-decision-#2 completeness flag (raised, not re-litigated):** Decision #2 says "mirror `Clay_RenderData` layout." We mirror its *shape and spirit* but deliberately **widen** corner-radius to per-corner-elliptical and border to per-side (width+color+style). Clay's exact field widths (single uniform radius, single border color) would **cap** fidelity below CSS and below what components want to author; honoring decision #1 (zero loss) and #3 (component owns all styling) requires the superset. This is the one place the final design is a strict superset of Clay rather than a byte mirror. It is called out again in §J.

---

## 0. Baseline inventory (the parity target)

This is the exact current behavior the zero-loss guarantee is measured against. Every literal here gets a one-to-one destination in §C.2.

`src/ui/draw_list.cpp` (current builder):
- `build_draw_list` does `*out = {}` then DFS over `NodeSnapshot`s, emitting into `DrawList`.
- `append_rect` (≈ lines 41–77): emits a rect when `styled_box || control_box || focused`. For a **focused** node it injects a focus ring (`border = kFocusBorder`) **regardless of role** — this is the behind-the-back injection decision #3 deletes (the headline de-injection example, lines 59–66).
- `append_text` (≈ line 80–93, 201): uses `has_color(node.style.text) ? node.style.text : <fallback>`; threads `inherited_disabled` so a `Text` child of a disabled control renders dimmed (`kTextDisabledFill`).
- `append_input_contents` (≈ 120, 146, 182): caret/selection X computed as `text_rect.x + index * kCharWidth`, `kCharWidth = 8`, `kInsetX = 8`, `kTextHeight = 16`, plus selection/caret colors.
- File-level constants (`draw_list.cpp`): `kButtonFill`, `kFocusBorder`, `kTextFill`, `kTextDisabledFill`, and the others enumerated in §C.2.
- `src/client/ui/common.h`: a **separate** control palette `kControlFill`, `kControlDisabledFill`, `kControlBorder`, `kControlDisabledBorder` (used by component helpers such as `control_style`). §C.2 reconciles `kButton*` vs `kControl*`.

`src/renderer/sdl_retained_renderer.cpp` (current renderer):
- `render_rect` (≈ 71–85): border drawn as an **inset** loop — `set_draw_color(border)` then `SDL_RenderRect` at `rect + inset` for `border_width` passes (border eats inward, overlaps fill). This is the geometry the focus ring currently uses.
- Text (≈ 97–113): `TTF_RenderText_Blended` → `SDL_RenderTexture` blit. Blits **respect** `SDL_SetRenderClipRect`. No letter-spacing handling anywhere.

`src/ui/element.h:71`: `Style style = {}` on `HostProps` — the field whose type changes (§F).
`src/ui/tree.h`: `Node::value` is `char value[UI_RETAINED_VALUE_CAP]` (`UI_RETAINED_VALUE_CAP == 96`, so a node's source text is capped at 95 bytes + NUL **today and after this change** — see §A.6). `Rect` already carries computed `margin`/`border`/`padding` edges. `Display::{None,Contents}` and `Overflow` already exist. `set_measure`/`MeasureFn` (per-node Yoga measure hook) already exist (~lines 378, 422).

**Confirmed in-repo (replaces external citations):** the current `draw_list.cpp` + `sdl_retained_renderer.cpp` + `font_registry.cpp` apply **no** letter-spacing, and no sample screen sets a non-zero spacing. Honoring only `letter_spacing == 0` in v1 is therefore provable in-repo parity (§J.3), independent of any external adapter.

---

## A. DrawCommand vocabulary

### A.1 Kinds

```cpp
enum class DrawCommandKind : uint8_t {
    None = 0,     // padding / unused slot
    Rect,         // solid (optionally rounded) fill
    Gradient,     // linear N-stop fill (radial DEFERRED-but-not-exposed; see §J.5)
    Image,        // textured rect; tint + nine-slice + corner radius
    Border,       // per-side ring; is_outline marks the focus-ring/outline variant
    Text,         // one wrapped line of text (per-line emission, see §D.6)
    Shadow,       // drop or inset box-shadow
    ClipPush,     // push a resolved clip rect (+ optional rounded clip) onto the stack
    ClipPop,      // pop the top clip
    LayerPush,    // begin a group-opacity (or future transform) offscreen layer
    LayerPop,     // composite the layer with its opacity
    Custom,       // app escape hatch (typed handle; tint + radius pass-through)
};
```

`ClipPush/ClipPop` and `LayerPush/LayerPop` are **structural** commands the renderer executes as a stack in array order. The §A.8/§D.5 paint-order algorithm guarantees these brackets are always balanced and contiguous after z-resolution.

### A.2 Per-kind payloads (POD aggregates)

All payloads are trivially copyable aggregates with default member initializers. Colors are premultiplied at emit time except where noted (§E.0).

```cpp
struct Color  { uint8_t r=0,g=0,b=0,a=0; };          // a==0 ⇒ "unset" sentinel in VisualStyle
struct Vec2   { float x=0.f, y=0.f; };

// Per-corner elliptical radius. Clay superset (Clay had one uniform radius).
struct CornerRadius {
    float tl_x=0,tl_y=0, tr_x=0,tr_y=0, br_x=0,br_y=0, bl_x=0,bl_y=0;
    constexpr bool any() const {
        return tl_x||tl_y||tr_x||tr_y||br_x||br_y||bl_x||bl_y;
    }
    static constexpr CornerRadius all(float r){ return {r,r,r,r,r,r,r,r}; }
};

struct SideWidths { float top=0,right=0,bottom=0,left=0;
    constexpr bool any() const { return top||right||bottom||left; } };
struct SideColors { Color top{},right{},bottom{},left{}; };

enum class GradientKind : uint8_t { Linear=0 /*, Radial reserved – not exposed, §J.5 */ };
struct GradientStop { float t=0.f; Color color{}; };           // t in [0,1]
constexpr int UI_MAX_GRADIENT_STOPS = 8;

struct RectData {
    Color        fill{};
    CornerRadius radius{};
};

// Gradient payload lives OUT-OF-LINE (side pool, §A.5) so the union stays small.
struct GradientData {
    uint32_t      pool_index = 0;        // -> DrawList::gradients[pool_index]
    CornerRadius  radius{};
};
struct GradientPayload {                 // pooled
    GradientKind kind = GradientKind::Linear;
    Vec2  start{}, end{};                // linear endpoints (resolved in node space)
    uint8_t      stop_count = 0;
    GradientStop stops[UI_MAX_GRADIENT_STOPS]{};
};

struct ImageData {
    uint32_t     texture_id = 0;         // -> renderer TextureCache
    Color        tint{255,255,255,255};  // multiply tint (see §B.6 mapping rule)
    CornerRadius radius{};
    SideWidths   nine_slice{};           // nonzero ⇒ 9-slice; 0 ⇒ stretch
};

enum class BorderStyle : uint8_t { Solid=0, None };
struct BorderData {
    SideWidths   width{};
    SideColors   color{};
    CornerRadius radius{};               // OUTER radius of the box this border rings
    BorderStyle  style = BorderStyle::Solid;
    bool         is_outline = false;     // true ⇒ outline/focus-ring variant (§E.3)
    float        outline_offset = 0.f;   // signed; only meaningful when is_outline
};

enum class TextAlign : uint8_t { Left=0, Center, Right };
struct TextData {
    uint32_t     arena_offset = 0;       // byte slice into DrawList::text_arena (§A.6)
    uint16_t     arena_len = 0;          // this LINE's bytes (per-line emission)
    uint32_t     fingerprint = 0;        // FNV-1a of the slice (cache change-gate, §E.5)
    Color        color{};
    uint16_t     font_id = 0;            // renderer font face id
    uint16_t     font_size = 0;          // px
    uint16_t     line_index = 0;         // 0-based line within the source node
    int16_t      letter_spacing = 0;     // v1 must be 0 to render (§J.3); measured==drawn
    // x/y/w/h carried by the common rect header (this line's pre-aligned box)
};

enum class ShadowKind : uint8_t { Drop=0, Inset };
struct ShadowData {
    ShadowKind   kind = ShadowKind::Drop;
    Color        color{};
    Vec2         offset{};
    float        blur = 0.f;
    float        spread = 0.f;
    CornerRadius radius{};               // matches the shadowed box (§E.8 split)
};

struct ClipData {
    CornerRadius radius{};               // rounded clip (0 ⇒ axis-aligned rect clip)
    // the clip rect itself is the common rect header
};

struct LayerData {
    float    opacity = 1.f;              // <1 ⇒ group opacity layer
    // Affine2D transform; RESERVED seam, DEFERRED (§J.1). Identity in v1.
    // float m[6] = {1,0,0,1,0,0};
};

struct CustomData {
    uint16_t     callback_id = 0;        // -> renderer CustomRegistry (§E.6)
    uint32_t     payload_id  = 0;        // -> DrawList::custom_arena (snapshot, §E.6)
    Color        tint{255,255,255,255};  // Clay CustomRenderData.backgroundColor parity
    CornerRadius radius{};               // Clay CustomRenderData.cornerRadius parity
};
```

### A.3 between_children dividers (resolved at A.3, declared in §B)

Clay's `Clay_BorderWidth.betweenChildren` synthesizes a `RECTANGLE` between consecutive laid-out children. We make this **declarable** (a `BetweenChildren` field on `Style`, §B) and **resolve** it in the builder: between each pair of adjacent non-collapsed children, the builder emits a `Border` (with only the cross-axis side set) or a `Rect` strip whose geometry is computed from the two children's laid-out rects and the container's layout axis:

- **Row** containers: a vertical strip of width `between_children.width`, full content height, centered in the gap between child[i].right and child[i+1].left.
- **Column** containers: a horizontal strip, full content width, centered in the gap.
- The strip position is **translated by the container's resolved `scroll_offset`** (§A.4) exactly like the children, matching Clay's "childOffset added on the DFS return pass."

Color/width come from `between_children`. Dividers are emitted **inside** the container's Clip bracket (so they clip with the content) and after the children's background but interleaved at the children's z (they are part of the container subtree, never floated).

### A.4 Scroll child_offset (overflow:scroll completeness)

Clay's `ClipElementConfig.childOffset` shifts all children of a clipping container. We represent it as a resolved input threaded through the builder, not only as deferred behavior:

- `Style::scroll_offset` (`Vec2`, §B) is the channel. It is consumed by the builder: when visiting children of a clipping container, every descendant rect passed to `visit` is translated by `-scroll_offset` (content scrolls under the clip). The data model is therefore **complete** even if the stateful *scroll position* behavior (input → offset) is wired later.
- The same `scroll_offset` is applied to `between_children` divider positions (§A.3) and to the clip rect's interior — the clip rect itself stays fixed; only descendants move.

### A.5 The command, the list, sizing, and triviality

```cpp
struct UserData {                       // typed handle (replaces Clay void*; §E.6/§J)
    uint16_t kind  = 0;                 // 0 ⇒ none
    uint32_t index = 0;                 // -> app-owned, build->render-stable pool
};

struct DrawCommand {
    // ---- hoisted common header (mirrors Clay_RenderData common fields) ----
    DrawCommandKind kind = DrawCommandKind::None;
    int16_t         z_index = 0;        // RESOLVED effective z (debug/inspection only)
    uint32_t        node_id = 0;        // stable hook-runtime id (provenance, cache key)
    Rect            rect{};             // float box: fill / clip / line / layer bounds
    UserData        user_data{};        // pass-through (see §J: may be elided if unused)

    // ---- kind-specific arm; first arm carries the init so DrawCommand stays aggregate ----
    union {
        RectData     rect_data{};       // <-- default member initializer keeps aggregate-ness
        GradientData gradient;
        ImageData    image;
        BorderData   border;
        TextData     text;
        ShadowData   shadow;
        ClipData     clip;
        LayerData    layer;
        CustomData   custom;
    };
};

static_assert(std::is_aggregate_v<DrawCommand>,
              "DrawCommand must stay an aggregate so designated-init call sites compile");
static_assert(std::is_trivially_copyable_v<DrawCommand>,
              "DrawCommand must be memcpy/assign-safe (push, *out={} reset)");
static_assert(std::is_trivially_copyable_v<GradientPayload>);
```

**Aggregate + trivial proof (Phase 0):** every arm is a trivially-copyable aggregate (only scalars/`Color`/`Vec2`/fixed arrays; `CornerRadius`'s `any()`/`all()` are member *functions*, not data, so they do not affect triviality). A C++20 union may carry a default member initializer on **one** variant member; doing so on the first arm (`RectData rect_data{};`) supplies the union's default initializer **without** a user-declared constructor. `DrawCommand` therefore remains an aggregate (designated initializers such as `DrawCommand{.kind=…, .border=…}` keep compiling everywhere, including all current `draw_list.cpp` sites) and is trivially copyable (no user-declared copy/move/dtor; all members trivially copyable). The two `static_assert`s are added in **Phase 0** so the foundation is proven before anything builds on it. Under `-Wall -Wextra`, designated-init that sets only `.kind` plus one arm does not warn (the union's default member initializer value-initializes the rest); we additionally pin the convention "always brace-init the active arm" to avoid `-Wmissing-field-initializers` on payloads.

**Per-frame reset:** `build_draw_list` resets **only the cursors** — `count = 0; text_arena_used = 0; gradient_count = 0; custom_used = 0; error_count = 0;` — and never `memset`s the command array (commands `>= count` are never read). This removes the enlarged-reset cost the draft introduced (the §"per-frame *out={}*" finding). `DrawList` is large and is **never copied by value** on the hot path: `build_draw_list(out*)` and `render(const DrawList&)` take pointers/refs; a Phase-0 `static_assert(sizeof(DrawList) < BUDGET)` plus a comment forbids stack allocation in any frame function (it is a single static/owned instance).

**Sizes (reviewable budget):** with `GradientPayload` pooled out-of-line, the largest in-union arm is `ImageData`/`BorderData`/`TextData` (≈ `4×CornerRadius`-free, dominated by a `CornerRadius` of 32B + a few scalars ≈ 48–56B). `sizeof(DrawCommand)` ≈ **96–112B** (header `Rect`≈32B + `node_id`/`z`/`kind` + `UserData` + largest arm). Capacity (§A.6) is derived from a worst case, not asserted.

### A.6 Text arena, capacity, and the truthful length cap

- **Lifetime:** text bytes are snapshotted into a per-frame `DrawList`-owned arena (`char text_arena[N]`). This is required because the tree mutates between the declaration pass and the render pass; the renderer reads slices after declaration. The arena offset is a **transient render input only** — never a cache identity or change signal (§E.5).
- **Honest length cap (correcting the draft):** a node's *source* text is `char value[96]` in `tree.h` (95 bytes + NUL). The arena does **not** remove that upstream cap; it relocates already-bounded bytes. We therefore **drop the "arbitrary length" claim**. Per-line emission may re-intern bytes per wrapped line, so the arena is sized for the worst case below, not `NODES×96`.
- **Worst-case sizing.** Per node, interned bytes ≤ `min(95, value_len)` re-emitted across at most `UI_MAX_TEXT_LINES` lines (a line never re-stores bytes beyond the source, but wrapping splits the same ≤95 bytes across lines, so total interned per node ≤ 95). Input contents add ≤ source length. Therefore:
  `UI_RETAINED_TEXT_ARENA_BYTES = UI_RETAINED_MAX_NODES × (UI_RETAINED_VALUE_CAP + UI_MAX_TEXT_LINES) + HEADROOM`
  with `UI_MAX_TEXT_LINES` defined in §D.6. We size against the largest real string across the sample screens plus headroom and validate in a test.
- **Overflow is a hard, test-visible failure, never a silent clamp.** `intern_text` on overflow sets `error_count` and the frame is treated as failed (consistent with `build_draw_list`'s existing `error_count` handling). A test (`retained_ui_draw_list_tests`) interns past capacity and asserts the frame aborts (no partial list rendered) rather than truncating mid-UTF-8.

```cpp
constexpr int UI_RETAINED_MAX_DRAW_COMMANDS =
    UI_RETAINED_MAX_NODES * UI_MAX_CMDS_PER_NODE;   // see §A.7 derivation
constexpr int UI_RETAINED_MAX_GRADIENTS = UI_RETAINED_MAX_NODES;   // rare; one per node max

struct DrawList {
    DrawCommand cmds[UI_RETAINED_MAX_DRAW_COMMANDS]{};
    int         count = 0;

    GradientPayload gradients[UI_RETAINED_MAX_GRADIENTS]{};
    int             gradient_count = 0;

    char     text_arena[UI_RETAINED_TEXT_ARENA_BYTES]{};
    int      text_arena_used = 0;

    unsigned char custom_arena[UI_RETAINED_CUSTOM_ARENA_BYTES]{};  // §E.6
    int           custom_used = 0;

    int      error_count = 0;
};
```

### A.7 Command-count worst case (derives the multiplier)

Per node, the maximum **fixed** primitives:
`Shadow(drop) + Rect(bg) + Gradient + Image + Shadow(inset) + Border + Outline + ClipPush + ClipPop + LayerPush + LayerPop = 11`, plus **text = `UI_MAX_TEXT_LINES`** per-line `Text` commands, plus **between_children dividers ≤ (child_count − 1)** which are charged to the *parent* node and bounded globally by total node count (each child contributes at most one divider). So:

`UI_MAX_CMDS_PER_NODE = 11 + UI_MAX_TEXT_LINES` and the global divider count ≤ `UI_RETAINED_MAX_NODES`. We add the divider budget explicitly:
`UI_RETAINED_MAX_DRAW_COMMANDS = UI_RETAINED_MAX_NODES × (11 + UI_MAX_TEXT_LINES) + UI_RETAINED_MAX_NODES`.
This is a derived bound, not the draft's asserted `×8`. Overflow ⇒ `error_count` ⇒ failed frame (no silent drop).

### A.8 z-index / stacking contexts (hierarchical, never a flat sort)

**A stacking context is opened by** a node whose resolved style has any of: explicit `z_index` set (sentinel-distinguished from "auto"), `opacity < 1`, or (future) a non-identity transform. The root is an implicit context at z=0.

**Algorithm (no global `std::stable_sort`):** during the DFS, when a node opens (or is) a stacking context, its **direct children** are partitioned and ordered for painting *within that context only*:
1. Negative-z children (ascending z), then
2. The context node's **own** in-flow commands (background, gradient, image, border, text) — i.e. the node paints its own box between its negative-z and zero-z descendants, matching CSS,
3. Zero/auto-z, non-positioned children in document order,
4. Positive-z children (ascending z, ties broken by sibling index — stable).

Each child is emitted by recursing, and **the child's entire bracketed command range is appended contiguously** in the sorted order. Because we sort whole subtrees (not individual commands) and emit each subtree's Clip/Layer push…pop together, brackets are always balanced and a `z:9999` descendant of a `z:1` context can never jump above a sibling context — it is trapped in its parent's range. `z_index` on the command header is informational (inspection/tests); ordering is achieved purely by emission order. There is **no** int16 saturation hazard because we never pack hierarchy into a scalar key.

`stacking()` is fully defined (no ambiguity): `bool opens_context = r.z_index_set || r.opacity < 1.f || r.has_transform;`. Worked example: a screen (context z=0) contains a dialog with `z_index=100` (opens a context) which contains a button with `:focus-visible` outline (auto-z). The dialog's whole subtree (clip + bg + button + outline) is emitted as one contiguous range, placed after all z=0 siblings of the screen; the button's outline is at the dialog's context, never above a tooltip the screen places at `z=200`.

### A.9 Opacity / layers (IN); transform (DEFERRED with seam)

`opacity < 1` opens a stacking context and emits `LayerPush{opacity}` … subtree … `LayerPop`. Only nodes with `opacity < 1` allocate a layer (§E.7). Transform is deferred (§J.1) but `LayerData` reserves the `m[6]` seam and §E.2/§J.1 document that a future transformed+clipped subtree must route through the layer path, never `SDL_SetRenderClipRect`.

---

## B. Style model

The cascade does **not** operate on the full `Style` (which fuses layout + visual + text). Layout is already resolved by Yoga before `build_draw_list`. We introduce **`VisualStyle`** — the paint-relevant subset — and run the cascade over that. `Style` keeps its existing layout fields plus the new visual/text fields below; the theme and the resolver speak `VisualStyle`.

### B.1 VisualStyle (paint subset, dense, with sentinels)

```cpp
enum class Visibility : uint8_t { Visible=0, Hidden };   // Hidden is inherited (B.4)

struct VisualStyle {
    // ----- box paint -----
    Color        background{};            // a==0 ⇒ unset
    CornerRadius radius{};                // any()==false ⇒ unset
    SideWidths   border_width{};          // any()==false ⇒ unset
    SideColors   border_color{};          // each side a==0 ⇒ unset
    BorderStyle  border_style = BorderStyle::Solid;

    // ----- outline (focus ring etc.; never participates in layout) -----
    float        outline_width = 0.f;     // 0 ⇒ unset
    Color        outline_color{};         // a==0 ⇒ unset
    float        outline_offset = 0.f;    // signed; meaningful only if outline_width>0

    // ----- effects -----
    BoxShadow    shadow{};                // shadow.color.a==0 ⇒ unset
    GradientSpec gradient{};              // gradient.stop_count==0 ⇒ unset
    BackgroundImage bg_image{};           // bg_image.texture_id==0 ⇒ unset
    float        opacity = 1.f;           // 1 ⇒ no layer
    Visibility   visibility = Visibility::Visible;  // INHERITED
    bool         z_index_set = false;     // sentinel for "auto" vs explicit z
    int16_t      z_index = 0;

    // ----- typography (INHERITED) -----
    Color        text{};                  // a==0 ⇒ unset/inherit
    uint16_t     font_id = 0;             // 0 ⇒ unset/inherit
    uint16_t     font_size = 0;           // 0 ⇒ unset/inherit
    int16_t      letter_spacing = 0;      // 0 default == "none" (also v1 render limit)
    float        line_height = 0.f;       // 0 ⇒ unset/inherit (⇒ font's natural skip)
    TextAlign    text_align = TextAlign::Left;  // INHERITED
    TextWrap     text_wrap = TextWrap::None;     // None | Words

    // ----- container-driven (resolved, not inherited) -----
    Vec2         scroll_offset{};         // §A.4
    BetweenChildren between_children{};    // §A.3 (width==0 ⇒ none)
    // text/caret/selection colors come from theme tokens, not per-node (C.2)
};

struct BoxShadow { Color color{}; Vec2 offset{}; float blur=0, spread=0;
                   ShadowKind kind=ShadowKind::Drop; };
struct GradientSpec { GradientKind kind=GradientKind::Linear; float angle_deg=0;
                      uint8_t stop_count=0; GradientStop stops[UI_MAX_GRADIENT_STOPS]{}; };
struct BackgroundImage { uint32_t texture_id=0; Color tint{255,255,255,255};
                         SideWidths nine_slice{}; };
struct BetweenChildren { float width=0; Color color{}; };
```

**Sentinel table (the "unset ⇒ inherit/skip" rule for every field).** This is the single source of truth that makes the dense cascade work:

| Field | "unset" sentinel |
|---|---|
| `background`, `text`, each `border_color` side, `outline_color`, `shadow.color`, `bg_image.tint`-as-override | `Color.a == 0` |
| `radius`, `border_width`, `between_children` | `.any() == false` / `width == 0` |
| `font_id`, `font_size` | `== 0` |
| `line_height` | `== 0.f` |
| `gradient` | `stop_count == 0` |
| `bg_image` | `texture_id == 0` |
| `outline_width` | `== 0.f` |
| `z_index` | `z_index_set == false` |
| `opacity`, `visibility`, `text_align`, `text_wrap`, `border_style` | enum/scalar defaults; treated as "explicitly default" — these are **not inherited** (except `visibility`/`text_align`, see B.4), so dense overwrite is safe |

A component that genuinely wants "transparent on purpose" sets `opacity` to fade, or uses a fully-transparent value it does not care to inherit; for the rare "explicitly transparent text" case, authors set `text = {0,0,0,1}` (a==1) which is visually transparent yet not the unset sentinel. This corner is documented in the authoring guide (§F).

### B.2 Sparse override layers

```cpp
template <class T> struct Opt { bool set=false; T value{}; };

// Sparse overlay over VisualStyle: only fields the author touched.
struct StyleOverride {
    Opt<Color>        background, text, outline_color, gradient_color0 /*…*/;
    Opt<CornerRadius> radius;
    Opt<SideWidths>   border_width;
    Opt<SideColors>   border_color;
    Opt<float>        outline_width, outline_offset, opacity, line_height;
    Opt<Color>        shadow_color; Opt<Vec2> shadow_offset; Opt<float> shadow_blur, shadow_spread;
    Opt<uint16_t>     font_id, font_size;
    Opt<int16_t>      letter_spacing, z_index;     // z_index Opt also sets z_index_set
    Opt<Visibility>   visibility;
    Opt<TextAlign>    text_align;
    Opt<TextWrap>     text_wrap;
    // (gradient/bg_image overrides expressed as whole-struct Opt for brevity)
    Opt<GradientSpec>    gradient;
    Opt<BackgroundImage> bg_image;
    Opt<BetweenChildren> between_children;
    Opt<Vec2>            scroll_offset;
};
```

`apply(VisualStyle& dst, const StyleOverride& o)` writes only `set` fields. This is the **field-wise conditional overlay** used by every sparse layer (theme states, base-as-sparse-for-inherited, and the per-state interaction layers).

### B.3 StatefulStyle (what a component authors)

```cpp
struct StatefulStyle {
    VisualStyle    base{};           // dense; the component's source-of-truth defaults
    StyleOverride  hover{};
    StyleOverride  focus{};
    StyleOverride  focus_visible{};
    StyleOverride  checked{};
    StyleOverride  active{};
    StyleOverride  disabled{};

    // implicit conversion so legacy `.style = {Style}` sites keep compiling (§F/§G)
    StatefulStyle() = default;
    StatefulStyle(const VisualStyle& b) : base(b) {}   // NOTE: ctor ⇒ not aggregate; see §F
};
```

`merge_style(StatefulStyle variant, StatefulStyle caller)` (the explicit caller-override primitive, §F.2): **per-layer sparse overlay** — `base` is merged field-wise treating the caller's base via its **sentinels** (only non-sentinel caller fields override the variant; unset caller fields leave the variant intact), and each state layer is a sparse `StyleOverride`-onto-`StyleOverride` overlay (caller's `set` fields win, others survive). A caller passing only `background` keeps the variant's border, radius, hover, etc. (Worked example in §F.2.)

### B.4 The inherited set

Inherited (seeded from parent's *resolved* `VisualStyle`): `text`, `font_id`, `font_size`, `letter_spacing`, `line_height`, `text_align`, `visibility`. **Not** inherited: background, radius, border, outline, shadow, gradient, bg_image, opacity, z_index, scroll_offset, between_children, text_wrap. `disabled` is **not** an inherited field — it is a per-node interaction state (§D.2) — but a disabled control's **resolved dimmed `text` color is inherited** by descendant `Text` nodes, which is exactly how the old `inherited_disabled` thread is reproduced (worked example in §B.5).

### B.5 The cascade (exact algorithm)

`resolve(node, parent_resolved, interaction, theme) -> VisualStyle`:

```
out = VisualStyle{}                                  # L0: initial values (all sentinels)

# L0b — theme dense role defaults (visual only; never layout)
apply_dense_visual(out, theme.defaults_for(node.role).base)
#   ^ field-wise: writes every NON-sentinel field of the theme role base.
#     (Theme carries VisualStyle, NOT Style — so no layout field exists to clobber. §C.)

# L1 — inheritance: seed ONLY the inherited set from the parent's resolved style
for f in INHERITED_SET:
    if is_unset(out.f):            # don't override a theme default that is meaningful
        out.f = parent_resolved.f  # text/font/letter/line_height/align/visibility

# L2 — component base (dense struct, but applied FIELD-WISE CONDITIONALLY via sentinels)
apply_dense_visual(out, node.style.base)
#   ^ CRITICAL: this is NOT a whole-struct copy. apply_dense_visual writes a field
#     ONLY if the base's field is non-sentinel. So a base that leaves text unset does
#     NOT clobber the inherited text color; a base that sets text DOES override it.

# L3 — sparse interaction layers, fixed precedence (low -> high)
if interaction.hover         : apply(out, node.style.hover)
if interaction.focus         : apply(out, node.style.focus)
if interaction.focus_visible : apply(out, node.style.focus_visible)   # gated, see D.2
if interaction.checked       : apply(out, node.style.checked)
if interaction.active        : apply(out, node.style.active)
if interaction.disabled      : apply(out, node.style.disabled)

# legacy scalar fold (migration only; §G): if a node still carries the old single
# Style.text scalar and out.text is unset, fold it in here. Removed after Phase 3.

return out
```

`apply_dense_visual(dst, src)` is the field-wise conditional copy that respects every sentinel in the §B.1 table. This single function resolves **both** the blocker ("dense base clobbers inheritance") and the major ("theme dense overlay clobbers"): nothing is ever a whole-struct copy of a `VisualStyle`; every dense application is sentinel-gated. Layout fields are simply not in `VisualStyle`, so they cannot be touched.

**Worked example 1 — nested text inheritance (proves inheritance survives a base).**
Parent `<Box style={{ text:{220,220,220,255}, font_size:16 }}>` resolves `out.text={220…}`, `out.font_size=16`. Child `<Text>"hi"</Text>` with **no** color/size in its base: L0b theme `text.base` may set nothing for `text` (unset), L1 seeds `out.text={220…}, out.font_size=16` from parent, L2 base is all-unset for text ⇒ `apply_dense_visual` writes nothing ⇒ the inherited values survive. The child renders at `{220…}/16`. (Golden test in §H covers this exact case.)

**Worked example 2 — disabled control dims its text (replaces `inherited_disabled`).**
`<Button disabled>` has `interaction.disabled=true`. The button's `StatefulStyle.disabled` layer (from the variant, seeded by theme) sets `text = theme.button.disabled.text` (the dimmed color). L3 applies it ⇒ the button's resolved `out.text = dimmed`. The button's child `<Text>` inherits via L1 (`out.text` unset on the child base ⇒ seeded from parent's dimmed `out.text`). Result: the label renders dimmed, with **no** per-node disabled bit on the `Text` node — exactly the old behavior. The chain is guaranteed because (a) `theme.button.disabled.text` is set in `default_theme()` (§C.2 table) and (b) `text` is inherited (§B.4). A §H test asserts a disabled Button's child Text resolves to the dimmed color.

---

## C. Theme provider

### C.1 Type

```cpp
namespace ui::design {
struct RoleStyle {
    VisualStyle   base{};            // role defaults (visual only)
    StyleOverride hover, focus, focus_visible, checked, active, disabled;
};
struct Theme {
    RoleStyle generic, box, button, input, checkbox, text, dialog;   // by NodeRole
    // global tokens (not per-node):
    Color text_default{}, text_disabled{};
    Color caret{}, selection{};
    Color focus_ring{};
    const RoleStyle& defaults_for(NodeRole r) const;   // maps role -> RoleStyle
};
const Theme& default_theme();
}
```

`Theme` carries `VisualStyle`/`StyleOverride`, **never** `Style` — so the "theme dictates geometry" hazard is structurally impossible (resolves the §B/C blocker about layout clobber).

### C.2 default_theme() — complete constant table (the bit-for-bit guarantee)

Every current literal maps one-to-one. `kButton*` (in `draw_list.cpp`) and `kControl*` (in `common.h`) are **reconciled**: they describe the same control surface from two call paths; `default_theme()` uses the `draw_list.cpp` values as canonical and the component helper `control_style` is rewritten to read the theme, deleting the `common.h` duplicates (§G Phase 5).

| Current source literal | Value (RGBA / px) | Destination token / field |
|---|---|---|
| `kButtonFill` (draw_list) ≡ `kControlFill` (common.h) | `{24,28,36,255}` | `button.base.background` |
| implicit 1px control border (append_rect `control_box`) | `1.0f` all sides | `button.base.border_width = SideWidths::all(1)`; same for `input`, `checkbox` |
| `kControlBorder` | (current value) | `button.base.border_color = all(kControlBorder)` |
| `kControlDisabledFill` | (current value) | `button.disabled.background`, `input.disabled.background`, `checkbox.disabled.background` |
| `kControlDisabledBorder` | (current value) | `button.disabled.border_color`, etc. |
| `kTextFill` | (current value) | `text_default`; `text.base.text`; `button.base.text` |
| `kTextDisabledFill` | (current value) | `text_disabled`; `*.disabled.text` (button/input/checkbox/text) |
| `kFocusBorder` | (current value) | `focus_ring`; and **per-role** `*.focus_visible.outline_color = focus_ring`, `*.focus_visible.outline_width = <ring width>`, `*.focus_visible.outline_offset = <inset, see E.3>` for **generic, box, button, input, checkbox, text, dialog** (the old ring is role-independent) |
| checkbox checked fill | (current value) | `checkbox.checked.background` |
| selection color (append_input_contents) | (current value) | `selection` |
| caret color (append_input_contents) | (current value) | `caret` |
| `kInsetX = 8` | `8` | **text inset**, not layout padding — see note below |
| `kTextHeight = 16` | `16` | default line box height fallback (text metrics) |
| `kCharWidth = 8` | `8` | **removed** — caret/selection now use measured advances (intentional change, §G/§H) |

**`kInsetX` does not become layout padding** (resolves the "layout regression" finding). It was a render-time text inset, and moving it to Yoga `padding` would change measured box sizes. We keep it as a **non-layout text inset** applied by the builder when positioning input/text content within the already-laid-out box, preserving today's geometry exactly.

**Generic/box focus ring** is included explicitly (resolves the "focused generic box" finding): because the old ring fires for *any* focused non-disabled node regardless of role, `generic` and `box` carry `focus_visible.outline_*` identically. A §H golden covers a focused `role=Generic` box.

**Golden coverage statement (so uncovered constants are not assumed safe):** the Phase-2 golden set covers, at minimum: a button (normal/hover/focus/disabled), a checkbox (checked/unchecked/focused), an input (with selection + caret + clipped overflow), a plain Box with nested Text (inheritance), a **focused generic box**, and a disabled Button with a child Text. Any constant not exercised by these is flagged in the migration checklist for manual diff.

### C.3 Delivery (single, consistent mechanism — nested themes supported)

**Decision (resolving the either/or fork):** the theme is delivered by **context lookup carried as a recursion parameter**, *not* by widening the committed `NodeSnapshot`/reconciler. `ThemeProvider` is an ordinary provider/context component (existing provider/context machinery). The resolver obtains the **nearest-ancestor** theme as a `const Theme*` passed down the `build_draw_list` DFS:

- `BuildContext` (§D.1) carries `const Theme* theme` as the **current** theme.
- When the DFS enters a node that is a `ThemeProvider` (detected via the existing context chain queryable from the snapshot), it pushes the provider's theme as the new current `theme` for the subtree and restores it on return. This supports **nested** `ThemeProvider`s — the single pointer is correct because it is a *recursion parameter*, re-seeded per subtree, not one global.
- The reconciler and `NodeSnapshot` are **untouched** (consistent with §D.2/§I). The only requirement is that context is queryable from the snapshot during the DFS, which the provider/context system already supports.

§D.2 and §I are aligned with this: `BuildContext.theme` is mutated push/pop during the DFS; there is no single immutable global theme.

---

## D. Runtime plumbing

### D.1 BuildContext

```cpp
struct BuildContext {
    const design::Theme* theme = &design::default_theme();  // current (nested) theme, §C.3
    uint32_t focused_id   = 0;     // single focused node (focus is genuinely single-node)
    bool     focus_visible = false;// keyboard-driven focus this frame (gates :focus-visible)
    Vec2     pointer{};            // current pointer position in layout space
    bool     pointer_down = false; // for :active ancestor-chain
    MeasureTextFn measure = nullptr;  // §D.4 (same fn installed as Yoga MeasureFn)
};
```

### D.2 Per-node interaction (point-in-rect, not single-scalar)

Resolves the major finding: `:hover`/`:active` are computed by **point-in-rect against each node's laid-out box during the DFS**, restoring CSS ancestor-chain semantics; `:focus`/`:focus-visible` stay id-based (focus is single-node by definition).

```cpp
struct NodeInteraction {
    bool hover=false, active=false, focus=false, focus_visible=false,
         checked=false, disabled=false;
};

NodeInteraction interaction_for(const NodeSnapshot& n, const Rect& box,
                                const BuildContext& ctx) {
    NodeInteraction it;
    bool inside = box.contains(ctx.pointer);     // ancestor-chain: true for the node AND
                                                 //  every ancestor whose box contains pointer
    it.disabled      = n.disabled;               // per-node bit on the control
    it.hover         = inside && !it.disabled;
    it.active        = inside && ctx.pointer_down && !it.disabled;
    it.focus         = (n.id == ctx.focused_id) && !it.disabled;
    it.focus_visible = it.focus && ctx.focus_visible;
    it.checked       = n.checked;
    return it;
}
```

`FocusRuntime` gains `focused_id` (already implied) and the frame's `focus_visible` flag and `pointer`/`pointer_down`; `NodeInteraction` is **not** widened with persisted hovered/pressed scalars (no `hovered_id`/`pressed_id` needed — they were the single-scalar half-measure). Cost is one `Rect::contains` per node per frame.

### D.3 Per-node emission order

For each visited node, after `resolve()` yields `VisualStyle r` and `interaction`:

```
if r.visibility == Visible:
    1. Shadow (drop)             if r.shadow set && kind==Drop
    2. Rect (background)         if r.background set            (Clay: solid fill)
    3. Gradient                  if r.gradient set
    4. Image (bg_image)          if r.bg_image set              (tinted; §B.6)
    5. Shadow (inset)            if r.shadow set && kind==Inset
    6. Border                    if r.border_width.any()
    7. Text / Input contents     per wrapped line (§D.6), text inset applied (kInsetX)
# children always recurse (layout slots preserved even when this node is Hidden):
8. push Layer  if r.opacity < 1                       (LayerPush)
9. push Clip   if overflow clips || rounded clip      (ClipPush)
10.   for child in paint_order(node):                 (§D.5 hierarchical z)
          visit(child, child_rect translated by -r.scroll_offset)   # §A.4
      emit between_children dividers between adjacent children       # §A.3
11. pop Clip   (ClipPop)
12. pop Layer  (LayerPop)
if r.visibility == Visible:
    13. Outline (focus ring etc.)  if r.outline_width > 0   <-- GATED by Visible (resolves finding)
```

**Visibility:hidden is inherited and gates the whole subtree + outline** (resolves the minor finding): `visibility` is in `INHERITED_SET`, so a child of a `Hidden` parent inherits `Hidden` (and skips its own paint) unless it explicitly sets `Visible`. Children still recurse (layout preserved). The **outline** is moved inside `if Visible` so a hidden-but-focused control shows no ring.

### D.4 MeasureTextFn — the SDL-free measurement seam (single mechanism for layout + draw)

Resolves the "two un-reconciled measure phases" and "per-line geometry hand-wave" blockers/majors. There is **one** measurer, renderer-backed but injected as a function pointer so `ui/` stays SDL-free. It is installed as the **per-node Yoga `MeasureFn`** for Text/Input nodes (so wrapping drives layout height) **and** reused inside `build_draw_list` for per-line emission, caret, and selection geometry.

```cpp
struct TextMetricsQuery {
    const char* utf8; uint16_t len;
    uint16_t font_id; uint16_t font_size;
    int16_t  letter_spacing;      // v1: measurer MUST ignore (==draw, see J.3)
    float    line_height;         // 0 ⇒ font natural skip
    TextAlign align;
    TextWrap  wrap;
    float     wrap_width;         // available width (for Words wrap); INF for None
};

struct LineRun {                  // one wrapped line, alignment pre-baked into x
    uint16_t slice_offset;        // byte offset into the query's utf8
    uint16_t slice_len;
    float    x, y, w, h;          // box of this line, RELATIVE to the text content origin
};

struct TextMetricsResult {
    float    width = 0, height = 0;     // total block size (feeds Yoga MeasureFn)
    uint16_t line_count = 0;
    LineRun  lines[UI_MAX_TEXT_LINES];  // fixed buffer (house style)
    bool     overflowed = false;        // line_count would exceed UI_MAX_TEXT_LINES
};

using MeasureTextFn = TextMetricsResult (*)(const TextMetricsQuery&);
void set_text_measurer(MeasureTextFn);   // global install; same fn fed to set_measure()
```

- **`UI_MAX_TEXT_LINES`** is a fixed cap (e.g. 64) sized against the sample screens plus headroom. `overflowed == true` is a hard, test-visible condition (raises `error_count`), never silent truncation.
- **Wrapping owner:** the renderer-side measurer performs manual word-wrap (`TTF_GetStringSizeWrapped` gives only a size, not break positions, so it is insufficient) — it iterates words, measures with the cached `(face,size)` font, and records break positions into `lines[]`. `line_height` and the v1 `letter_spacing==0` rule are applied here so **measure and draw agree exactly**.
- **Same font state in both phases** (resolves the minor finding): the measurer and the draw-time `TTF_Text` use the **identical cached `(face,size,style)` handle in the identical wrap-width / line-skip / hinting / alignment state**. Font-level mutable state (`TTF_SetFontWrapAlignment`, `TTF_SetFontLineSkip`) is set consistently or avoided in favor of per-text settings. A §H test asserts measured size == `TTF_GetTextSize` of the rendered `TTF_Text` for wrapped multi-line input.
- **Installer/ownership:** the app installs the renderer-backed measurer once at init via `set_text_measurer`, and the reconciler calls `set_measure(node, yoga_text_measure_adapter)` for Text/Input nodes (the adapter calls the same global `MeasureTextFn`). A single global function pointer matches the existing `MeasureFn` style; it is set once before any frame, so no thread/ownership race (UI runs single-threaded per the project).

### D.5 paint_order(node) — the hierarchical z helper

Implements §A.8: returns the node's direct children in CSS painting order for the node's stacking context (negatives asc → node's own box already emitted by D.3 steps 1–7 → zero/auto in doc order → positives asc, stable on sibling index). `visit` then emits each child's whole bracketed range contiguously. No global sort.

### D.6 The chosen text model: per-line emission (single, unambiguous)

**Decision (resolving the split-brained text blocker):** per-line emission is the model. `build_draw_list`:
1. Calls `ctx.measure(query)` for the Text/Input node → `TextMetricsResult`.
2. For each `LineRun`, interns that line's slice into `text_arena`, computes `fingerprint = fnv1a(slice)`, and emits one `Text` command with `rect = {content_origin + line.x + kInsetX_text, content_origin + line.y, line.w, line.h}`, `arena_offset/len`, `line_index`, resolved `color/font_id/font_size`.
3. Caret/selection geometry uses the same `LineRun` data + measured advances (§E.5/F.3).

The alternative "single TTF_Text with wrap width" is **dropped** entirely; §E.5 is rewritten to the per-line model so command count, alignment, caret/selection, and goldens are all single-valued.

---

## E. Renderer

### E.0 Alpha convention (pipeline-wide invariant)

Resolves the "no pipeline-wide alpha convention" and "double-blend feather" majors. **The entire pipeline is premultiplied-alpha:**

- All `Color` values are **premultiplied at emit time** in the builder (or at upload for textures). Vertex colors fed to `SDL_RenderGeometry` are premultiplied.
- Every feathered/geometry/text/image/shadow/layer draw uses **`SDL_BLENDMODE_BLEND_PREMULTIPLIED`**.
- Offscreen layers (§E.7) are **cleared with `SDL_BLENDMODE_NONE`** to `(0,0,0,0)` and **composited** with `SDL_BLENDMODE_BLEND_PREMULTIPLIED` — no edge double-darkening.
- `stb_image` straight-alpha RGBA and `TTF` straight-alpha glyph output are **converted to premultiplied** at texture-upload / glyph-cache time (the one straight→premultiplied conversion point, documented here).

This single convention eliminates AA halos and layer-composite seams that the golden tests would otherwise catch only post-implementation.

### E.1 Dispatch

The renderer is a dumb linear executor over `DrawList::cmds[0..count)`, switching on `kind`. `ClipPush/Pop` and `LayerPush/Pop` manipulate fixed stacks. Per-target SDL state is saved/restored around `SetRenderTarget` (§E.7).

### E.2 Clip stack

```cpp
struct ClipStack { SDL_Rect rects[UI_MAX_CLIP_DEPTH]; int depth=0; };
```

- `clip_push(rect, radius)`: `round_out` the float rect (floor origin, ceil extent) to integer `SDL_Rect`, **intersect with the current top**, push, `SDL_SetRenderClipRect`.
- **Rounding rule + consequence (resolves the 1px finding):** `round_out` makes the clip up to ~1px **larger** per side (leak outward), so hairline overflow can leak by ≤1px; intersection compounds rounding with depth. We accept this for the integer-clip fast path and add a **1px-overflow-into-clip golden test** (§H) that pins the behavior. Rounded clips (`radius.any()`) that need exact edges route through the layer path (offscreen, clipped at composite) when correctness demands it.
- **Transform interaction wired now (resolves the deferred-trap finding):** §J.1 documents that any **transformed** subtree that also clips MUST use the offscreen-layer path, never `SDL_SetRenderClipRect` (axis-aligned integer clip cannot clip a rotated box). The `LayerData.m[6]` seam and this rule are recorded so the deferral is not a hidden trap.

### E.3 Geometry: rounded fill, border band, outline, co-feathering

**Exact Clay topology for the uniform-radius case (resolves the topology/segment-count regression):** rounded fill = **center rect inset by clamped radius + 4 corner fans + 4 edge quads**, `segments_per_corner = max(16, (int)ceil(radius * 0.5f))` (matching Clay's `max(16, radius*0.5)`; the draft's `max(2,…)` is rejected as a faceting regression). Per-corner UV `(0,0)` for solid fills.

**Per-corner / elliptical case (Clay superset):** each corner samples its own ellipse arc (`tl_x,tl_y` … independently), `segments_per_corner` derived from `max(16, ceil(max(rx,ry)*0.5))`, capped at `UI_MAX_ARC_SEGMENTS` (§E.9 budget). The four arcs + four edges + center region triangulate into a **single** ring/fill mesh (not a naive center-fan, which is unsafe for asymmetric corners).

**Co-feathered single pass (resolves the double-blend seam major):** a node's **fill + border band + one shared 1px outer fringe** are generated as **one** `SDL_RenderGeometry` mesh with a single AA boundary. Fill interior is opaque; the border band carries its per-side color; the outer fringe vertices have alpha 0 (premultiplied → `(0,0,0,0)`). There is exactly **one** feather edge, so fill and border never composite their fringes over each other. (When a node has fill but no border, the same single-pass mesh is emitted without the band.)

**Per-side multi-color / multi-width rounded corners (resolves the "one sentence" major).** Full spec:
- The **outer** ring follows the box's outer `CornerRadius`. The **inner** ring radius per corner is `inner = max(0, outer − adjacent_side_width)`; because adjacent sides may differ, the two inner radii meeting at a corner differ — we generate the inner contour by sampling each side's inner ellipse up to the corner and **joining them at the corner's color/geometry boundary**.
- The color boundary at a corner is **not** a fixed 45°; it falls at the angle `θ = atan2(w_v, w_h)` determined by the two adjacent side widths `w_v` (vertical side) and `w_h` (horizontal side) (the CSS miter bisector for unequal widths). Vertices on the arc before `θ` take the first side's color, after `θ` the second side's; the seam vertex is duplicated so the color is a hard split (matching CSS solid-border corners).
- AA feather at the multi-color seam: the shared outer fringe carries the **interpolated** premultiplied color across the two-vertex seam over the 1px band, avoiding a visible notch.
- Worked example (top=4 red, left=2 blue, radius=8): outer radius 8 both arcs; inner radius at the top-left corner is `max(0,8−4)=4` along the top contribution and `max(0,8−2)=6` along the left; seam angle `θ=atan2(2,4)≈26.6°` from the top edge; arc vertices < θ red, > θ blue. (Multi-color corners are **beyond Clay** — Clay had a single border color — so they cannot be a Clay regression; they are included because decision #3 makes per-side authorable. If exact CSS miter ever proves too costly, this is the one spot that can degrade to a simple split with a documented delta; the rest of the design does not depend on it.)

**Outline (focus ring) — exact offset-radius math (resolves the geometry finding):** the outline is a rounded **ring** computed from the **border-box rect expanded by `outline_offset`** with per-corner radii `= base_radius + outline_offset` (clamped `>= 0`), width `= outline_width`, painted in step 13 (after children). This differs from the border **band** (which insets and uses `inner = max(0, outer − side)`).

**Focus-ring parity is an INSET match, not an outset (resolves the inset-vs-outline blocker-adjacent major).** The old ring is a 2px **inset** `SDL_RenderRect` loop (eats inward). To reproduce it pixel-for-pixel at the Phase-2/3 boundary, the theme sets `*.focus_visible.outline_offset = −outline_width` (pull inward by the width) so the new outline occupies the **same** pixels as the old inset border. A §H golden asserts the migrated ring equals commit `a7457b5`'s restored-focus-ring pixels. (If a future redesign wants an outset ring, that is an explicit, separately-golden change — the zero-loss claim is honored only with the inset offset.)

### E.4 Images / TextureCache

```cpp
struct TextureCache { /* texture_id -> SDL_Texture*; owns lifetime; premultiplied upload */ };
```

- **Path selection predicate (explicit):** `radius.any() == false && nine_slice.any() == false` ⇒ **plain path** (`SDL_RenderTexture`); otherwise the **geometry path** (tessellated rounded fill textured with the image, UVs mapped) for rounded, and **9× `SDL_RenderTexture`** for nine-slice.
- **Tint math, both paths identical (resolves the minor finding):** plain path uses `SDL_SetTextureColorMod`/`SetTextureAlphaMod` with **mandatory restore to 255** after the draw (shared cache). The geometry path applies tint **per vertex** as `SDL_FColor = premul(tint)` (because `SDL_RenderGeometry` ignores color/alpha mod) — the multiply matches colormod (`vertex = tint/255` in straight terms, premultiplied here). A §H test compares a tinted square vs tinted rounded render of the same source to catch divergence.
- **rounded + nine-slice combination:** v1 renders nine-slice **without** corner rounding (the two are independent fields; combining is rare). Declaring both is **not** a Clay regression (Clay had neither rounded nor tinted-rounded nine-slice); it is documented as deferred (§J) with a debug warning, never silently wrong.

### E.5 Text path (retained TTF_Text), cache, clip, fonts

**Font table:** fonts are keyed `(face_id, ptsize, style)` and **owned** per key. `TTF_SetFontSize` is **NEVER** called on a live shared font (it would re-lay every `TTF_Text` bound to it); a size change selects a **different** cached `(face,size)` handle.

**TTF_Text cache — full identity and change-gate (resolves 3 majors):**
- **Cache identity = `(node_id, line_index, face_id, ptsize, style)`.** Including `(face,size,style)` means a resolved-font change (via `:hover`/`:checked` typography, nested `ThemeProvider`, responsive size) **rebinds**: `TTF_DestroyText` the old + `TTF_CreateText` against the new `(face,size)` font. A font change is a **rebuild, never an in-place update** (no API can repoint a `TTF_Text`'s font).
- **Change-gate for string = the content `fingerprint`** stored on the entry, **never the per-frame arena offset/pointer** (the arena is rebuilt every frame so offsets always differ — comparing them would reshape every frame). Call `TTF_SetTextString` **only** when `fingerprint` differs; `TTF_SetTextColor` only when resolved color differs; `TTF_SetTextWrapWidth` only when wrap width differs. (Per-line emission means wrap-on-the-text is off; wrapping is done by the measurer and each line is its own single-line `TTF_Text`, so `SetTextWrapWidth` is effectively unused in the per-line model — kept for completeness.)
- **Eviction = mark-and-sweep by `(node_id, line_index)` presence each frame.** A node going 3→2 lines drops the stale `line_index=2` entry (resolves the per-line leak finding). Node-id churn (keyed-sibling reorder) is handled by the same sweep: entries whose `node_id` was not touched this frame are destroyed. A §H `renderer_text_cache_tests` asserts (a) N identical frames ⇒ **exactly one** create and **zero** `SetTextString` calls (no per-frame reshape), and (b) a reflow that reduces line count leaks **zero** `TTF_Text` objects, and (c) a font-size state change triggers exactly one rebuild.

**Draw + clip (resolves the clip blocker with a verified fact, not an assumption):** text is drawn with `TTF_DrawRendererText(text, x, y)` on the renderer's text engine, **inside** any active `ClipPush/ClipPop`. **Verified by test, not assumed:** a §H golden renders text larger than a clipped container and asserts **zero glyph pixels outside the clip rect**, confirming `SDL_SetRenderClipRect` is honored by `TTF_DrawRendererText` on the shared renderer. **Documented fallback (designed now, not deferred):** if the verification ever fails on a target, clipped text routes through the offscreen-layer path (§E.7), clipped at composite time (or retains the texture-blit path under any active clip). This is wired into Phase 4a's gate so it cannot surface late.

**Caret/selection (intentional change, see §G/§H):** computed from measured per-glyph advances via the measurer/`LineRun` data + `TTF_GetTextSize` of substrings (not the old flat `kCharWidth=8`).

### E.6 Custom escape hatch — type-safe + state contract + snapshot lifetime

```cpp
using CustomRenderFn = void(*)(SDL_Renderer*, const Rect&, Color tint,
                               CornerRadius, const void* payload);
struct CustomRegistry { CustomRenderFn fns[UI_MAX_CUSTOM_CALLBACKS]; int count=0; };
uint16_t register_custom(CustomRenderFn);   // returns callback_id
```

- **Payload lifetime = same discipline as text (resolves the minor "use-after-free" finding):** the custom payload is **snapshotted into a `DrawList`-owned `custom_arena`** at emit time (`payload_id` is the offset), exactly like the text arena, because the tree mutates between build and render. No "app-owned pool index" that could be freed/reordered between phases.
- **SDL state contract (resolves the escape-hatch hazard):** before invoking the callback the renderer establishes the active **clip** (and future transform) and current layer/target; **after** the callback the renderer **saves and restores ALL mutable SDL render state** (draw color, blend mode, clip rect, render target, texture mods). The callback is documented as ideally state-neutral; a debug build asserts state is unchanged. Custom content **is clipped by the active clip stack and composited into the current layer/opacity** like any other subtree.
- **callback_id ↔ payload-type pairing:** `register_custom` is documented to imply a payload type; a mismatch is a programmer error caught by a debug assert. The `const void* payload` is acknowledged as the **one** intentional type-unsafe boundary (justified as the escape hatch; the broader "no raw void*" goal stands everywhere else). `CustomData.payload_id` (arena offset) and the header `user_data` are **distinct and non-overlapping**: `payload_id` is the custom draw payload; `user_data` is the optional general pass-through (which §J recommends eliding unless a concrete consumer exists).

### E.7 Group-opacity layers

- **Layer bounds = descendant-ink union (resolves the "geometry undefined" major).** Group opacity composites the union of all descendant ink (outlines/shadows extend beyond the border box). Because a single forward DFS cannot know descendant extents up front, we compute the union via a **bounded pre-pass** over the subtree's already-laid-out rects **inflated** by each node's outline/shadow extent. `LayerPush.rect` = that union (clamped to the viewport). This avoids clipping focus rings/shadows of an opacity group.
- **Per-target state saved/restored (resolves the major):** across `SDL_SetRenderTarget`, save **and** restore: clip stack, **logical presentation** (`SDL_SetRenderLogicalPresentation`), **render scale**, **viewport**, draw color, draw blend mode. **Verified baseline fact (not an open audit item):** the current renderer setup (`platform/sdl/window.cpp:17,22`) calls only `SDL_CreateRenderer` + `SDL_SetRenderVSync` — it uses **neither** `SDL_SetRenderLogicalPresentation` **nor** a render scale today. Therefore in v1 the layer texture is sized in **output pixels** and child clip rects inside the layer are output-pixel coordinates. The save/restore of logical-presentation/scale/viewport is implemented **defensively** so that if logical presentation is ever introduced, the layer path is already correct (and the §H HiDPI golden below would catch a regression). A **HiDPI/logical-presentation golden** for an opacity-layered subtree is in §H, gated on if/when logical presentation is enabled.
- **Clip across the switch:** entering the layer target resets the clip to the full target; the saved clip stack is restored on `LayerPop`. Child clip rects inside the layer are in **target-local** coordinates.
- **Pool sizing / budget / eviction (resolves the VRAM-churn major):** target textures are pooled by **size bucket with hysteresis** (small bounds changes do **not** reallocate — a bucket only changes past a threshold), keyed by `node_id`. A **max concurrent-layer count** and a **total-VRAM budget** are enforced; on exceedance the **documented degraded fallback** is: skip the layer and premultiply the group opacity into the subtree's leaf colors (accepting the documented double-blend at overlaps) — never crash, never unbounded allocation. A **per-frame cap on render-target switches** is enforced. §H adds the two realistic worst cases: **many list rows each `opacity<1`** and an **animating-opacity modal** (bounds/contents change per frame).

### E.8 Shadows

- **First-ship path:** feathered quads via the tessellator (follows **per-corner** radii directly — symmetry-safe).
- **Nine-slice Gaussian fast path (reconciled with per-corner radii, resolves the major):** nine-slice is restricted to the **uniform-radius** case (cache key `{blur, spread, uniform_radius}`); the four corners are identical so one corner sprite reuses correctly. **Per-corner / elliptical** shadows render as a **full blurred sprite** cached by the **full radius signature** (8 floats) or rendered per-frame for unique shapes — accepting the larger/again-keyed cache. The split is explicit; the feather-quad path remains the universal fallback and is symmetry-safe by construction.
- All shadow draws use premultiplied alpha (§E.0) to avoid halos.

### E.9 Vertex/index scratch budget (resolves the "command count ≠ vertex budget" major)

Two **separate** bounds:
1. **Command-list capacity** — §A.7 (derived).
2. **Per-frame vertex/index scratch** — a fixed buffer sized to a documented worst-case-per-node tessellation: `UI_MAX_ARC_SEGMENTS × 4 corners × 2 rings × feather_factor` vertices, plus the gradient/nine-slice/shadow expansions. `UI_MAX_ARC_SEGMENTS` is a **hard cap** (e.g. 64) so worst-case vertices per node are bounded and the fixed scratch buffer is deterministically sizeable. **Overflow policy:** clamp segment count down (graceful quality reduction) then, if still exceeded, **flush-and-continue** (emit the partial mesh, start a new batch) — **never silent geometry drop**. The buffer is sized so a single complex node (per-corner elliptical border, max segments, two rings, feather) fits without flushing in the common case.

---

## F. Component authoring API

### F.1 HostProps.style type change

`HostProps.style` becomes `StatefulStyle` (was `Style` at `element.h:71`). The implicit `StatefulStyle(const VisualStyle&)` ctor (and a `StatefulStyle(const Style&)` adapter during migration, §G) keeps existing `.style = {…}` call sites compiling. **Aggregate note (resolves the F/G finding):** because `StatefulStyle` has a user-declared converting ctor it is **not** an aggregate; §F.2 examples therefore use explicit member init (`StatefulStyle s; s.base = …; s.hover = …;`) rather than designated init. A Phase-0 `static_assert(std::is_trivially_copyable_v<StatefulStyle>)` and a compile check of the cppx-generated Props struct (it stores a `StatefulStyle` field, verified against `tools/editor/cppx` codegen in Phase 2) gate the change.

### F.2 Button before/after (explicit variants, no boolean-prop soup, focus ring declared)

```cpp
// BEFORE (focus ring injected behind the back; file-level constants):
//   <Button style={ control_style(props) } />   // ring added in build_draw_list

// AFTER — component is the source of truth:
StatefulStyle button_variant(ButtonVariant v) {
    StatefulStyle s;
    s.base.background   = theme_token_or(/*…*/);     // dense base, sentinels for unset
    s.base.border_width = SideWidths::all(1);
    s.hover.background  = {/*…*/};                    // :hover layer (sparse)
    s.focus_visible.outline_width  = ring_width;     // FOCUS RING DECLARED, not injected
    s.focus_visible.outline_color  = /*theme.focus_ring via cascade*/;
    s.focus_visible.outline_offset = -ring_width;    // INSET parity (§E.3)
    s.disabled.background = /*…*/; s.disabled.text = /*dimmed*/;  // dims child Text via inherit
    return s;
}

StatefulStyle resolved = merge_style(button_variant(props.variant), props.style);
// merge_style: caller props override the variant, per-layer, field-wise via sentinels.
```

**`merge_style` worked example (resolves the "never defined" major):** `<Button variant=Primary style={{ background: red }} />`. The caller's `VisualStyle` has only `background` non-sentinel; `merge_style` overlays it field-wise onto `button_variant(Primary).base`, so `background` becomes red while the variant's `border_width`, `radius`, `hover`, `focus_visible`, `disabled` all **survive**. A caller passing a `StatefulStyle` with its own `hover` composes per-layer (caller's `hover.set` fields win; others survive). Granularity: base + each of the six state layers, every field gated by its sentinel/`Opt::set`.

### F.3 Input

Caret/selection authored via theme tokens (`caret`, `selection`); geometry from measured advances (§E.5). No hardcoded `kCharWidth`.

---

## G. Migration (keep-tests-green; Phase 4 split)

- **Phase 0 — Foundations (proofs first).** Land `DrawCommand`/`VisualStyle`/payload structs with the two `static_assert`s (`is_aggregate_v`, `is_trivially_copyable_v`) and `static_assert(sizeof(DrawList) < BUDGET)`. Renderer setup is already audited (verified fact in §E.7: `platform/sdl/window.cpp` uses neither logical presentation nor render scale, so v1 sizes layers in output pixels; the defensive save/restore lands anyway). No behavior change.
- **Phase 1 — Cascade + theme types, unused.** Add `VisualStyle` cascade, `Theme`, `default_theme()` with the **complete §C.2 table**; unit-test the cascade (inheritance, disabled-text, sentinels) in isolation. Renderer/builder untouched.
- **Phase 2 — Route builder through `resolve()` + theme; delete focus-ring injection in the SAME phase.** `build_draw_list` resolves via the cascade and theme; **delete `draw_list.cpp:59–66` injection and add `theme.*.focus_visible` simultaneously**, gated so **exactly one** ring is emitted (a §H test asserts one focus-ring command per focused node — no double ring, no missing ring). Legacy `Style`→`StatefulStyle` wrap (base-only) + legacy-scalar fold keep components compiling. **Phase-2 golden = zero visual change** for the §C.2 golden set (proves theme reproduces constants bit-for-bit, incl. focused generic box and disabled-Button-child-Text), **except** input caret/selection which is excluded (Phase 4e).
- **Phase 3 — Components author `StatefulStyle`.** Migrate screens/components to declare base + state layers; remove the legacy wrap and legacy-scalar fold. Focus ring now fully component/theme-owned. Goldens unchanged (inset-offset parity).
- **Phase 4 — Renderer fidelity, split into independently-gated sub-phases:**
  - **4a** Text cache + `(face,size,style)` font table + measure seam **behind identical pixels** (incl. the **clip-honored-by-text-engine** verification gate and fallback wiring).
  - **4b** Geometry rounded-rect + per-side border band + co-feather + exact topology; goldens change on this axis only.
  - **4c** Clip stack for overflow (+ 1px-leak golden) and `scroll_offset` plumbing.
  - **4d** Image TextureCache + tint (both paths) + nine-slice; `between_children` dividers.
  - **4e** Input caret/selection de-hardcoding to measured advances + theme tokens — **explicitly an intentional fidelity change**, excluded from the zero-loss golden; existing input-rect tests **rewritten** against a **deterministic mock measurer** (keeps `retained_ui_draw_list_tests` hermetic — no real TTF/font dependency).
- **Phase 5 — Cleanup.** Delete `common.h` `kControl*` duplicates (now theme-sourced); `control_style` becomes a thin theme adapter or is removed; remove `user_data` if no consumer materialized (§J).
- **Phase 6 — Opacity layers + shadows.** Land `LayerPush/Pop`, pool + budget + fallback, shadows. Add the opacity stress goldens.

Each phase compiles and passes `ctest`.

## H. Testing

New/extended targets (hermetic where possible; SDL-touching tests are golden/BMP):

- **`retained_ui_draw_list_tests`** (no SDL): cascade unit tests — nested-Text inheritance survives a base; disabled-Button child Text resolves dimmed; every sentinel; `merge_style` field survival; arena overflow ⇒ failed frame (no partial list, UTF-8-safe boundary); command-count worst case; **exactly one focus-ring command per focused node** across migration; per-line emission counts and pre-aligned x; uses a **deterministic mock `MeasureTextFn`** so input caret/selection asserts are portable.
- **`renderer_text_cache_tests`** (SDL): N identical frames ⇒ 1 create / 0 `SetTextString`; reflow 3→2 lines ⇒ 0 leaked `TTF_Text`; font-size state change ⇒ 1 rebuild; measured size == `TTF_GetTextSize` of the rendered `TTF_Text` (measure==draw).
- **`renderer_geometry_tests`** (golden BMP): exact rounded-rect topology vs baseline (≥16 segments, no faceting); co-feather corner has no seam/halo; multi-color/multi-width corner worked example; **focus-ring equals commit `a7457b5` pixels** (inset offset); tinted square == tinted rounded image math; 1px-overflow-into-clip; **text clipped — zero glyph pixels outside clip rect**.
- **`renderer_layer_tests`** (golden BMP): opacity group bounds = descendant-ink union (focus ring/shadow not clipped); HiDPI/logical-presentation opacity layer; **many rows each opacity<1** and **animating-opacity modal** stress (asserts no per-frame texture alloc thrash via hysteresis, and the degraded-fallback path when budget exceeded).
- **Phase-2/3 golden regression**: the §C.2 golden set proves zero visual change at the theme/de-injection boundary (caret/selection excluded).
- **Python CLI smoke** (`tests/ui_ci_smoke.py` via `tools/ui_cli.py`): unchanged screens still capture identical BMPs at the migration gates.

## I. Architecture compliance

- New code: `DrawCommand`/`VisualStyle`/cascade/`Theme` live under `src/ui/` and `src/ui/design/` (generic toolkit, **no** game vocabulary). Renderer additions live under `src/renderer/`. Game/client screens stay in `src/client/ui/`.
- `ui/` stays **SDL-free**: the only renderer dependency is the injected `MeasureTextFn` function pointer (and the typed `CustomRenderFn` registered by the app/renderer).
- **Reconciler/`NodeSnapshot` untouched** (consistent with §C.3/§D.2): theme is a DFS recursion parameter via the existing context chain, not a committed-node field. The only existing-machinery use is context queryability (already supported).
- No exceptions, no RTTI; fixed-size aggregates throughout; `static_assert`s pin triviality/aggregate-ness/size budgets under `-Wall -Wextra`.
- Deferred writes during layout still go through `client::ui::ClientUi::queue_deferred_write` (unaffected by this change).

## J. Deferred / out-of-scope (each justified)

1. **Affine transforms.** Seam reserved (`LayerData.m[6]`). Clay had no transform, so deferral is **not** a fidelity loss. **Trap pre-empted:** §E.2 documents that a future transformed+clipped subtree MUST use the offscreen-layer path (axis-aligned integer clip cannot clip a rotated box).
2. **Radial gradients.** `GradientKind::Radial` is **removed from the exposed style model** for v1 (resolves the "exposed but undefined" minor) — only Linear N-stop is authorable (exact per-vertex, Clay superset). Clay had no gradients, so this is not a loss. (The enum reserves the name internally but the style model does not surface it; declaring it is impossible, so there is no undefined-behavior surface.)
3. **`letter_spacing != 0` rendering.** v1 measures and draws with spacing **ignored, identically** (no measure/draw divergence — resolves that major). Justified **in-repo** (§0): the current `draw_list.cpp`/`sdl_retained_renderer.cpp`/`font_registry.cpp` apply no spacing and no screen sets it, so honoring only `0` is provable parity. Non-zero spacing (manual per-glyph layout for both measure and draw) is a future, flag-gated addition.
4. **`user_data` pass-through.** Retained in the header for Clay-shape parity but **may be elided in Phase 5** if no concrete consumer exists (YAGNI; it bloats every command). The §E.6 contract narrows Clay's transparent `void*` to a snapshot-arena handle; this is an acknowledged **deliberate contract narrowing** (consumers must register payloads, not stuff raw pointers) — acceptable because the project has **no** current custom/userData consumers; the "preserves Clay semantics" phrasing is corrected to "preserves Clay's *shape*; pointer pass-through is intentionally narrowed to a safe handle."
5. **rounded + nine-slice image combination.** Deferred (rare; neither existed in Clay). Declaring both emits a debug warning and renders nine-slice without rounding — never silently wrong, not a Clay regression.
6. **Exact CSS multi-color rounded-corner miter beyond the §E.3 spec.** The §E.3 algorithm is implemented; should the unequal-width seam ever prove too costly, the single documented fallback (hard split with a characterized delta) applies. Multi-color corners are beyond-Clay (Clay had one border color), so any simplification cannot be a Clay regression.

**Locked-decision flags (completeness, not philosophy):**
- **#2 (mirror `Clay_RenderData`):** honored in shape/spirit; deliberately **widened** to per-corner-elliptical radius and per-side border (width+color+style) because Clay's exact field widths would cap fidelity below CSS and below what components must author under decisions #1 and #3. This is a strict superset, called out so reviewers ratify the one place we diverge from a byte-for-byte mirror.
