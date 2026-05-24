#pragma once

#include <clay.h>

struct PanelProps {
    Clay_ElementId       id;
    Clay_Sizing          sizing;
    Clay_Padding         padding;
    uint16_t             child_gap;
    Clay_ChildAlignment  child_alignment;
    Clay_LayoutDirection direction;
    Clay_Color           background;
    Clay_CornerRadius    radius;
};

template <typename Children>
static inline void Panel(const PanelProps &props, Children children) {
    CLAY({
        .id = props.id,
        .layout = {
            .sizing = props.sizing,
            .padding = props.padding,
            .childGap = props.child_gap,
            .childAlignment = props.child_alignment,
            .layoutDirection = props.direction,
        },
        .backgroundColor = props.background,
        .cornerRadius = props.radius,
    }) {
        children();
    }
}
