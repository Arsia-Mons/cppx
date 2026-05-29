// P3: golden BMP verification of the new DrawCommand IR -> pixels path.
// Hand-authors a DrawCommandList exercising the new visual capabilities
// (rounded-rect fill, fused border+outline frame, linear gradient) and renders
// it through the executor, comparing to a committed golden.
//
// First run / intentional updates: UI_GOLDEN_REGEN=1 ctest -R renderer_golden
// regenerates tests/fixtures/golden/*.bmp. CI runs without it and compares.

#include "golden_util.h"
#include "renderer/draw_executor.h"
#include "ui/runtime/draw_command.h"

#include <stdio.h>

using namespace ui;

namespace {

constexpr int kTolerance = 2; // per-channel, absorbs software-rasterizer rounding

DrawCommandList &scene() {
  static DrawCommandList list; // large; keep off the stack
  list.reset();

  // (1) Rounded-rect solid fill — proves corner tessellation.
  {
    DrawCommand c{};
    c.kind = DrawCommandKind::Rect;
    c.rect = {8, 8, 48, 32};
    c.payload.rect.fill = {40, 120, 200, 255};
    c.payload.rect.corner_radius = 10.f;
    list.push(c);
  }
  // (2) Fused frame: per-side border + outset outline ring — proves co-feather.
  {
    DrawCommand c{};
    c.kind = DrawCommandKind::Border;
    c.rect = {72, 8, 48, 32};
    c.payload.border.corner_radius = 6.f;
    c.payload.border.border.width = {2, 2, 2, 2};
    const Color b{220, 90, 90, 255};
    c.payload.border.border.color = {b, b, b, b};
    c.payload.border.outline.width = 2.f;
    c.payload.border.outline.color = {120, 200, 120, 255};
    c.payload.border.outline.offset = 3.f; // outset
    list.push(c);
  }
  // (3) Linear gradient fill — proves per-vertex gradient.
  {
    DrawCommand c{};
    c.kind = DrawCommandKind::Gradient;
    c.rect = {8, 52, 112, 36};
    GradientStop stops[2] = {{0.f, {235, 90, 40, 255}}, {1.f, {40, 90, 235, 255}}};
    uint16_t off = 0;
    if (!list.push_stops(stops, 2, &off))
      return list;
    c.payload.gradient.stop_off = off;
    c.payload.gradient.stop_count = 2;
    c.payload.gradient.angle_deg = 0.f; // left -> right
    list.push(c);
  }
  return list;
}

bool run() {
  ui_test::GoldenContext ctx;
  if (!ctx.init()) {
    fprintf(stderr, "renderer_golden_tests: GoldenContext init failed\n");
    return false;
  }
  const DrawCommandList &list = scene();
  if (list.error_count != 0) {
    fprintf(stderr, "renderer_golden_tests: scene overflowed the command list\n");
    return false;
  }

  auto draw = [&](SDL_Renderer *r) {
    renderer::execute_draw_commands(r, list, /*fonts=*/nullptr);
  };

  ui_test::CompareReport rep;
  const bool ok = ui_test::render_and_compare(
      ctx, 128, 96, draw, "tests/fixtures/golden/new_ir_scene.bmp", kTolerance,
      &rep);
  if (!ok)
    fprintf(stderr, "renderer_golden_tests: %s\n", rep.message.c_str());
  return ok;
}

} // namespace

int main() {
  if (!run()) {
    fprintf(stderr, "renderer_golden_tests: FAIL\n");
    return 1;
  }
  printf("renderer_golden_tests: OK\n");
  return 0;
}
