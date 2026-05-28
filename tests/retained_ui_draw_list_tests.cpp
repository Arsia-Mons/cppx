#include "ui/retained/draw_list.h"
#include "ui/retained/element_components.h"
#include "ui/retained/flex_layout.h"
#include "ui/retained/yoga_flex_layout.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr)                                                            \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__,       \
              #expr);                                                          \
      return false;                                                            \
    }                                                                          \
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
  UiElementFrame frame;

  UiElement root = BoxElement(
      frame,
      {
          .key = "root",
          .width = Length::points(320.0f),
          .height = Length::points(220.0f),
          .align_items = AlignItems::Start,
          .padding = {4.0f, 4.0f, 4.0f, 4.0f},
          .gap = 6.0f,
          .background = {18, 27, 32, 245},
          .border = {83, 108, 118, 255},
          .border_width = 1.0f,
          .children = frame.children({
              TextElement(frame,
                          {
                              .key = "title",
                              .value = "Title",
                              .text_color = {235, 246, 242, 255},
                              .font_size = 24,
                          }),
              ButtonElement(frame,
                            {
                                .key = "confirm",
                                .id = "ConfirmButton",
                                .label = "Confirm",
                            }),
              ToggleElement(frame,
                            {
                                .key = "music",
                                .id = "MusicToggle",
                                .label = "Music",
                                .checked = true,
                            }),
              SelectableElement(frame,
                                {
                                    .key = "primary",
                                    .id = "PrimarySlot",
                                    .label = "Rifle",
                                    .selected = true,
                                    .disabled = true,
                                }),
          }),
      });
  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 320.0f, 220.0f);
  CHECK(result.ok);

  FlexLayoutAdapter adapter = make_yoga_flex_layout_adapter();
  CHECK(compute_flex_layout(adapter, tree, {320.0f, 220.0f}));

  NodeId root_id = tree.child_at(tree.root_id(), 0);
  NodeId title = tree.child_at(root_id, 0);
  NodeId button = tree.child_at(root_id, 1);
  NodeId button_label = tree.child_at(button, 0);
  NodeId toggle = tree.child_at(root_id, 2);
  NodeId toggle_mark = tree.child_at(toggle, 0);
  NodeId toggle_label = tree.child_at(toggle, 1);
  NodeId selectable = tree.child_at(root_id, 3);
  NodeId selectable_label = tree.child_at(selectable, 0);

  DrawList list = {};
  CHECK(build_draw_list(tree, &list));
  CHECK(list.error_count == 0);
  CHECK(list.count == 9);

  const DrawCommand *root_rect =
      find_command(list, root_id, DrawCommandKind::Rect);
  CHECK(root_rect != nullptr);
  CHECK(same_color(root_rect->fill, {18, 27, 32, 245}));
  CHECK(same_color(root_rect->border, {83, 108, 118, 255}));
  CHECK(root_rect->border_width == 1.0f);

  const DrawCommand *title_text =
      find_command(list, title, DrawCommandKind::Text);
  CHECK(title_text != nullptr);
  CHECK(same_text(title_text->text, "Title"));
  CHECK(same_color(title_text->fill, {235, 246, 242, 255}));
  CHECK(title_text->font_size == 24);

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
  CHECK(same_color(toggle_rect->fill, {24, 28, 36, 255}));

  const DrawCommand *toggle_mark_rect =
      find_command(list, toggle_mark, DrawCommandKind::Rect);
  CHECK(toggle_mark_rect != nullptr);
  CHECK(same_color(toggle_mark_rect->fill, {44, 92, 128, 255}));

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
  CHECK(same_color(selectable_rect->fill, {42, 80, 60, 255}));

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
