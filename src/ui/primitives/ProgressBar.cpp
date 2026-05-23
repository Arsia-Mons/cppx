#include "ui/primitives/ProgressBar.h"

#include "ui/design/Theme.h"
#include "ui/primitives/Text.h"

#include <algorithm>

namespace ui {

void ProgressBar(UiFrameContext &frame, Clay_ElementId id, const std::string &label, float value, Clay_Color fill) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    CLAY({ .id = id,
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(54)},
            .childGap = 8,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        }
    }) {
        Text(frame, label, {.color = color::Text, .size = type::BodySmall, .font = font::Body});
        CLAY({ .id = CLAY_IDI("Track", id.id),
            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(12)}},
            .backgroundColor = color::SurfaceMuted,
            .cornerRadius = radius::Small
        }) {
            CLAY({ .id = CLAY_IDI("Fill", id.id),
                .layout = {.sizing = {.width = CLAY_SIZING_PERCENT(clamped), .height = CLAY_SIZING_GROW(0)}},
                .backgroundColor = fill,
                .cornerRadius = radius::Small
            }) {}
        }
    }
}

}
