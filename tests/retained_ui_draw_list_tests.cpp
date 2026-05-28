#include "ui/components/components.h"
#include "ui/runtime/draw_list.h"
#include "ui/runtime/flex_layout.h"
#include "ui/runtime/yoga_flex_layout.h"

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
using namespace ui::components;

static bool same_text(const char *actual, const char *expected) {
  return strcmp(actual ? actual : "", expected ? expected : "") == 0;
}

static bool same_color(Color actual, Color expected) {
  return actual.r == expected.r && actual.g == expected.g &&
         actual.b == expected.b && actual.a == expected.a;
}

static const DrawCommand *find_command(const DrawList &list, NodeId id,
                                       DrawCommandKind kind, int ordinal = 0) {
  for (int i = 0; i < list.count; ++i) {
    const DrawCommand &command = list.commands[i];
    if (command.node_id == id && command.kind == kind) {
      if (ordinal == 0)
        return &command;
      --ordinal;
    }
  }
  return nullptr;
}

static bool retained_draw_list_uses_html_primitive_metadata_and_layout(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;

  UiElement root = Box(
      frame,
      {
          .key = "root",
          .style =
              {
                  .width = Length::points(320.0f),
                  .height = Length::points(260.0f),
                  .align_items = AlignItems::Start,
                  .padding = {4.0f, 4.0f, 4.0f, 4.0f},
                  .gap = 6.0f,
                  .background = {18, 27, 32, 245},
                  .border = {83, 108, 118, 255},
                  .border_width = 1.0f,
              },
          .children = frame.children({
              Text(frame,
                   {
                       .key = "title",
                       .value = "Title",
                       .style =
                           {
                               .text = {235, 246, 242, 255},
                               .font_size = 24,
                           },
                   }),
              Button(frame,
                     {
                         .key = "confirm",
                         .id = "ConfirmButton",
                         .label = "Confirm",
                     }),
              Checkbox(frame,
                       {
                           .key = "music",
                           .id = "MusicCheckbox",
                           .checked = true,
                           .label = "Music",
                       }),
              Input(frame,
                    {
                        .key = "name",
                        .id = "NameInput",
                        .value = "abc",
                    }),
          }),
      });
  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 320.0f, 260.0f);
  CHECK(result.ok);

  NodeId root_id = tree.child_at(tree.root_id(), 0);
  NodeId input = tree.child_at(root_id, 3);
  ::ui::UiKeyInputEvent select_all = {
      .key = ::ui::UiKey::A,
      .modifiers = ::ui::UI_KEY_MOD_CTRL,
  };
  CHECK(tree.invoke_key(input, select_all));

  frame.reset();
  root = Box(
      frame,
      {
          .key = "root",
          .style =
              {
                  .width = Length::points(320.0f),
                  .height = Length::points(260.0f),
                  .align_items = AlignItems::Start,
                  .padding = {4.0f, 4.0f, 4.0f, 4.0f},
                  .gap = 6.0f,
                  .background = {18, 27, 32, 245},
                  .border = {83, 108, 118, 255},
                  .border_width = 1.0f,
              },
          .children = frame.children({
              Text(frame,
                   {
                       .key = "title",
                       .value = "Title",
                       .style =
                           {
                               .text = {235, 246, 242, 255},
                               .font_size = 24,
                           },
                   }),
              Button(frame,
                     {
                         .key = "confirm",
                         .id = "ConfirmButton",
                         .label = "Confirm",
                     }),
              Checkbox(frame,
                       {
                           .key = "music",
                           .id = "MusicCheckbox",
                           .checked = true,
                           .label = "Music",
                       }),
              Input(frame,
                    {
                        .key = "name",
                        .id = "NameInput",
                        .value = "abc",
                    }),
          }),
      });
  result = reconcile_retained_tree(tree, frame, root, 320.0f, 260.0f);
  CHECK(result.ok);

  FlexLayoutAdapter adapter = make_yoga_flex_layout_adapter();
  CHECK(compute_flex_layout(adapter, tree, {320.0f, 260.0f}));

  root_id = tree.child_at(tree.root_id(), 0);
  NodeId title = tree.child_at(root_id, 0);
  NodeId button = tree.child_at(root_id, 1);
  NodeId button_label = tree.child_at(button, 0);
  NodeId checkbox = tree.child_at(root_id, 2);
  NodeId checkbox_mark = tree.child_at(checkbox, 0);
  NodeId checkbox_label = tree.child_at(checkbox, 1);
  input = tree.child_at(root_id, 3);

  DrawList list = {};
  CHECK(build_draw_list(tree, &list, input));
  CHECK(list.error_count == 0);
  CHECK(list.count == 11);

  const DrawCommand *root_rect =
      find_command(list, root_id, DrawCommandKind::Rect);
  CHECK(root_rect != nullptr);
  CHECK(same_color(root_rect->fill, {18, 27, 32, 245}));
  CHECK(same_color(root_rect->border, {83, 108, 118, 255}));

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
  CHECK(same_color(button_rect->fill, {24, 28, 36, 255}));

  const DrawCommand *button_text =
      find_command(list, button_label, DrawCommandKind::Text);
  CHECK(button_text != nullptr);
  CHECK(same_text(button_text->text, "Confirm"));

  const DrawCommand *checkbox_rect =
      find_command(list, checkbox, DrawCommandKind::Rect);
  CHECK(checkbox_rect != nullptr);
  CHECK(checkbox_rect->rect.width == 178.0f);
  CHECK(same_color(checkbox_rect->fill, {24, 28, 36, 255}));

  const DrawCommand *checkbox_mark_rect =
      find_command(list, checkbox_mark, DrawCommandKind::Rect);
  CHECK(checkbox_mark_rect != nullptr);
  CHECK(checkbox_mark_rect->rect.width == 18.0f);
  CHECK(same_color(checkbox_mark_rect->fill, {44, 92, 128, 255}));

  const DrawCommand *checkbox_text =
      find_command(list, checkbox_label, DrawCommandKind::Text);
  CHECK(checkbox_text != nullptr);
  CHECK(same_text(checkbox_text->text, "Music"));

  const DrawCommand *input_rect =
      find_command(list, input, DrawCommandKind::Rect, 0);
  const DrawCommand *selection_rect =
      find_command(list, input, DrawCommandKind::Rect, 1);
  const DrawCommand *input_text =
      find_command(list, input, DrawCommandKind::Text);
  const DrawCommand *caret_rect =
      find_command(list, input, DrawCommandKind::Rect, 2);
  CHECK(input_rect != nullptr);
  CHECK(input_rect->rect.width == 220.0f);
  CHECK(selection_rect != nullptr);
  CHECK(selection_rect->rect.width == 24.0f);
  CHECK(input_text != nullptr);
  CHECK(same_text(input_text->text, "abc"));
  CHECK(caret_rect != nullptr);
  CHECK(caret_rect->rect.width == 1.0f);
  return true;
}

int main(void) {
  if (!retained_draw_list_uses_html_primitive_metadata_and_layout())
    return 1;
  return 0;
}
