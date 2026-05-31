#include "ui/components/components.h"
#include "ui/runtime/flex_layout.h"
#include "ui/runtime/yoga_flex_layout.h"

#include <stdio.h>
#include <string.h>
#include <string>

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

static bool same_text(const char *actual, const char *expected) {
  return strcmp(actual ? actual : "", expected ? expected : "") == 0;
}

static bool snapshot_node(UiTree &tree, NodeId id, NodeSnapshot *snapshot) {
  CHECK(id != 0);
  CHECK(tree.snapshot(id, snapshot));
  return true;
}

static bool commit_root(UiTree &tree, UiElementFrame &frame,
                        const UiElement &root, float width, float height) {
  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, width, height);
  CHECK(result.ok);
  return true;
}

static bool html_primitives_write_semantic_metadata_and_layout(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  UiElementFrameScope frame_scope(frame);

  UiElement root = Box({
      .key = "root",
      .layout =
          {
              .align_items = AlignItems::Start,
              .width = Length::points(360.0f),
              .height = Length::points(260.0f),
              .padding = {4.0f, 4.0f, 4.0f, 4.0f},
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
          Input({
              .key = "name",
              .id = "NameInput",
              .value = "Ace",
          }),
          Box({
              .key = "card",
              .id = "ComposedCard",
              .focusable = true,
              .layout =
                  {
                      .width = Length::points(120.0f),
                      .height = Length::points(44.0f),
                  },
              .style = ::ui::patch().background({20, 28, 32, 255}),
              .children = ::ui::children({
                  Text({
                      .key = "label",
                      .value = "Card",
                  }),
              }),
          }),
      }),
  });
  CHECK(commit_root(tree, frame, root, 360.0f, 260.0f));

  FlexLayoutAdapter adapter = make_yoga_flex_layout_adapter();
  CHECK(compute_flex_layout(adapter, tree, {360.0f, 260.0f}));

  NodeSnapshot root_snapshot = {};
  CHECK(snapshot_node(tree, tree.child_at(tree.root_id(), 0), &root_snapshot));
  CHECK(root_snapshot.role == NodeRole::Box);
  CHECK(root_snapshot.child_count == 4);
  CHECK(root_snapshot.layout.width == 360.0f);
  CHECK(root_snapshot.layout.height == 260.0f);

  NodeSnapshot button = {};
  CHECK(snapshot_node(tree, tree.child_at(root_snapshot.id, 0), &button));
  CHECK(button.role == NodeRole::Button);
  CHECK(button.semantic_role == SemanticRole::Button);
  CHECK(same_text(button.control_id, "ConfirmButton"));
  CHECK(same_text(button.value, "Confirm"));
  CHECK(button.interaction.focusable);
  CHECK(button.layout.width == 132.0f);
  CHECK(button.layout.height == 38.0f);

  NodeSnapshot button_label = {};
  CHECK(snapshot_node(tree, tree.child_at(button.id, 0), &button_label));
  CHECK(button_label.role == NodeRole::Text);
  CHECK(same_text(button_label.value, "Confirm"));
  CHECK(button_label.layout.width == 56.0f);

  NodeSnapshot checkbox = {};
  CHECK(snapshot_node(tree, tree.child_at(root_snapshot.id, 1), &checkbox));
  CHECK(checkbox.role == NodeRole::Checkbox);
  CHECK(checkbox.semantic_role == SemanticRole::Checkbox);
  CHECK(same_text(checkbox.control_id, "MusicCheckbox"));
  CHECK(checkbox.interaction.focusable);
  CHECK(checkbox.interaction.checked);
  CHECK(checkbox.child_count == 2);
  CHECK(checkbox.layout.width == 178.0f);

  NodeSnapshot input = {};
  CHECK(snapshot_node(tree, tree.child_at(root_snapshot.id, 2), &input));
  CHECK(input.role == NodeRole::Input);
  CHECK(input.semantic_role == SemanticRole::TextBox);
  CHECK(same_text(input.control_id, "NameInput"));
  CHECK(same_text(input.value, "Ace"));
  CHECK(input.text_edit.caret == 3);
  CHECK(input.layout.width == 220.0f);

  NodeSnapshot card = {};
  CHECK(snapshot_node(tree, tree.child_at(root_snapshot.id, 3), &card));
  CHECK(card.role == NodeRole::Box);
  CHECK(same_text(card.control_id, "ComposedCard"));
  CHECK(card.interaction.focusable);
  CHECK(card.layout.width == 120.0f);
  CHECK(card.child_count == 1);
  return true;
}

