#include "client/ui/ui_pipeline.h"

#include "client/ui/navigation/ui_screen.h"
#include "react.h"
#include "ui/components/components.h"
#include "ui/runtime/draw_command.h"
#include "ui/runtime/focus.h"

#include <memory>
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

using client::ui::ScreenNavigator;
using client::ui::UiPipeline;
using client::ui::UiPipelineFrame;
using client::ui::UiScreen;

static UiPipelineFrame test_frame(::ui::UiInputFrame input = {}) {
  return {
      .input = input,
      .layout = {640, 480},
      .pointer = {-1000.0f, -1000.0f},
  };
}

struct FrameProviderProbeScreenProps {
  bool *observed_;
};

static ::ui::UiElement
FrameProviderProbeScreenView(const FrameProviderProbeScreenProps &props) {
  const UiPipelineFrame *frame = client::ui::use_ui_pipeline_frame();
  if (props.observed_)
    *props.observed_ = frame && frame->input.nav_right;
  return ::ui::empty();
}

struct RetainedProbeScreenProps {
  uint32_t unused = 0;
};

struct PopOnRetainedConfirmScreenProps {
  uint32_t unused = 0;
};

static const char *screen_entry_key(const char *prefix,
                                    client::ui::UiScreenEntryId entry_id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
  return ::ui::copy_string(key);
}

class FrameProviderProbeScreen final : public UiScreen {
public:
  explicit FrameProviderProbeScreen(bool *observed) : observed_(observed) {}

  const char *debug_name() const override { return "FrameProviderProbe"; }

  bool build_element(::ui::UiElementFrame &frame,
                     ::ui::UiElement *out) override {
    if (!out)
      return false;
    *out = ::ui::component("FrameProviderProbeScreen",
                           FrameProviderProbeScreenProps{.observed_ =
                                                             observed_},
                           FrameProviderProbeScreenView,
                           screen_entry_key("frame-provider", entry_id()));
    return true;
  }

  void build_ui() override {}

private:
  bool *observed_ = nullptr;
};

static ::ui::UiElement
RetainedProbeScreenView(const RetainedProbeScreenProps &props) {
  (void)props;
  return ::ui::components::elements::Button({
      .key = "confirm",
      .id = "PipelineRetainedButton",
      .label = "Retained",
  });
}

static ::ui::UiElement PopOnRetainedConfirmScreenView(
    const PopOnRetainedConfirmScreenProps &props) {
  (void)props;
  ScreenNavigator nav = client::ui::use_screen_navigator();
  return ::ui::components::elements::Button({
      .key = "pop",
      .id = "PipelineRetainedPopButton",
      .label = "Pop",
      .on_activate =
          [pop = nav.pop_current](const ::ui::ActivationEvent &) {
            if (pop)
              pop();
          },
  });
}

class RetainedProbeScreen final : public UiScreen {
public:
  const char *debug_name() const override { return "RetainedProbe"; }

  bool build_element(::ui::UiElementFrame &frame,
                     ::ui::UiElement *out) override {
    if (!out)
      return false;
    *out =
        ::ui::component("RetainedProbeScreenView", RetainedProbeScreenProps{},
                        RetainedProbeScreenView,
                        screen_entry_key("retained-probe", entry_id()));
    return true;
  }

  void build_ui() override {}
};

class PopOnRetainedConfirmScreen final : public UiScreen {
public:
  const char *debug_name() const override { return "PopOnRetainedConfirm"; }

  bool build_element(::ui::UiElementFrame &frame,
                     ::ui::UiElement *out) override {
    if (!out)
      return false;
    *out = ::ui::component("PopOnRetainedConfirmScreenView",
                           PopOnRetainedConfirmScreenProps{},
                           PopOnRetainedConfirmScreenView,
                           screen_entry_key("pop-retained", entry_id()));
    return true;
  }

  void build_ui() override {}
};

