#pragma once

#include <array>
#include <stdint.h>

namespace ui::retained {

using NodeId = uint64_t;

constexpr int UI_RETAINED_MAX_NODES = 256;
constexpr int UI_RETAINED_MAX_DEPTH = 64;
constexpr int UI_RETAINED_MAX_CHILDREN = 64;
constexpr int UI_RETAINED_LABEL_CAP = 48;

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
  Column,
};

enum class AlignItems : uint8_t {
  Stretch,
  Start,
  Center,
  End,
};

enum class JustifyContent : uint8_t {
  Start,
  Center,
  End,
  SpaceBetween,
};

struct EdgeSizes {
  float left = 0.0f;
  float right = 0.0f;
  float top = 0.0f;
  float bottom = 0.0f;
};

struct Style {
  Length width = Length::auto_size();
  Length height = Length::auto_size();
  float flex_grow = 0.0f;
  FlexDirection direction = FlexDirection::Column;
  AlignItems align_items = AlignItems::Stretch;
  JustifyContent justify_content = JustifyContent::Start;
  EdgeSizes padding = {};
  float gap = 0.0f;
};

struct Rect {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
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

using CleanupFn = void (*)(void *user);
using MeasureFn = Size (*)(MeasureInput input, void *user);

struct NodeSnapshot {
  NodeId id = 0;
  NodeId parent_id = 0;
  const char *type = "";
  const char *key = "";
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
  bool measure(NodeId id, MeasureInput input, Size *out) const;
  bool set_layout(NodeId id, Rect rect);

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
    std::array<NodeId, UI_RETAINED_MAX_CHILDREN> children = {};
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
