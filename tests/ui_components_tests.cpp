#include "react.h"
#include "ui/components/components.h"
#include "ui/runtime/element.h"
#include "ui/runtime/tree.h"

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

using namespace ui::retained;
using namespace ui::components;

static bool snapshot(UiTree &tree, NodeId id, NodeSnapshot *out) {
  CHECK(tree.snapshot(id, out));
  return true;
}

static bool button_element_returns_button_host(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  int activates = 0;

  UiElement root = Button(
      frame, {
                 .key = "start",
                 .id = "StartButton",
                 .id_offset = 3,
                 .autofocus = true,
                 .label = "Start",
                 .on_activate =
                     [&activates](const ActivationEvent &) { ++activates; },
             });

  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 640.0f, 480.0f);
  CHECK(result.ok);

  NodeId button = tree.child_at(tree.root_id(), 0);
  NodeSnapshot button_snapshot = {};
  CHECK(snapshot(tree, button, &button_snapshot));
  CHECK(strcmp(button_snapshot.type, "Button") == 0);
  CHECK(strcmp(button_snapshot.key, "start") == 0);
  CHECK(button_snapshot.role == NodeRole::Button);
  CHECK(button_snapshot.semantic_role == SemanticRole::Button);
  CHECK(button_snapshot.interaction.focusable);
  CHECK(button_snapshot.interaction.initial_focus);
  CHECK(strcmp(button_snapshot.control_id, "StartButton") == 0);
  CHECK(button_snapshot.control_offset == 3);
  CHECK(strcmp(button_snapshot.value, "Start") == 0);
  CHECK(button_snapshot.child_count == 1);

  NodeSnapshot label = {};
  CHECK(snapshot(tree, tree.child_at(button, 0), &label));
  CHECK(strcmp(label.type, "Text") == 0);
  CHECK(strcmp(label.value, "Start") == 0);

  CHECK(tree.invoke_activate(button));
  CHECK(activates == 1);

  react_shutdown();
  return true;
}

static bool box_text_and_dialog_elements_return_host_nodes(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  int box_activates = 0;

  UiElement root = Dialog(
      frame,
      {
          .key = "dialog",
          .style =
              {
                  .width = Length::points(200.0f),
                  .height = Length::points(80.0f),
              },
          .children = frame.children({
              Box(frame,
                  {
                      .key = "box-action",
                      .id = "BoxAction",
                      .focusable = true,
                      .on_activate =
                          [&box_activates](const ActivationEvent &) {
                            ++box_activates;
                          },
                      .children = frame.children({
                          Text(frame,
                               {
                                   .key = "title",
                                   .value = "Title",
                               }),
                      }),
                  }),
          }),
      });

  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 640.0f, 480.0f);
  CHECK(result.ok);

  NodeId dialog = tree.child_at(tree.root_id(), 0);
  NodeSnapshot dialog_snapshot = {};
  CHECK(snapshot(tree, dialog, &dialog_snapshot));
  CHECK(strcmp(dialog_snapshot.type, "Dialog") == 0);
  CHECK(dialog_snapshot.role == NodeRole::Dialog);
  CHECK(dialog_snapshot.semantic_role == SemanticRole::Dialog);
  CHECK(dialog_snapshot.interaction.modal);
  CHECK(dialog_snapshot.style.width.value == 200.0f);

  NodeId box = tree.child_at(dialog, 0);
  NodeSnapshot box_snapshot = {};
  CHECK(snapshot(tree, box, &box_snapshot));
  CHECK(strcmp(box_snapshot.type, "Box") == 0);
  CHECK(box_snapshot.role == NodeRole::Box);
  CHECK(box_snapshot.interaction.focusable);
  CHECK(tree.invoke_activate(box));
  CHECK(box_activates == 1);

  NodeSnapshot title = {};
  CHECK(snapshot(tree, tree.child_at(box, 0), &title));
  CHECK(strcmp(title.type, "Text") == 0);
  CHECK(title.role == NodeRole::Text);
  CHECK(strcmp(title.value, "Title") == 0);
  CHECK(title.has_measure);

  react_shutdown();
  return true;
}

static bool checkbox_element_dispatches_changed_value(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  bool observed = false;

  UiElement root = Checkbox(
      frame, {
                 .key = "music",
                 .id = "MusicCheckbox",
                 .checked = false,
                 .label = "Music",
                 .on_change = [&observed](bool value) { observed = value; },
             });

  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 640.0f, 480.0f);
  CHECK(result.ok);

  NodeId checkbox = tree.child_at(tree.root_id(), 0);
  NodeSnapshot checkbox_snapshot = {};
  CHECK(snapshot(tree, checkbox, &checkbox_snapshot));
  CHECK(strcmp(checkbox_snapshot.type, "Checkbox") == 0);
  CHECK(checkbox_snapshot.role == NodeRole::Checkbox);
  CHECK(checkbox_snapshot.semantic_role == SemanticRole::Checkbox);
  CHECK(checkbox_snapshot.interaction.focusable);
  CHECK(!checkbox_snapshot.interaction.checked);
  CHECK(checkbox_snapshot.child_count == 2);

  CHECK(tree.invoke_activate(checkbox));
  CHECK(observed);

  react_shutdown();
  return true;
}

static bool input_element_edits_controlled_text(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  std::string value = "A";

  UiElement root = Input(
      frame, {
                 .key = "name",
                 .id = "NameInput",
                 .autofocus = true,
                 .value = value.c_str(),
                 .on_change = [&value](const std::string &next) {
                   value = next;
                 },
             });
  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 640.0f, 480.0f);
  CHECK(result.ok);

  NodeId input = tree.child_at(tree.root_id(), 0);
  NodeSnapshot input_snapshot = {};
  CHECK(snapshot(tree, input, &input_snapshot));
  CHECK(strcmp(input_snapshot.type, "Input") == 0);
  CHECK(input_snapshot.role == NodeRole::Input);
  CHECK(input_snapshot.semantic_role == SemanticRole::TextBox);
  CHECK(strcmp(input_snapshot.value, "A") == 0);
  CHECK(input_snapshot.text_edit.caret == 1);

  ::ui::UiTextInputEvent text = {};
  strcpy(text.text, "B");
  CHECK(tree.invoke_text_input(input, text));
  CHECK(value == "AB");

  frame.reset();
  root = Input(frame, {
                          .key = "name",
                          .id = "NameInput",
                          .autofocus = true,
                          .value = value.c_str(),
                          .on_change =
                              [&value](const std::string &next) {
                                value = next;
                              },
                      });
  result = reconcile_retained_tree(tree, frame, root, 640.0f, 480.0f);
  CHECK(result.ok);

  ::ui::UiKeyInputEvent left = {.key = ::ui::UiKey::Left};
  CHECK(tree.invoke_key(input, left));
  ::ui::UiKeyInputEvent backspace = {.key = ::ui::UiKey::Backspace};
  CHECK(tree.invoke_key(input, backspace));
  CHECK(value == "B");

  react_shutdown();
  return true;
}

int main(void) {
  if (!button_element_returns_button_host())
    return 1;
  if (!box_text_and_dialog_elements_return_host_nodes())
    return 1;
  if (!checkbox_element_dispatches_changed_value())
    return 1;
  if (!input_element_edits_controlled_text())
    return 1;
  return 0;
}
