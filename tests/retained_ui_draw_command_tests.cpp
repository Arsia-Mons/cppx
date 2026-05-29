#include "ui/components/components.h"
#include "ui/runtime/draw_command_builder.h"
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

using namespace ui;
using namespace ui::components::elements;

static bool same_color(Color actual, Color expected) {
  return actual.r == expected.r && actual.g == expected.g &&
         actual.b == expected.b && actual.a == expected.a;
}

// Straight-alpha -> premultiplied, matching the transcriber's emit convention.
static Color premul(Color c) {
  return {
      static_cast<uint8_t>(static_cast<int>(c.r) * c.a / 255),
      static_cast<uint8_t>(static_cast<int>(c.g) * c.a / 255),
      static_cast<uint8_t>(static_cast<int>(c.b) * c.a / 255),
      c.a,
  };
}

static const DrawCommand *find_command(const DrawCommandList &list, NodeId id,
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

static bool arena_text_matches(const DrawCommandList &list,
                               const TextData &text, const char *expected) {
  size_t expected_len = strlen(expected);
  if (text.text_len != expected_len)
    return false;
  if (text.text_off + text.text_len > static_cast<uint32_t>(list.text_len_used))
    return false;
  return memcmp(list.text_arena + text.text_off, expected, expected_len) == 0;
}

static bool button_emits_fill_and_border(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  UiElementFrameScope frame_scope(frame);

  UiElement root = Box({
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
      .children = ::ui::children({
          Text({
              .key = "title",
              .value = "Title",
              .style =
                  {
                      .text = {235, 246, 242, 255},
                      .font_size = 24,
                  },
          }),
          Button({
              .key = "confirm",
              .id = "ConfirmButton",
              .label = "Confirm",
          }),
          Checkbox({
              .key = "music",
              .id = "MusicCheckbox",
              .checked = true,
              .label = "Music",
          }),
          Input({
              .key = "name",
              .id = "NameInput",
              .value = "abc",
          }),
      }),
  });
  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 320.0f, 260.0f);
  CHECK(result.ok);

  NodeId root_id0 = tree.child_at(tree.root_id(), 0);
  NodeId input0 = tree.child_at(root_id0, 3);
  ::ui::UiKeyInputEvent select_all = {
      .key = ::ui::UiKey::A,
      .modifiers = ::ui::UI_KEY_MOD_CTRL,
  };
  CHECK(tree.invoke_key(input0, select_all));

  frame.reset();
  root = Box({
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
      .children = ::ui::children({
          Text({
              .key = "title",
              .value = "Title",
              .style =
                  {
                      .text = {235, 246, 242, 255},
                      .font_size = 24,
                  },
          }),
          Button({
              .key = "confirm",
              .id = "ConfirmButton",
              .label = "Confirm",
          }),
          Checkbox({
              .key = "music",
              .id = "MusicCheckbox",
              .checked = true,
              .label = "Music",
          }),
          Input({
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

  NodeId root_id = tree.child_at(tree.root_id(), 0);
  NodeId title = tree.child_at(root_id, 0);
  NodeId button = tree.child_at(root_id, 1);
  NodeId button_label = tree.child_at(button, 0);
  NodeId checkbox = tree.child_at(root_id, 2);
  NodeId checkbox_mark = tree.child_at(checkbox, 0);
  NodeId checkbox_label = tree.child_at(checkbox, 1);
  NodeId input = tree.child_at(root_id, 3);

  DrawCommandList list = {};
  CHECK(build_draw_command_list(tree, &list, input));
  CHECK(list.error_count == 0);

  // Root box: a styled fill + a styled border (border_width=1 single-color).
  const DrawCommand *root_rect =
      find_command(list, root_id, DrawCommandKind::Rect);
  CHECK(root_rect != nullptr);
  CHECK(same_color(root_rect->payload.rect.fill, premul({18, 27, 32, 245})));
  const DrawCommand *root_border =
      find_command(list, root_id, DrawCommandKind::Border);
  CHECK(root_border != nullptr);
  CHECK(same_color(root_border->payload.border.border.color.top,
                   premul({83, 108, 118, 255})));
  CHECK(root_border->payload.border.border.width.top == 1.0f);
  CHECK(root_border->payload.border.has_outline == false);

  // Title text: premultiplied authored color, font_size honored, arena bytes.
  const DrawCommand *title_text =
      find_command(list, title, DrawCommandKind::Text);
  CHECK(title_text != nullptr);
  CHECK(arena_text_matches(list, title_text->payload.text, "Title"));
  CHECK(same_color(title_text->payload.text.color, premul({235, 246, 242, 255})));
  CHECK(title_text->payload.text.font_size == 24);

  // Button: the new theme paints controls with a subtle vertical gradient, so
  // the fill is a Gradient command (not a flat Rect) carrying the 8px radius and
  // the top/bottom slate stops (premultiplied into grad_arena). The button is
  // NOT focused (the input is), so its border is the default control border at
  // 1px and no outline.
  const DrawCommand *button_grad =
      find_command(list, button, DrawCommandKind::Gradient);
  CHECK(button_grad != nullptr);
  CHECK(button_grad->rect.w == 132.0f);
  CHECK(button_grad->payload.gradient.corner_radius == 8.0f);
  CHECK(button_grad->payload.gradient.stop_count == 2);
  {
    uint16_t off = button_grad->payload.gradient.stop_off;
    CHECK(same_color(list.grad_arena[off].color, premul({40, 46, 58, 255})));
    CHECK(same_color(list.grad_arena[off + 1].color, premul({28, 33, 43, 255})));
  }
  const DrawCommand *button_border =
      find_command(list, button, DrawCommandKind::Border);
  CHECK(button_border != nullptr);
  CHECK(same_color(button_border->payload.border.border.color.top,
                   premul({70, 80, 98, 255})));
  CHECK(button_border->payload.border.border.width.top == 1.0f);
  CHECK(button_border->payload.border.corner_radius == 8.0f);
  CHECK(button_border->payload.border.has_outline == false);

  const DrawCommand *button_text =
      find_command(list, button_label, DrawCommandKind::Text);
  CHECK(button_text != nullptr);
  CHECK(arena_text_matches(list, button_text->payload.text, "Confirm"));

  // Checked checkbox mark uses the accent checked fill (solid, no gradient).
  const DrawCommand *checkbox_mark_rect =
      find_command(list, checkbox_mark, DrawCommandKind::Rect);
  CHECK(checkbox_mark_rect != nullptr);
  CHECK(checkbox_mark_rect->rect.w == 18.0f);
  CHECK(same_color(checkbox_mark_rect->payload.rect.fill,
                   premul({96, 165, 250, 255})));
  CHECK(checkbox_mark_rect->payload.rect.corner_radius == 4.0f);

  const DrawCommand *checkbox_text =
      find_command(list, checkbox_label, DrawCommandKind::Text);
  CHECK(checkbox_text != nullptr);
  CHECK(arena_text_matches(list, checkbox_text->payload.text, "Music"));

  // Focused input: a gradient body fill + selection rect (ranged) + value text
  // + caret rect. The Input is migrated, so its body fill is the resolved theme
  // control gradient (the legacy kInputFill fallback only applies to not-yet-
  // migrated nodes via control_fill()). The body fill being a Gradient means the
  // only Rect commands on the input node are the selection (ordinal 0) and the
  // caret (ordinal 1).
  const DrawCommand *input_grad =
      find_command(list, input, DrawCommandKind::Gradient);
  const DrawCommand *selection_rect =
      find_command(list, input, DrawCommandKind::Rect, 0);
  const DrawCommand *input_text =
      find_command(list, input, DrawCommandKind::Text);
  const DrawCommand *caret_rect =
      find_command(list, input, DrawCommandKind::Rect, 1);
  CHECK(input_grad != nullptr);
  CHECK(input_grad->rect.w == 220.0f);
  CHECK(input_grad->payload.gradient.corner_radius == 8.0f);
  CHECK(input_grad->payload.gradient.stop_count == 2);
  CHECK(selection_rect != nullptr);
  CHECK(selection_rect->rect.w == 24.0f);
  // Selection fill is half-ish alpha (180): premultiplied rgb must be scaled.
  CHECK(same_color(selection_rect->payload.rect.fill,
                   premul({72, 116, 164, 180})));
  CHECK(selection_rect->payload.rect.fill.a == 180);
  CHECK(selection_rect->payload.rect.fill.r ==
        static_cast<uint8_t>(72 * 180 / 255));
  CHECK(input_text != nullptr);
  CHECK(arena_text_matches(list, input_text->payload.text, "abc"));
  CHECK(caret_rect != nullptr);
  CHECK(caret_rect->rect.w == 1.0f);

  // The focused input must carry an outline (focus ring) on its Border command.
  const DrawCommand *input_border =
      find_command(list, input, DrawCommandKind::Border);
  CHECK(input_border != nullptr);
  CHECK(input_border->payload.border.has_outline == true);
  CHECK(same_color(input_border->payload.border.outline.color,
                   premul({96, 165, 250, 255})));
  CHECK(input_border->payload.border.outline.width == 2.0f);
  CHECK(input_border->payload.border.outline.offset == 2.0f);
  return true;
}

static bool focused_button_emits_focus_ring_outline(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  UiElementFrameScope frame_scope(frame);

  UiElement root = Box({
      .key = "root",
      .style =
          {
              .width = Length::points(320.0f),
              .height = Length::points(260.0f),
              .align_items = AlignItems::Start,
              .gap = 6.0f,
          },
      .children = ::ui::children({
          Button({
              .key = "confirm",
              .id = "ConfirmButton",
              .label = "Confirm",
          }),
          Checkbox({
              .key = "music",
              .id = "MusicCheckbox",
              .checked = true,
              .label = "Music",
          }),
      }),
  });
  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 320.0f, 260.0f);
  CHECK(result.ok);

  FlexLayoutAdapter adapter = make_yoga_flex_layout_adapter();
  CHECK(compute_flex_layout(adapter, tree, {320.0f, 260.0f}));

  NodeId root_id = tree.child_at(tree.root_id(), 0);
  NodeId button = tree.child_at(root_id, 0);
  NodeId checkbox = tree.child_at(root_id, 1);

  DrawCommandList list = {};
  CHECK(build_draw_command_list(tree, &list, button));
  CHECK(list.error_count == 0);

  // The focused control gets a Border with an outline (the focus ring) in the
  // accent color, premultiplied.
  const DrawCommand *button_border =
      find_command(list, button, DrawCommandKind::Border);
  CHECK(button_border != nullptr);
  CHECK(button_border->payload.border.has_outline == true);
  CHECK(same_color(button_border->payload.border.outline.color,
                   premul({96, 165, 250, 255})));
  CHECK(button_border->payload.border.outline.width == 2.0f);

  // Unfocused control: a Border with no outline, carrying the 8px radius and the
  // refined control border color.
  const DrawCommand *checkbox_border =
      find_command(list, checkbox, DrawCommandKind::Border);
  CHECK(checkbox_border != nullptr);
  CHECK(checkbox_border->payload.border.has_outline == false);
  CHECK(same_color(checkbox_border->payload.border.border.color.top,
                   premul({70, 80, 98, 255})));
  CHECK(checkbox_border->payload.border.border.width.top == 1.0f);
  CHECK(checkbox_border->payload.border.corner_radius == 8.0f);
  return true;
}

static bool default_text_color_is_premultiplied(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  UiElementFrameScope frame_scope(frame);

  // A bare Text node with no style: the transcriber falls back to the default
  // text fill (226,234,242,255). At full alpha premultiply is a no-op, so to
  // genuinely exercise scaling we assert on the half-alpha selection elsewhere
  // and here on the opaque default identity.
  UiElement root = Box({
      .key = "root",
      .style =
          {
              .width = Length::points(200.0f),
              .height = Length::points(80.0f),
          },
      .children = ::ui::children({
          Text({
              .key = "label",
              .value = "Hi",
          }),
      }),
  });
  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 200.0f, 80.0f);
  CHECK(result.ok);

  FlexLayoutAdapter adapter = make_yoga_flex_layout_adapter();
  CHECK(compute_flex_layout(adapter, tree, {200.0f, 80.0f}));

  NodeId root_id = tree.child_at(tree.root_id(), 0);
  NodeId label = tree.child_at(root_id, 0);

  DrawCommandList list = {};
  CHECK(build_draw_command_list(tree, &list, 0));
  CHECK(list.error_count == 0);

  const DrawCommand *label_text =
      find_command(list, label, DrawCommandKind::Text);
  CHECK(label_text != nullptr);
  CHECK(arena_text_matches(list, label_text->payload.text, "Hi"));
  CHECK(same_color(label_text->payload.text.color, premul({226, 234, 242, 255})));
  CHECK(label_text->payload.text.font_size == 15);
  return true;
}

int main(void) {
  if (!button_emits_fill_and_border())
    return 1;
  if (!focused_button_emits_focus_ring_outline())
    return 1;
  if (!default_text_color_is_premultiplied())
    return 1;
  return 0;
}
