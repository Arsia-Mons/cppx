#pragma once

#include "../input.h"

#include <array>
#include <functional>
#include <stdint.h>

namespace ui::retained {

using NodeId = uint64_t;

constexpr int UI_RETAINED_MAX_NODES = 256;
constexpr int UI_RETAINED_MAX_DEPTH = 64;
constexpr int UI_RETAINED_MAX_CHILDREN = 64;
constexpr int UI_RETAINED_LABEL_CAP = 48;
constexpr int UI_RETAINED_VALUE_CAP = 96;

constexpr NodeId UI_RETAINED_ROOT_ID = 0xCBF29CE484222325ull;

enum class LengthKind : uint8_t {
  Auto,
  Points,
  Percent,
  Grow,
};

struct Length {
  LengthKind kind = LengthKind::Auto;
  float value = 0.0f;

  static Length auto_size() { return {}; }
  static Length points(float value) { return {LengthKind::Points, value}; }
  static Length percent(float value) { return {LengthKind::Percent, value}; }
  static Length grow(float value = 1.0f) { return {LengthKind::Grow, value}; }
};

enum class FlexDirection : uint8_t {
  Row,
  RowReverse,
  Column,
  ColumnReverse,
};

enum class AlignItems : uint8_t {
  Auto,
  Stretch,
  Start,
  Center,
  End,
  Baseline,
  SpaceBetween,
  SpaceAround,
  SpaceEvenly,
};

enum class JustifyContent : uint8_t {
  Start,
  Center,
  End,
  SpaceBetween,
  SpaceAround,
  SpaceEvenly,
};

enum class FlexWrap : uint8_t {
  NoWrap,
  Wrap,
  WrapReverse,
};

enum class PositionType : uint8_t {
  Static,
  Relative,
  Absolute,
};

enum class Overflow : uint8_t {
  Visible,
  Hidden,
  Scroll,
};

enum class Display : uint8_t {
  Flex,
  None,
  Contents,
};

enum class NodeRole : uint8_t {
  Generic,
  Box,
  Text,
  Button,
  Input,
  Checkbox,
  Dialog,
};

enum class SemanticRole : uint8_t {
  Auto,
  Button,
  Checkbox,
  TextBox,
  Tab,
  Dialog,
};

struct EdgeSizes {
  float left = 0.0f;
  float right = 0.0f;
  float top = 0.0f;
  float bottom = 0.0f;
};

struct Rect {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 0;
};

struct Style {
  Display display = Display::Flex;
  PositionType position = PositionType::Relative;
  Overflow overflow = Overflow::Visible;
  FlexDirection direction = FlexDirection::Column;
  FlexWrap wrap = FlexWrap::NoWrap;
  AlignItems align_items = AlignItems::Stretch;
  AlignItems align_content = AlignItems::Start;
  AlignItems align_self = AlignItems::Auto;
  JustifyContent justify_content = JustifyContent::Start;

  Length width = Length::auto_size();
  Length height = Length::auto_size();
  Length min_width = Length::auto_size();
  Length min_height = Length::auto_size();
  Length max_width = Length::auto_size();
  Length max_height = Length::auto_size();
  Length flex_basis = Length::auto_size();
  float flex_grow = 0.0f;
  float flex_shrink = 0.0f;
  float aspect_ratio = 0.0f;

  EdgeSizes margin = {};
  EdgeSizes padding = {};
  EdgeSizes position_inset = {};
  float gap = 0.0f;
  float row_gap = 0.0f;
  float column_gap = 0.0f;

  Color background = {};
  Color border = {};
  Color text = {};
  float border_width = 0.0f;
  uint16_t font_size = 0;
};

enum class MeasureMode : uint8_t {
  Undefined,
  Exactly,
  AtMost,
};

struct MeasureInput {
  float width = 0.0f;
  float height = 0.0f;
  MeasureMode width_mode = MeasureMode::Undefined;
  MeasureMode height_mode = MeasureMode::Undefined;
};

struct Size {
  float width = 0.0f;
  float height = 0.0f;
};

struct Point {
  float x = 0.0f;
  float y = 0.0f;
};

struct NodeInteraction {
  bool focusable = false;
  bool disabled = false;
  bool checked = false;
  bool modal = false;
  bool initial_focus = false;
};

struct TextEditMetadata {
  int caret = 0;
  int selection_start = 0;
  int selection_end = 0;
  const char *composition = "";
  int composition_start = 0;
  int composition_length = 0;
};

struct FocusEvent {
  NodeId target = 0;
};

struct BlurEvent {
  NodeId target = 0;
};

struct ActivationEvent {
  NodeId target = 0;
};

struct KeyEvent {
  NodeId target = 0;
  ::ui::UiKey key = ::ui::UiKey::Unknown;
  uint16_t modifiers = ::ui::UI_KEY_MOD_NONE;
  bool repeat = false;
};

struct TextInputEvent {
  NodeId target = 0;
  const char *text = "";
};

struct TextEditingEvent {
  NodeId target = 0;
  const char *text = "";
  int start = 0;
  int length = 0;
};

struct NodeMetadata {
  NodeRole role = NodeRole::Generic;
  SemanticRole semantic_role = SemanticRole::Auto;
  const char *control_id = "";
  int control_offset = 0;
  const char *accessibility_label = "";
  const char *accessibility_description = "";
  const char *value = "";
  NodeInteraction interaction = {};
  TextEditMetadata text_edit = {};
  std::function<void(const FocusEvent &)> on_focus = {};
  std::function<void(const BlurEvent &)> on_blur = {};
  std::function<void(const ActivationEvent &)> on_activate = {};
  std::function<void(const KeyEvent &)> on_key = {};
  std::function<void(const TextInputEvent &)> on_text_input = {};
  std::function<void(const TextEditingEvent &)> on_text_editing = {};
};

using CleanupFn = void (*)(void *user);
using MeasureFn = Size (*)(MeasureInput input, void *user);

struct NodeSnapshot {
  NodeId id = 0;
  NodeId parent_id = 0;
  const char *type = "";
  const char *key = "";
  const char *control_id = "";
  int control_offset = 0;
  const char *accessibility_label = "";
  const char *accessibility_description = "";
  const char *value = "";
  NodeRole role = NodeRole::Generic;
  SemanticRole semantic_role = SemanticRole::Auto;
  NodeInteraction interaction = {};
  TextEditMetadata text_edit = {};
  Style style = {};
  Rect layout = {};
  int child_count = 0;
  bool has_measure = false;
  bool mounted_this_frame = false;
};

class UiTree {
public:
  UiTree();
  ~UiTree();