struct RenderProbe {
  int render_count = 0;
  int pending_mutations_at_render = 0;
  int screen_count_at_render = 0;
};

struct RetainedRenderProbe {
  int render_count = 0;
  int draw_count = 0;
  bool saw_button_rect = false;
  bool saw_label_text = false;
  ::ui::NodeId button_id = 0;
  ::ui::NodeId focused_id = 0;
};

static bool same_color(::ui::Color actual, ::ui::Color expected) {
  return actual.r == expected.r && actual.g == expected.g &&
         actual.b == expected.b && actual.a == expected.a;
}

static ::ui::NodeId find_retained_control(const ::ui::UiTree &tree,
                                          ::ui::NodeId id,
                                          const char *name) {
  ::ui::NodeSnapshot node = {};
  if (!tree.snapshot(id, &node))
    return 0;
  if (strcmp(node.control_id ? node.control_id : "", name) == 0)
    return id;
  for (int i = 0; i < tree.child_count(id); ++i) {
    ::ui::NodeId found =
        find_retained_control(tree, tree.child_at(id, i), name);
    if (found != 0)
      return found;
  }
  return 0;
}

static bool ui_pipeline_frame_provider_exposes_current_frame(void) {
  react_init_runtime();
  UiPipeline pipeline;
  bool observed = false;

  CHECK(pipeline.client_ui().push_screen(
      std::make_unique<FrameProviderProbeScreen>(&observed)));
  ::ui::UiInputFrame input = {};
  input.nav_right = true;
  pipeline.render_client_ui_frame(test_frame(input), {});

  CHECK(observed);
  return true;
}

static bool pipeline_renders_before_draining_client_mutations(void) {
  react_init_runtime();
  UiPipeline pipeline;
  RenderProbe probe = {};

  CHECK(pipeline.client_ui().push_screen(
      std::make_unique<PopOnRetainedConfirmScreen>()));
  pipeline.render_client_ui_frame(test_frame(), {});
  CHECK(pipeline.client_ui().screens().count() == 1);

  ::ui::UiInputFrame confirm = {};
  confirm.confirm_pressed = true;
  confirm.confirm_down = true;
  confirm.source = ::ui::UiFocusSource::Keyboard;

  pipeline.render_client_ui_frame(test_frame(confirm), [&] {
    probe.render_count += 1;
    probe.pending_mutations_at_render =
        pipeline.client_ui().pending_mutation_count();
    probe.screen_count_at_render = pipeline.client_ui().screens().count();
  });

  CHECK(probe.render_count == 1);
  CHECK(probe.pending_mutations_at_render == 1);
  CHECK(probe.screen_count_at_render == 1);
  CHECK(pipeline.client_ui().screens().count() == 0);
  return true;
}

static bool pipeline_updates_retained_runtime_before_render(void) {
  react_init_runtime();
  UiPipeline pipeline;
  RetainedRenderProbe probe = {};

  CHECK(pipeline.client_ui().push_screen(
      std::make_unique<RetainedProbeScreen>()));

  pipeline.render_client_ui_frame(test_frame(), [&] {
    probe.render_count += 1;
    // The live IR is the new tagged-union DrawCommandList: the focused button
    // emits a fill (Gradient/Rect), a Border, and a Text command for its label.
    const ::ui::DrawCommandList &draw =
        pipeline.client_ui().retained_command_list();
    probe.draw_count = draw.count;
    probe.focused_id =
        ::ui::focus_focused_id(pipeline.client_ui().retained_focus());
    for (int i = 0; i < draw.count; ++i) {
      const ::ui::DrawCommand &command = draw.commands[i];
      if (command.kind == ::ui::DrawCommandKind::Rect ||
          command.kind == ::ui::DrawCommandKind::Gradient) {
        probe.saw_button_rect = true;
        probe.button_id = command.node_id;
      }
      if (command.kind == ::ui::DrawCommandKind::Text) {
        const ::ui::TextData &t = command.payload.text;
        if (t.text_len == strlen("Retained") &&
            t.text_off + t.text_len <=
                static_cast<uint32_t>(draw.text_len_used) &&
            memcmp(draw.text_arena + t.text_off, "Retained", t.text_len) == 0) {
          probe.saw_label_text = true;
        }
      }
    }
  });

  CHECK(probe.render_count == 1);
  CHECK(probe.draw_count > 0);
  CHECK(probe.saw_button_rect);
  CHECK(probe.saw_label_text);
  CHECK(probe.button_id != 0);
  CHECK(probe.focused_id == probe.button_id);
  return true;
}

