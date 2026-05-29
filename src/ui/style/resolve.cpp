#include "resolve.h"

namespace ui {

VisualStyle resolve(const RoleStyle &role, const StylePatch &variant,
                    const InteractionState &st) {
  VisualStyle vs = role.base; // start dense
  apply(vs, variant);         // variant first
  if (st.hovered)
    apply(vs, role.hover);
  if (st.focus_visible)
    apply(vs, role.focus_visible);
  if (st.pressed)
    apply(vs, role.pressed);
  if (st.checked)
    apply(vs, role.checked);
  if (st.active)
    apply(vs, role.active);
  if (st.disabled)
    apply(vs, role.disabled); // disabled wins
  return vs;
}

} // namespace ui