static bool reused_nodes_clear_previous_control_metadata(void) {
  react_init_runtime();
  UiTree tree;
  int activate_count = 0;

  {
    UiElementFrame frame;
    UiElementFrameScope frame_scope(frame);
    UiElement root = Button({
        .key = "confirm",
        .id = "ConfirmButton",
        .disabled = true,
        .label = "Confirm",
        .on_activate =
            [&activate_count](const ActivationEvent &) { activate_count += 1; },
    });
    CHECK(commit_root(tree, frame, root, 200.0f, 80.0f));
  }

  NodeId button_id = tree.child_at(tree.root_id(), 0);
  NodeSnapshot first = {};
  CHECK(snapshot_node(tree, button_id, &first));
  CHECK(first.interaction.disabled);
  CHECK(same_text(first.value, "Confirm"));
  CHECK(!tree.invoke_activate(button_id));
  CHECK(activate_count == 0);

  {
    UiElementFrame frame;
    UiElementFrameScope frame_scope(frame);
    UiElement root = Button({
        .key = "confirm",
        .id = "ConfirmButton",
    });
    CHECK(commit_root(tree, frame, root, 200.0f, 80.0f));
  }

  NodeSnapshot second = {};
  CHECK(snapshot_node(tree, button_id, &second));
  CHECK(!second.interaction.disabled);
  CHECK(same_text(second.value, ""));
  CHECK(second.child_count == 0);
  CHECK(!tree.invoke_activate(button_id));
  CHECK(activate_count == 0);
  return true;
}

static bool checkbox_invokes_change_callback(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  UiElementFrameScope frame_scope(frame);
  int observed = -1;

  UiElement root = Checkbox({
      .key = "music",
      .id = "MusicCheckbox",
      .checked = true,
      .label = "Music",
      .on_change = [&observed](bool checked) { observed = checked ? 1 : 0; },
  });
  CHECK(commit_root(tree, frame, root, 220.0f, 80.0f));

  NodeId checkbox_id = tree.child_at(tree.root_id(), 0);
  CHECK(tree.invoke_activate(checkbox_id));
  CHECK(observed == 0);
  return true;
}

static bool input_handles_typing_caret_delete_selection_and_composition(void) {
  react_init_runtime();
  UiTree tree;
  std::string value = "abc";

  {
    UiElementFrame frame;
    UiElementFrameScope frame_scope(frame);
    UiElement root = Input({
        .key = "name",
        .id = "NameInput",
        .value = value.c_str(),
        .on_change = [&value](const std::string &next) { value = next; },
    });
    CHECK(commit_root(tree, frame, root, 240.0f, 80.0f));
  }

  NodeId input_id = tree.child_at(tree.root_id(), 0);
  ::ui::UiKeyInputEvent left = {.key = ::ui::UiKey::Left};
  CHECK(tree.invoke_key(input_id, left));
  ::ui::UiTextInputEvent text = {};
  strcpy(text.text, "Z");
  CHECK(tree.invoke_text_input(input_id, text));
  CHECK(value == "abZc");

  {
    UiElementFrame frame;
    UiElementFrameScope frame_scope(frame);
    UiElement root = Input({
        .key = "name",
        .id = "NameInput",
        .value = value.c_str(),
        .on_change = [&value](const std::string &next) { value = next; },
    });
    CHECK(commit_root(tree, frame, root, 240.0f, 80.0f));
  }

  ::ui::UiKeyInputEvent select_all = {
      .key = ::ui::UiKey::A,
      .modifiers = ::ui::UI_KEY_MOD_CTRL,
  };
  CHECK(tree.invoke_key(input_id, select_all));
  strcpy(text.text, "x");
  CHECK(tree.invoke_text_input(input_id, text));
  CHECK(value == "x");

  ::ui::UiTextEditingEvent composition = {};
  strcpy(composition.text, "compose");
  composition.start = 1;
  composition.length = 3;
  CHECK(tree.invoke_text_editing(input_id, composition));

  {
    UiElementFrame frame;
    UiElementFrameScope frame_scope(frame);
    UiElement root = Input({
        .key = "name",
        .id = "NameInput",
        .value = value.c_str(),
        .on_change = [&value](const std::string &next) { value = next; },
    });
    CHECK(commit_root(tree, frame, root, 240.0f, 80.0f));
  }

  NodeSnapshot input = {};
  CHECK(snapshot_node(tree, input_id, &input));
  CHECK(same_text(input.text_edit.composition, "compose"));
  CHECK(input.text_edit.composition_start == 1);
  CHECK(input.text_edit.composition_length == 3);

  {
    UiElementFrame frame;
    UiElementFrameScope frame_scope(frame);
    UiElement root = Input({
        .key = "disabled",
        .disabled = true,
        .value = value.c_str(),
        .on_change = [&value](const std::string &next) { value = next; },
    });
    CHECK(commit_root(tree, frame, root, 240.0f, 80.0f));
  }
  NodeId disabled_input = tree.child_at(tree.root_id(), 0);
  strcpy(text.text, "!");
  CHECK(!tree.invoke_text_input(disabled_input, text));
  CHECK(value == "x");
  return true;
}

int main(void) {
  if (!html_primitives_write_semantic_metadata_and_layout())
    return 1;
  if (!reused_nodes_clear_previous_control_metadata())
    return 1;
  if (!checkbox_invokes_change_callback())
    return 1;
  if (!input_handles_typing_caret_delete_selection_and_composition())
    return 1;
  return 0;
}