static bool pipeline_applies_hover_visual_after_pointer_hit_test(void) {
  react_init_runtime();
  UiPipeline pipeline;

  CHECK(pipeline.client_ui().push_screen(
      std::make_unique<RetainedProbeScreen>()));
  pipeline.render_client_ui_frame(test_frame(), {});

  ::ui::NodeId button = find_retained_control(
      pipeline.client_ui().retained_tree(),
      pipeline.client_ui().retained_tree().root_id(),
      "PipelineRetainedButton");
  CHECK(button != 0);

  ::ui::NodeSnapshot base = {};
  CHECK(pipeline.client_ui().retained_tree().snapshot(button, &base));
  CHECK(base.visual.gradient.stop_count == 2);

  UiPipelineFrame hover_frame = test_frame();
  hover_frame.pointer = {
      base.layout.x + base.layout.width * 0.5f,
      base.layout.y + base.layout.height * 0.5f,
  };
  pipeline.render_client_ui_frame(hover_frame, {});
  CHECK(::ui::focus_hovered_id(pipeline.client_ui().retained_focus()) ==
        button);

  // Interaction state is published with a one-frame lag, so the following build
  // should resolve the hovered visual into the button node.
  pipeline.render_client_ui_frame(hover_frame, {});
  ::ui::NodeSnapshot hovered = {};
  CHECK(pipeline.client_ui().retained_tree().snapshot(button, &hovered));
  CHECK(hovered.visual.gradient.stop_count == 2);
  CHECK(!same_color(hovered.visual.gradient.stops[0].color,
                    base.visual.gradient.stops[0].color));
  CHECK(!same_color(hovered.visual.border.color.top,
                    base.visual.border.color.top));
  return true;
}

static bool pipeline_invokes_retained_confirm_before_render(void) {
  react_init_runtime();
  UiPipeline pipeline;
  RenderProbe probe = {};

  CHECK(pipeline.client_ui().push_screen(
      std::make_unique<PopOnRetainedConfirmScreen>()));
  pipeline.render_client_ui_frame(test_frame(), {});
  CHECK(pipeline.client_ui().screens().count() == 1);

  ::ui::UiInputFrame confirm = {};
  confirm.confirm_pressed = true;
  confirm.confirm_down = true;
  confirm.source = ::ui::UiFocusSource::Keyboard;

  pipeline.render_client_ui_frame(test_frame(confirm), [&] {
    probe.render_count += 1;
    probe.pending_mutations_at_render =
        pipeline.client_ui().pending_mutation_count();
    probe.screen_count_at_render = pipeline.client_ui().screens().count();
  });

  CHECK(probe.render_count == 1);
  CHECK(probe.pending_mutations_at_render == 1);
  CHECK(probe.screen_count_at_render == 1);
  CHECK(pipeline.client_ui().screens().count() == 0);
  return true;
}

int main(void) {
  if (!ui_pipeline_frame_provider_exposes_current_frame())
    return 1;
  if (!pipeline_renders_before_draining_client_mutations())
    return 1;
  if (!pipeline_updates_retained_runtime_before_render())
    return 1;
  if (!pipeline_applies_hover_visual_after_pointer_hit_test())
    return 1;
  if (!pipeline_invokes_retained_confirm_before_render())
    return 1;

  react_shutdown();
  return 0;
}
