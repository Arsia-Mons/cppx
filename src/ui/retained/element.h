#pragma once

#include "../../react.h"
#include "ui_tree.h"

#include <array>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <new>
#include <stdint.h>
#include <type_traits>

namespace ui::retained {

constexpr int UI_RETAINED_MAX_ELEMENTS = 384;
constexpr int UI_RETAINED_MAX_CHILD_ELEMENTS = 384;
constexpr int UI_RETAINED_ELEMENT_ARENA_BYTES = 8192;
constexpr int UI_RETAINED_MAX_ELEMENT_DESTRUCTORS = 128;
constexpr int UI_RETAINED_STRING_ARENA_BYTES = 8192;

struct UiElement;
class UiElementFrame;

struct UiChildren {
  const UiElement *items = nullptr;
  int count = 0;
};

enum class UiElementKind : uint8_t {
  Empty,
  Fragment,
  Host,
  Component,
  Provider,
};

enum class HostKind : uint8_t {
  Box,
  Text,
};

struct TextProps {
  const char *value = "";
};

struct AutomationProps {
  const char *id = "";
  int offset = 0;
};

struct AccessibilityProps {
  SemanticRole role = SemanticRole::Auto;
};

struct HostCallbacks {
  std::function<void()> on_focus = {};
  std::function<void()> on_confirm = {};
};

struct HostProps {
  const char *key = nullptr;
  Style style = {};
  VisualStyle visual = {};
  TextProps text = {};
  NodeInteraction interaction = {};
  AutomationProps automation = {};
  AccessibilityProps accessibility = {};
  HostCallbacks callbacks = {};
  UiChildren children = {};
};

using UiComponentRenderFn = UiElement (*)(const void *props,
                                          UiElementFrame &frame);

struct ComponentElement {
  const char *name = "";
  const char *key = nullptr;
  const void *props = nullptr;
  UiComponentRenderFn render = nullptr;
};

struct ProviderElement {
  const char *name = "";
  const char *key = nullptr;
  ReactContext *context = nullptr;
  void *value = nullptr;
  UiChildren children = {};
};

struct HostElement {
  HostKind kind = HostKind::Box;
  HostProps props = {};
};

struct UiElement {
  UiElementKind kind = UiElementKind::Empty;
  UiChildren children = {};
  HostElement host = {};
  ComponentElement component = {};
  ProviderElement provider = {};
};

class UiElementFrame {
public:
  UiElementFrame();
  ~UiElementFrame();

  UiElementFrame(const UiElementFrame &) = delete;
  UiElementFrame &operator=(const UiElementFrame &) = delete;

  void reset();

  UiChildren children(std::initializer_list<UiElement> items);
  UiElement empty();
  UiElement fragment(UiChildren children);
  UiElement host(HostKind kind, const HostProps &props);
  UiElement box(const HostProps &props);
  UiElement text(const char *value, const char *key = nullptr,
                 const VisualStyle &visual = {});
  UiElement provider(const char *name, ReactContext *context, void *value,
                     UiChildren children, const char *key = nullptr);

  template <typename T> const T *copy_value(const T &value) {
    return store(value);
  }

  template <typename Props>
  UiElement component(const char *name, const Props &props,
                      UiElement (*render)(const Props &, UiElementFrame &),
                      const char *key = nullptr) {
    ComponentRecord<Props> record = {
        .render = render,
        .props = props,
    };
    const ComponentRecord<Props> *stored = store(record);
    if (!stored)
      return empty();
    return component_raw(name, key, stored, &render_component_record<Props>);
  }

  const char *copy_string(const char *value);

  int error_count() const { return error_count_; }

private:
  template <typename Props> struct ComponentRecord {
    UiElement (*render)(const Props &, UiElementFrame &);
    Props props;
  };

  struct DestructorEntry {
    void *storage = nullptr;
    void (*destroy)(void *storage) = nullptr;
  };

  template <typename T> static void destroy_value(void *storage) {
    static_cast<T *>(storage)->~T();
  }

  template <typename T>
  static UiElement render_component_record(const void *raw,
                                           UiElementFrame &frame) {
    const ComponentRecord<T> *record =
        static_cast<const ComponentRecord<T> *>(raw);
    if (!record || !record->render)
      return frame.empty();
    return record->render(record->props, frame);
  }

  template <typename T> const T *store(const T &value) {
    static_assert(!std::is_reference_v<T>);
    uintptr_t base = reinterpret_cast<uintptr_t>(arena_.data()) + arena_offset_;
    uintptr_t aligned =
        (base + alignof(T) - 1u) & ~(uintptr_t)(alignof(T) - 1u);
    size_t next_offset =
        (aligned - reinterpret_cast<uintptr_t>(arena_.data())) + sizeof(T);
    if (next_offset > arena_.size() ||
        destructor_count_ >= UI_RETAINED_MAX_ELEMENT_DESTRUCTORS) {
      ++error_count_;
      return nullptr;
    }

    void *storage = reinterpret_cast<void *>(aligned);
    new (storage) T(value);
    destructors_[destructor_count_++] = {
        .storage = storage,
        .destroy = &destroy_value<T>,
    };
    arena_offset_ = next_offset;
    return static_cast<const T *>(storage);
  }

  UiElement component_raw(const char *name, const char *key, const void *props,
                          UiComponentRenderFn render);
  HostProps copy_host_props(const HostProps &props);

  std::array<UiElement, UI_RETAINED_MAX_ELEMENTS> elements_ = {};
  std::array<UiElement, UI_RETAINED_MAX_CHILD_ELEMENTS> child_elements_ = {};
  std::array<std::byte, UI_RETAINED_ELEMENT_ARENA_BYTES> arena_ = {};
  std::array<DestructorEntry, UI_RETAINED_MAX_ELEMENT_DESTRUCTORS>
      destructors_ = {};
  std::array<char, UI_RETAINED_STRING_ARENA_BYTES> strings_ = {};
  int element_count_ = 0;
  int child_element_count_ = 0;
  size_t arena_offset_ = 0;
  int destructor_count_ = 0;
  int string_count_ = 0;
  int error_count_ = 0;
};

struct ReconcileResult {
  bool ok = false;
  int error_count = 0;
};

ReconcileResult reconcile_retained_tree(UiTree &tree, UiElementFrame &frame,
                                        const UiElement &root, float width,
                                        float height);
ReconcileResult commit_retained_elements(UiTree &tree, UiElementFrame &frame,
                                         const UiElement &root);

} // namespace ui::retained
