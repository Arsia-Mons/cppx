#include "ui_tree.h"

#include <string.h>

namespace ui::retained {

namespace {

uint64_t hash_bytes(uint64_t hash, const char *text) {
  const unsigned char *cursor =
      reinterpret_cast<const unsigned char *>(text ? text : "");
  while (*cursor) {
    hash ^= *cursor++;
    hash *= 1099511628211ull;
  }
  return hash;
}

uint64_t mix_u64(uint64_t hash, uint64_t value) {
  hash ^= value;
  hash *= 1099511628211ull;
  hash ^= value >> 32u;
  hash *= 1099511628211ull;
  return hash;
}

} // namespace

UiTree::UiTree() = default;

UiTree::~UiTree() { reset(); }

void UiTree::reset() {
  for (int i = 0; i < node_capacity_used_; ++i) {
    destroy_node(nodes_[i]);
  }
  nodes_ = {};
  stack_ = {};
  unmounted_ = {};
  node_capacity_used_ = 0;
  stack_count_ = 0;
  unmounted_count_ = 0;
  generation_ = 0;
  error_count_ = 0;
}

void UiTree::begin_frame(float width, float height) {
  ++generation_;
  if (generation_ == 0)
    generation_ = 1;

  stack_count_ = 0;
  unmounted_count_ = 0;

  Style root_style = {};
  root_style.width = Length::points(width);
  root_style.height = Length::points(height);

  Node *root = ensure_node(UI_RETAINED_ROOT_ID, 0, "Root", "", root_style);
  if (!root)
    return;

  root->layout = {0.0f, 0.0f, width, height};
  stack_[stack_count_++] = static_cast<int>(root - nodes_.data());
}

bool UiTree::end_frame() {
  if (stack_count_ != 1) {
    report_error();
    stack_count_ = stack_count_ > 0 ? 1 : 0;
  }

  for (int i = 0; i < node_capacity_used_; ++i) {
    Node &node = nodes_[i];
    if (node.id == 0 || node.generation == generation_)
      continue;
    NodeId removed = node.id;
    destroy_node(node);
    if (unmounted_count_ < UI_RETAINED_MAX_NODES) {
      unmounted_[unmounted_count_++] = removed;
    }
  }

  stack_count_ = 0;
  return error_count_ == 0;
}

NodeId UiTree::begin_node(const char *type, const Style &style) {
  return begin_keyed_node(type, nullptr, style);
}

NodeId UiTree::begin_keyed_node(const char *type, const char *key,
                                const Style &style) {
  if (stack_count_ <= 0 || stack_count_ >= UI_RETAINED_MAX_DEPTH) {
    report_error();
    return 0;
  }

  Node &parent = nodes_[stack_[stack_count_ - 1]];
  uint32_t sibling_index = parent.next_child_index++;

  bool keyed = key && key[0] != '\0';
  NodeId id = make_child_id(parent.id, type, key, sibling_index, keyed);
  Node *node = ensure_node(id, parent.id, type, keyed ? key : "", style);
  if (!node)
    return 0;

  if (parent.child_count >= UI_RETAINED_MAX_CHILDREN) {
    report_error();
    return 0;
  }
  parent.children[parent.child_count++] = id;

  stack_[stack_count_++] = static_cast<int>(node - nodes_.data());
  return id;
}

bool UiTree::end_node() {
  if (stack_count_ <= 1) {
    report_error();
    return false;
  }
  --stack_count_;
  return true;
}

bool UiTree::set_cleanup(NodeId id, CleanupFn cleanup, void *user) {
  Node *node = find_mutable(id);
  if (!node)
    return false;
  node->cleanup = cleanup;
  node->cleanup_user = user;
  return true;
}

bool UiTree::set_measure(NodeId id, MeasureFn measure, void *user) {
  Node *node = find_mutable(id);
  if (!node)
    return false;
  node->measure = measure;
  node->measure_user = user;
  return true;
}

bool UiTree::set_metadata(NodeId id, const NodeMetadata &metadata) {
  Node *node = find_mutable(id);
  if (!node)
    return false;
  node->role = metadata.role;
  node->interaction = metadata.interaction;
  node->visual = metadata.visual;
  node->on_confirm = metadata.on_confirm;
  copy_label(node->control_id, metadata.control_id);
  copy_value(node->value, metadata.value);
  return true;
}

bool UiTree::measure(NodeId id, MeasureInput input, Size *out) const {
  if (!out)
    return false;
  const Node *node = find(id);
  if (!node || !node->measure)
    return false;
  *out = node->measure(input, node->measure_user);
  return true;
}

bool UiTree::set_layout(NodeId id, Rect rect) {
  Node *node = find_mutable(id);
  if (!node)
    return false;
  node->layout = rect;
  return true;
}

bool UiTree::invoke_confirm(NodeId id) const {
  const Node *node = find(id);
  if (!node || !node->interaction.focusable || node->interaction.disabled ||
      !node->on_confirm)
    return false;
  node->on_confirm();
  return true;
}

bool UiTree::snapshot(NodeId id, NodeSnapshot *out) const {
  if (!out)
    return false;
  const Node *node = find(id);
  if (!node)
    return false;
  *out = {
      .id = node->id,
      .parent_id = node->parent_id,
      .type = node->type,
      .key = node->key,
      .control_id = node->control_id,
      .value = node->value,
      .role = node->role,
      .interaction = node->interaction,
      .style = node->style,
      .visual = node->visual,
      .layout = node->layout,
      .child_count = node->child_count,
      .has_measure = node->measure != nullptr,
      .mounted_this_frame = node->mounted_this_frame,
  };
  return true;
}

bool UiTree::contains(NodeId id) const { return find(id) != nullptr; }

NodeId UiTree::current_parent_id() const {
  if (stack_count_ <= 0)
    return 0;
  return nodes_[stack_[stack_count_ - 1]].id;
}

int UiTree::node_count() const {
  int count = 0;
  for (int i = 0; i < node_capacity_used_; ++i) {
    if (nodes_[i].id != 0)
      ++count;
  }
  return count;
}

int UiTree::child_count(NodeId id) const {
  const Node *node = find(id);
  return node ? node->child_count : 0;
}

NodeId UiTree::child_at(NodeId id, int index) const {
  const Node *node = find(id);
  if (!node || index < 0 || index >= node->child_count)
    return 0;
  return node->children[index];
}

NodeId UiTree::unmounted_at(int index) const {
  if (index < 0 || index >= unmounted_count_)
    return 0;
  return unmounted_[index];
}

UiTree::Node *UiTree::find_mutable(NodeId id) {
  for (int i = 0; i < node_capacity_used_; ++i) {
    if (nodes_[i].id == id)
      return &nodes_[i];
  }
  return nullptr;
}

const UiTree::Node *UiTree::find(NodeId id) const {
  for (int i = 0; i < node_capacity_used_; ++i) {
    if (nodes_[i].id == id)
      return &nodes_[i];
  }
  return nullptr;
}

UiTree::Node *UiTree::ensure_node(NodeId id, NodeId parent_id, const char *type,
                                  const char *key, const Style &style) {
  if (id == 0) {
    report_error();
    return nullptr;
  }

  Node *existing = find_mutable(id);
  if (existing) {
    existing->parent_id = parent_id;
    existing->generation = generation_;
    existing->next_child_index = 0;
    existing->child_count = 0;
    existing->mounted_this_frame = false;
    existing->style = style;
    existing->role = NodeRole::Generic;
    existing->interaction = {};
    existing->visual = {};
    existing->on_confirm = {};
    existing->control_id[0] = '\0';
    existing->value[0] = '\0';
    copy_label(existing->type, type);
    copy_label(existing->key, key);
    return existing;
  }

  int slot = -1;
  for (int i = 0; i < node_capacity_used_; ++i) {
    if (nodes_[i].id == 0) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    if (node_capacity_used_ >= UI_RETAINED_MAX_NODES) {
      report_error();
      return nullptr;
    }
    slot = node_capacity_used_++;
  }

  Node &node = nodes_[slot];
  node = {};
  node.id = id;
  node.parent_id = parent_id;
  node.generation = generation_;
  node.style = style;
  node.mounted_this_frame = true;
  copy_label(node.type, type);
  copy_label(node.key, key);
  return &node;
}

void UiTree::destroy_node(Node &node) {
  if (node.id == 0)
    return;
  if (node.cleanup)
    node.cleanup(node.cleanup_user);
  node = {};
}

NodeId UiTree::make_child_id(NodeId parent_id, const char *type,
                             const char *key, uint32_t sibling_index,
                             bool keyed) const {
  uint64_t hash = 1469598103934665603ull;
  hash = mix_u64(hash, parent_id);
  hash = hash_bytes(hash, type);
  hash = mix_u64(hash, keyed ? 0x9E3779B97F4A7C15ull : 0x85EBCA6B27D4EB2Full);
  if (keyed) {
    hash = hash_bytes(hash, key);
  } else {
    hash = mix_u64(hash, sibling_index);
  }
  return hash == 0 ? 1ull : hash;
}

void UiTree::copy_label(char (&dest)[UI_RETAINED_LABEL_CAP],
                        const char *source) {
  const char *safe_source = source ? source : "";
  strncpy(dest, safe_source, UI_RETAINED_LABEL_CAP - 1);
  dest[UI_RETAINED_LABEL_CAP - 1] = '\0';
}

void UiTree::copy_value(char (&dest)[UI_RETAINED_VALUE_CAP],
                        const char *source) {
  const char *safe_source = source ? source : "";
  strncpy(dest, safe_source, UI_RETAINED_VALUE_CAP - 1);
  dest[UI_RETAINED_VALUE_CAP - 1] = '\0';
}

void UiTree::report_error() { ++error_count_; }

} // namespace ui::retained
