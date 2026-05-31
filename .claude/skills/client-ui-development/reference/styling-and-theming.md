# Styling & theming reference

Ground truth: `src/ui/style/{visual_style,style_patch,theme,resolve,interaction,default_theme}.*`,
`src/client/ui/app_theme.*`, `src/client/ui/components/tokens.h`, `src/ui/components/common.h`.

## The two-stage model

- **`VisualStyle` (dense)** — what the renderer reads (`node.visual`). Aggregate of
  `background, corner_radius, border, outline, gradient, image, shadow, opacity, hidden, text`.
  Presence in the *resolved output* is read via value cues (`background.a==0` ⇒ no fill,
  `gradient.stop_count==0` ⇒ none). Colors are **straight alpha**; premultiplied once at IR emit.
- **`StylePatch` (sparse)** — the ONLY authoring overlay: one `Opt<T>` per `VisualStyle` field.
  **Authoring presence is ALWAYS `Opt::set`** — never a value-space sentinel. Compound members
  (`Border`/`Gradient`/…) are wrapped *whole*, not per-sub-field.
- **`patch()` builder** — fluent, makes the set-flag impossible to forget:
  `::ui::patch().background(c).border(b)`. Converts to `StylePatch` **and** directly to a
  base-only `StyleStatePatch`.
- **`StyleStatePatch { base, hover, focus_visible, pressed, checked, active, disabled }`** — a
  per-instance override with one `StylePatch` per interaction state. A bare `StylePatch`
  implicitly becomes the `base`-only form.

## `resolve()` — where paint is computed (inside the component, at authoring time)

```cpp
VisualStyle resolve(const RoleStyle &role, const StyleStatePatch &ov, const InteractionState &st);
```
Starts dense from `role.base`, applies `ov.base`, then for each **active** interaction state
applies the theme-role patch **first** and the per-instance override **second** (override wins
per field), in fixed order: `base → hover → focus_visible → pressed → checked → active →
disabled` (disabled wins last). Sparse and per-field — an override tweaks one field for one state
and inherits the rest from the theme role. **Never a dense full-`VisualStyle` override.**

The renderer never sees the `Theme` and never queries `ThemeContext` during the draw DFS — paint
is pre-resolved into `node.visual`.

## `InteractionState` — and the hover-on-Box trap

```cpp
struct InteractionState { bool hovered, pressed, focused, focus_visible, checked, active, disabled; };
```
`hovered/pressed/focused/focus_visible` come from the live interaction hooks
(`detail::interaction_state()` in `common.h` reads `use_hovered()`/etc.); `checked/active/disabled`
are component-supplied.

**THE TRAP (caused a critical baseline failure):** interaction paint requires **two** things, and a
plain `Box` has neither:
1. **The node must be focusable.** Hover is gated on the focus set — `focus.cpp`'s `hovered_enabled`
   iterates only `runtime.focusables` (nodes with `interaction.focusable`). `Box` defaults
   `focusable = false` (`box.hx:12`), so it never even enters the hover candidate set.
2. **The host must resolve with live `InteractionState`.**
   - `ui::components::Button` → `resolve(use_theme().button, props.style, detail::interaction_state(disabled))` ✅ live.
   - `ui::components::Box` → `resolve(use_theme().box, props.style, {})` ❌ **empty** state. A
     `.hover`/`.pressed` slot on a `style` passed to a `Box` is **dead data** — it never fires.

Consequence: **a hover-reactive node is, by construction, a focusable (keyboard/gamepad nav-stop)
node.** If you need hover/press/focus paint, root on `Button` **or** author a component whose
`.cppx` sets `focusable` and itself calls `detail::interaction_state(...)` + `resolve(...)`. A
surface that must *not* be a nav stop (e.g. a HUD status pill) therefore cannot hover — don't add a
hover slot to a `Box` and expect it to render; surface the conflict instead.

## Theme: mechanism (ui/) vs values (client/)

- **`RoleStyle { VisualStyle base; StylePatch hover, focus_visible, pressed, checked, active,
  disabled; }`** — a widget role's dense default + a sparse patch per state.
- **`Theme`** — `box, text, button, input, checkbox, checkbox_mark, dialog` roles + loose color
  tokens (`focus_ring, text_default, text_disabled, caret, selection`). One immutable POD.
- **`ThemeContext` / `use_theme()`** — `use_theme()` reads the installed `Theme*`, falling back to
  `default_theme()` (a **neutral flat-gray, no-gradient** fallback) when no provider is installed.
  This is the seam between mechanism and values.
- **`src/ui/` owns only the mechanism + the neutral fallback.** The real product palette (dark
  slate, accent blue, control gradients) is **`client::ui::app_theme()`**, installed by
  **`client::ui::ThemeProvider`** outermost so the whole tree resolves to slate. `ThemeProvider`
  hands the context a `const_cast` pointer to the function-static `app_theme()` (no per-frame copy).

Adding palette/values to `src/ui/style/default_theme.cpp` is wrong — values go in `app_theme()`.

## Layout vs paint are separate channels

Primitives take a `LayoutStyle layout` **and** a `StyleStatePatch style`. `LayoutStyle.border_width`
is a **layout-only** field (reserves the Yoga border box); the paint border lives in the resolved
`VisualStyle.border`. Don't seed paint through layout.

## Design tokens (`components/tokens.h`)

The single source of app paint (shadcn-style), in `namespace shooter::tokens` (the bare `tokens::`
shorthand resolves only from inside `namespace shooter`). Header-only `constexpr` palette + font
sizes, plus three sparse `StylePatch` builders:
```cpp
fill_patch(Color background)                              // solid fill, no border
panel_patch(Color bg, Color border, float w = kBorderWidth)  // fill + uniform border
text_patch(Color color, uint16_t font_size)              // text paint
```
Components read `tokens::kAccent*/kDanger*/kSurface*/…` and call these builders — they do not
hand-author dense palettes. A genuinely feature-local color with no other consumer may be a
file-local constant in the `.cppx` anonymous namespace (with a comment), e.g. `kSunkenBg`.

## The variant → paint chain (concretely)

1. A screen passes `variant={AppButtonVariant::Primary}` to `AppButton` (a semantic enum only).
2. `app_button.cppx` computes `layout = app_button_layout(props.size, props.selected)` and
   `style = app_button_variant_patch(variant)` (→ a `StyleStatePatch` with its own
   `base`/`hover`/`pressed` slots built from tokens) and forwards both to `ui::components::Button`.
3. `button.cppx` calls `resolve(use_theme().button, props.style, interaction_state(disabled))` and
   commits the dense `VisualStyle` as the host `.visual`.
4. The renderer reads `node.visual`.

### Two subtleties when authoring a variant patch

- **Supply every interaction slot you want branded.** `resolve()` layers the theme-role's
  `hover`/`pressed` when those states are active, so a *base-only* override reverts to the theme's
  slate on hover/press. `Primary`/`Danger`/`Ghost` each set `base`+`hover`+`pressed`; `Secondary`
  returns `{}` deliberately (== the theme's slate button).
- **A solid fill must re-emit a flat 2-stop gradient, not a bare background.** A resolved gradient
  *is* the fill, so a bare `.background()` sits invisibly behind the theme role's gradient. Use the
  `solid()` helper pattern in `app_button_variant.h` (`gradient` with `{0:fill, 1:fill}`).
