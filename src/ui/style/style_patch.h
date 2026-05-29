#pragma once

// StylePatch: the ONE sparse authoring overlay over VisualStyle.
// Presence is ALWAYS Opt<T>::set — never a value-space sentinel. See design §3.
// apply(): overlay a patch onto a dense VisualStyle (last-write-wins layering).
// merge(): overlay a patch onto another patch (src wins, mutates dst).

#include "visual_style.h"

#include <type_traits>

namespace ui {

// The single optionality wrapper for authoring. Trivially-copyable aggregate.
template <class T> struct Opt {
  bool set = false;
  T value{};
};

template <class T> constexpr Opt<T> opt(T v) { return {true, v}; }

// Sparse overlay: one Opt<T> per VisualStyle field. Compound members are wrapped
// whole (a patch supplies a complete Border/Outline/Gradient/... not sub-fields).
struct StylePatch {
  Opt<Color> background;
  Opt<float> corner_radius;
  Opt<Border> border;
  Opt<Outline> outline;
  Opt<Gradient> gradient;
  Opt<BackgroundImage> image;
  Opt<Shadow> shadow;
  Opt<float> opacity;
  Opt<bool> hidden;
  Opt<TextVisual> text;
};

static_assert(std::is_aggregate_v<StylePatch>);
static_assert(std::is_trivially_copyable_v<StylePatch>);

// Write each set field of `p` onto `dst`. Presence is Opt::set only.
constexpr void apply(VisualStyle &dst, const StylePatch &p) {
  if (p.background.set)
    dst.background = p.background.value;
  if (p.corner_radius.set)
    dst.corner_radius = p.corner_radius.value;
  if (p.border.set)
    dst.border = p.border.value;
  if (p.outline.set)
    dst.outline = p.outline.value;
  if (p.gradient.set)
    dst.gradient = p.gradient.value;
  if (p.image.set)
    dst.image = p.image.value;
  if (p.shadow.set)
    dst.shadow = p.shadow.value;
  if (p.opacity.set)
    dst.opacity = p.opacity.value;
  if (p.hidden.set)
    dst.hidden = p.hidden.value;
  if (p.text.set)
    dst.text = p.text.value;
}

// Overlay src's set fields onto dst. src wins; dst is mutated in place.
constexpr void merge(StylePatch &dst, const StylePatch &src) {
  if (src.background.set)
    dst.background = src.background;
  if (src.corner_radius.set)
    dst.corner_radius = src.corner_radius;
  if (src.border.set)
    dst.border = src.border;
  if (src.outline.set)
    dst.outline = src.outline;
  if (src.gradient.set)
    dst.gradient = src.gradient;
  if (src.image.set)
    dst.image = src.image;
  if (src.shadow.set)
    dst.shadow = src.shadow;
  if (src.opacity.set)
    dst.opacity = src.opacity;
  if (src.hidden.set)
    dst.hidden = src.hidden;
  if (src.text.set)
    dst.text = src.text;
}

// Fluent builder so the set-flag is impossible to forget at authoring sites.
struct StylePatchBuilder {
  StylePatch p{};
  StylePatchBuilder &background(Color c) {
    p.background = opt(c);
    return *this;
  }
  StylePatchBuilder &corner_radius(float r) {
    p.corner_radius = opt(r);
    return *this;
  }
  StylePatchBuilder &border(Border b) {
    p.border = opt(b);
    return *this;
  }
  StylePatchBuilder &outline(Outline o) {
    p.outline = opt(o);
    return *this;
  }
  StylePatchBuilder &gradient(Gradient g) {
    p.gradient = opt(g);
    return *this;
  }
  StylePatchBuilder &image(BackgroundImage i) {
    p.image = opt(i);
    return *this;
  }
  StylePatchBuilder &shadow(Shadow s) {
    p.shadow = opt(s);
    return *this;
  }
  StylePatchBuilder &opacity(float o) {
    p.opacity = opt(o);
    return *this;
  }
  StylePatchBuilder &hidden(bool h) {
    p.hidden = opt(h);
    return *this;
  }
  StylePatchBuilder &text(TextVisual t) {
    p.text = opt(t);
    return *this;
  }
  operator StylePatch() const { return p; }
};

inline StylePatchBuilder patch() { return {}; }

} // namespace ui
