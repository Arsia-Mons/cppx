// P3/P5: golden BMP verification of the new DrawCommand IR -> pixels path.
// Hand-authors DrawCommandLists exercising the new visual capabilities
// (rounded-rect fill, fused border+outline frame, linear gradient, drop shadow,
// and textured Image: plain / nine-slice / rounded) and renders them through
// the executor, comparing to committed goldens.
//
// First run / intentional updates: UI_GOLDEN_REGEN=1 ctest -R renderer_golden
// regenerates tests/fixtures/golden/*.bmp. CI runs without it and compares.

#include "golden_util.h"
#include "renderer/draw_executor.h"
#include "renderer/texture_registry.h"
#include "ui/runtime/draw_command.h"

#include <stdio.h>
#include <vector>

using namespace ui;

namespace {

constexpr int kTolerance = 2; // per-channel, absorbs software-rasterizer rounding

// ---------------------------------------------------------------------------
// Scene 1: rounded fill + frame + gradient + drop shadow (P3 + P5 shadow).
// ---------------------------------------------------------------------------
DrawCommandList &scene() {
  static DrawCommandList list; // large; keep off the stack
  list.reset();

  // (0) Drop shadow BEHIND the rounded fill below — proves the feathered skirt
  // and the Shadow executor path. The shadow color is a COLORED premultiplied
  // value (not black) and the offset/blur are large enough that the skirt clearly
  // pokes out beyond the occluding blue rect: a pure-black shadow over the black
  // clear would composite to black (invisible), so a colored shadow is what makes
  // this a meaningful golden. (run_scene also pixel-probes the skirt region.)
  {
    DrawCommand c{};
    c.kind = DrawCommandKind::Shadow;
    c.rect = {8, 8, 48, 32};                   // same box as the rounded fill (1)
    c.payload.shadow.color = {60, 90, 160, 220}; // premultiplied (rgb<=a) blue-ish
    c.payload.shadow.offset = {7.f, 7.f};      // skirt pokes down-right of the box
    c.payload.shadow.blur = 6.f;
    c.payload.shadow.spread = 1.f;
    c.payload.shadow.corner_radius = 10.f;
    list.push(c);
  }
  // (1) Rounded-rect solid fill — proves corner tessellation (sits on shadow).
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

bool run_scene() {
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

  // Independent of the golden bytes: prove the Shadow path actually rasterized
  // by sampling the skirt band that pokes out DOWN-RIGHT of the blue rect (which
  // ends at x=56,y=40). With the colored shadow offset {7,7}, pixels in
  // x[57,68) y[41,52) must be non-black — a black shadow over a black clear would
  // composite invisibly, so this guards against a silently-broken executor case.
  {
    ui_test::Image probe;
    if (!ctx.render_to_image(128, 96, draw, &probe)) {
      fprintf(stderr, "renderer_golden_tests (scene): probe render failed\n");
      return false;
    }
    int lit = 0;
    for (int y = 41; y < 52; ++y) {
      for (int x = 57; x < 68; ++x) {
        uint8_t p[4];
        probe.at(x, y, p);
        if (p[0] > 4 || p[1] > 4 || p[2] > 4)
          ++lit;
      }
    }
    if (lit == 0) {
      fprintf(stderr,
              "renderer_golden_tests (scene): shadow skirt produced no visible "
              "pixels — the Shadow executor path is not rendering\n");
      return false;
    }
  }

  ui_test::CompareReport rep;
  const bool ok = ui_test::render_and_compare(
      ctx, 128, 96, draw, "tests/fixtures/golden/new_ir_scene.bmp", kTolerance,
      &rep);
  if (!ok)
    fprintf(stderr, "renderer_golden_tests (scene): %s\n", rep.message.c_str());
  return ok;
}

// ---------------------------------------------------------------------------
// Scene 2: textured Image — plain / nine-slice / rounded (P5 image).
//
// No app currently sets a BackgroundImage and there is no app-side texture
// loader, so this exercises the executor's Image path against an in-repo,
// procedurally-generated test texture uploaded through the renderer-side
// TextureRegistry. This proves the Image enum is no longer a silent no-op; it
// does NOT invent app-side image usage (scoping note in the step summary).
// ---------------------------------------------------------------------------

// A 16x16 RGBA test sprite: a 3px magenta border with a 1px corner inset and a
// teal interior — chosen so nine-slice corner fidelity and edge stretch are both
// visible. Straight alpha (the registry premultiplies at upload).
std::vector<uint8_t> make_test_sprite(int w, int h) {
  std::vector<uint8_t> px(static_cast<size_t>(w) * h * 4u, 0);
  const int border = 3;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      uint8_t *p = &px[(static_cast<size_t>(y) * w + x) * 4u];
      const bool edge = x < border || x >= w - border || y < border ||
                        y >= h - border;
      if (edge) {
        p[0] = 235; p[1] = 40; p[2] = 200; p[3] = 255; // magenta border
      } else {
        p[0] = 40; p[1] = 180; p[2] = 170; p[3] = 255; // teal interior
      }
    }
  }
  return px;
}