  UiTree(const UiTree &) = delete;
  UiTree &operator=(const UiTree &) = delete;

  void reset();

  void begin_frame(float width, float height);
  bool end_frame();

  NodeId begin_node(const char *type, const Style &style = {});
  NodeId begin_keyed_node(const char *type, const char *key,
                          const Style &style = {});
  bool end_node();

  bool set_cleanup(NodeId id, CleanupFn cleanup, void *user);
  bool set_measure(NodeId id, MeasureFn measure, void *user);
  bool set_metadata(NodeId id, const NodeMetadata &metadata);
  bool measure(NodeId id, MeasureInput input, Size *out) const;
  bool set_layout(NodeId id, Rect rect);
  bool invoke_focus(NodeId id) const;
  bool invoke_blur(NodeId id) const;
  bool invoke_activate(NodeId id) const;
  bool invoke_key(NodeId id, const ::ui::UiKeyInputEvent &event) const;
  bool invoke_text_input(NodeId id, const ::ui::UiTextInputEvent &event) const;
  bool invoke_text_editing(NodeId id,
                           const ::ui::UiTextEditingEvent &event) const;

  bool snapshot(NodeId id, NodeSnapshot *out) const;
  bool contains(NodeId id) const;

  NodeId root_id() const { return UI_RETAINED_ROOT_ID; }
  NodeId current_parent_id() const;
  int node_count() const;
  int child_count(NodeId id) const;
  NodeId child_at(NodeId id, int index) const;
  int unmounted_count() const { return unmounted_count_; }
  NodeId unmounted_at(int index) const;
  int error_count() const { return error_count_; }

private:
  struct Node {
    NodeId id = 0;
    NodeId parent_id = 0;
    uint32_t generation = 0;
    uint32_t next_child_index = 0;
    int child_count = 0;
    bool mounted_this_frame = false;
    char type[UI_RETAINED_LABEL_CAP] = {};
    char key[UI_RETAINED_LABEL_CAP] = {};
    char control_id[UI_RETAINED_LABEL_CAP] = {};
    int control_offset = 0;
    char accessibility_label[UI_RETAINED_VALUE_CAP] = {};
    char accessibility_description[UI_RETAINED_VALUE_CAP] = {};
    char value[UI_RETAINED_VALUE_CAP] = {};
    char composition[UI_RETAINED_VALUE_CAP] = {};
    std::array<NodeId, UI_RETAINED_MAX_CHILDREN> children = {};
    NodeRole role = NodeRole::Generic;
    SemanticRole semantic_role = SemanticRole::Auto;
    NodeInteraction interaction = {};
    TextEditMetadata text_edit = {};
    std::function<void(const FocusEvent &)> on_focus = {};
    std::function<void(const BlurEvent &)> on_blur = {};
    std::function<void(const ActivationEvent &)> on_activate = {};
    std::function<void(const KeyEvent &)> on_key = {};
    std::function<void(const TextInputEvent &)> on_text_input = {};
    std::function<void(const TextEditingEvent &)> on_text_editing = {};
    Style style = {};
    Rect layout = {};
    CleanupFn cleanup = nullptr;
    void *cleanup_user = nullptr;
    MeasureFn measure = nullptr;
    void *measure_user = nullptr;
  };

  Node *find_mutable(NodeId id);
  const Node *find(NodeId id) const;
  Node *ensure_node(NodeId id, NodeId parent_id, const char *type,
                    const char *key, const Style &style);
  void destroy_node(Node &node);
  NodeId make_child_id(NodeId parent_id, const char *type, const char *key,
                       uint32_t sibling_index, bool keyed) const;
  void copy_label(char (&dest)[UI_RETAINED_LABEL_CAP], const char *source);
  void copy_value(char (&dest)[UI_RETAINED_VALUE_CAP], const char *source);
  void report_error();

  std::array<Node, UI_RETAINED_MAX_NODES> nodes_ = {};
  std::array<int, UI_RETAINED_MAX_DEPTH> stack_ = {};
  std::array<NodeId, UI_RETAINED_MAX_NODES> unmounted_ = {};
  int node_capacity_used_ = 0;
  int stack_count_ = 0;
  int unmounted_count_ = 0;
  uint32_t generation_ = 0;
  int error_count_ = 0;
};

} // namespace ui::retained
