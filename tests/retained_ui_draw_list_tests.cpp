#include "ui/retained/components.h"
#include "ui/retained/draw_list.h"
#include "ui/retained/flex_layout.h"
#include "ui/retained/yoga_flex_layout.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, \
                    #expr);                                                    \
            return false;                                                      \
        }                                                                      \
    } while (0)

using namespace ui::retained;

static bool same_text(const char *actual, const char *expected) {
    return strcmp(actual ? actual : "", expected ? expected : "") == 0;
}

static bool same_color(Color actual, Color expected) {
    return actual.r == expected.r && actual.g == expected.g &&
           actual.b == expected.b && actual.a == expected.a;
}

static const DrawCommand *find_command(const DrawList &list, NodeId id,
                                       DrawCommandKind kind) {
    for (int i = 0; i < list.count; ++i) {
        const DrawCommand &command = list.commands[i];
        if (command.node_id == id && command.kind == kind) {
            return &command;
        }
    }
    return nullptr;
}

static bool retained_draw_list_uses_primitive_metadata_and_layout(void) {
    react_init_runtime();
    UiTree tree;

    CHECK(begin_retained_frame(tree, 320.0f, 220.0f));
    Panel(
        {
            .key = "root",
            .width = Length::points(320.0f),
            .height = Length::points(220.0f),
            .gap = 6.0f,
            .padding = {4.0f, 4.0f, 4.0f, 4.0f},
            .align_items = AlignItems::Start,
        },
        [] {
            Button(ButtonProps{
                .key = "confirm",
                .id = "ConfirmButton",
                .label = "Confirm",
            });
            Toggle(ToggleProps{
                .key = "music",
                .id = "MusicToggle",
                .label = "Music",
                .checked = true,
            });
            Selectable(SelectableProps{
                .key = "primary",
                .id = "PrimarySlot",
                .label = "Rifle",
                .selected = true,
                .disabled = true,
            });
        });
    CHECK(end_retained_frame());

    FlexLayoutAdapter adapter = make_yoga_flex_layout_adapter();
    CHECK(compute_flex_layout(adapter, tree, {320.0f, 220.0f}));

    NodeId root = tree.child_at(tree.root_id(), 0);
    NodeId button = tree.child_at(root, 0);
    NodeId button_label = tree.child_at(button, 0);
    NodeId toggle = tree.child_at(root, 1);
    NodeId toggle_label = tree.child_at(toggle, 1);
    NodeId selectable = tree.child_at(root, 2);
    NodeId selectable_label = tree.child_at(selectable, 0);

    DrawList list = {};
    CHECK(build_draw_list(tree, &list));
    CHECK(list.error_count == 0);
    CHECK(list.count == 6);

    const DrawCommand *button_rect =
        find_command(list, button, DrawCommandKind::Rect);
    CHECK(button_rect != nullptr);
    CHECK(button_rect->rect.width == 132.0f);
    CHECK(button_rect->rect.height == 38.0f);
    CHECK(same_color(button_rect->fill, {24, 28, 36, 255}));

    const DrawCommand *button_text =
        find_command(list, button_label, DrawCommandKind::Text);
    CHECK(button_text != nullptr);
    CHECK(same_text(button_text->text, "Confirm"));
    CHECK(button_text->rect.width == 56.0f);
    CHECK(same_color(button_text->fill, {226, 234, 242, 255}));

    const DrawCommand *toggle_rect =
        find_command(list, toggle, DrawCommandKind::Rect);
    CHECK(toggle_rect != nullptr);
    CHECK(toggle_rect->rect.width == 178.0f);
    CHECK(same_color(toggle_rect->fill, {44, 92, 128, 255}));

    const DrawCommand *toggle_text =
        find_command(list, toggle_label, DrawCommandKind::Text);
    CHECK(toggle_text != nullptr);
    CHECK(same_text(toggle_text->text, "Music"));
    CHECK(same_color(toggle_text->fill, {226, 234, 242, 255}));

    const DrawCommand *selectable_rect =
        find_command(list, selectable, DrawCommandKind::Rect);
    CHECK(selectable_rect != nullptr);
    CHECK(selectable_rect->rect.width == 132.0f);
    CHECK(selectable_rect->rect.height == 34.0f);
    CHECK(same_color(selectable_rect->fill, {30, 34, 42, 255}));

    const DrawCommand *selectable_text =
        find_command(list, selectable_label, DrawCommandKind::Text);
    CHECK(selectable_text != nullptr);
    CHECK(same_text(selectable_text->text, "Rifle"));
    CHECK(same_color(selectable_text->fill, {126, 134, 148, 255}));
    return true;
}

int main(void) {
    if (!retained_draw_list_uses_primitive_metadata_and_layout())
        return 1;
    return 0;
}