bool run_image_scene() {
  ui_test::GoldenContext ctx;
  if (!ctx.init()) {
    fprintf(stderr, "renderer_golden_tests: GoldenContext init failed\n");
    return false;
  }

  renderer::TextureRegistry textures;
  const int tw = 16, th = 16;
  std::vector<uint8_t> sprite = make_test_sprite(tw, th);
  const uint32_t tex_id =
      textures.upload_rgba(ctx.renderer(), sprite.data(), tw, th);
  if (tex_id == 0) {
    fprintf(stderr, "renderer_golden_tests: texture upload failed\n");
    return false;
  }

  static DrawCommandList list;
  list.reset();

  // (1) Plain stretched textured rect (no nine-slice, no radius), full tint.
  {
    DrawCommand c{};
    c.kind = DrawCommandKind::Image;
    c.rect = {8, 8, 48, 32};
    c.payload.image.texture_id = tex_id;
    c.payload.image.tint = {255, 255, 255, 255};
    list.push(c);
  }
  // (2) Nine-slice: corners stay 1:1, edges/center stretch (3px slice insets).
  {
    DrawCommand c{};
    c.kind = DrawCommandKind::Image;
    c.rect = {64, 8, 56, 40};
    c.payload.image.texture_id = tex_id;
    c.payload.image.tint = {255, 255, 255, 255};
    c.payload.image.nine_slice = {3.f, 3.f, 3.f, 3.f};
    list.push(c);
  }
  // (3) Rounded textured rect (radius>0.5, no nine-slice) with a tint applied —
  // proves the tessellated UV path + premultiplied tint fold.
  {
    DrawCommand c{};
    c.kind = DrawCommandKind::Image;
    c.rect = {8, 52, 60, 36};
    c.payload.image.texture_id = tex_id;
    c.payload.image.tint = {255, 200, 120, 255}; // warm tint
    c.payload.image.corner_radius = 10.f;
    list.push(c);
  }

  if (list.error_count != 0) {
    fprintf(stderr, "renderer_golden_tests: image scene overflowed\n");
    return false;
  }

  auto draw = [&](SDL_Renderer *r) {
    renderer::execute_draw_commands(r, list, /*fonts=*/nullptr, &textures);
  };

  ui_test::CompareReport rep;
  const bool ok = ui_test::render_and_compare(
      ctx, 128, 96, draw, "tests/fixtures/golden/new_ir_image.bmp", kTolerance,
      &rep);
  if (!ok)
    fprintf(stderr, "renderer_golden_tests (image): %s\n", rep.message.c_str());
  return ok;
}

} // namespace

int main() {
  if (!run_scene() || !run_image_scene()) {
    fprintf(stderr, "renderer_golden_tests: FAIL\n");
    return 1;
  }
  printf("renderer_golden_tests: OK\n");
  return 0;
}
