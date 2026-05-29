#pragma once

// resolve(): the single function that turns a RoleStyle + a component's chosen
// variant patch + the component's current InteractionState into one dense
// VisualStyle, run INSIDE the component at authoring time. Fixed precedence
// (low->high): base -> variant -> hover -> focus_visible -> pressed -> checked
// -> active -> disabled. See design §5.

#include "interaction.h"
#include "style_patch.h"
#include "theme.h"
#include "visual_style.h"

namespace ui {

VisualStyle resolve(const RoleStyle &role, const StylePatch &variant,
                    const InteractionState &st);

} // namespace ui
