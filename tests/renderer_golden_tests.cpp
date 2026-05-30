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

// ---------------------------------------------------------------------------
// Scene 3: group opacity (P6, design §9.10). Two OVERLAPPING opaque rects sit
// inside a LayerPush(opacity=0.5)/LayerPop bracket. The correct composite
// flattens the children ONCE inside the layer (so the overlap region is exactly
// the rect color, not double-painted) and then fades the WHOLE group by 0.5.
//
// The adversarial point: if the executor ignored the layer and drew children
// straight onto the background, both rects would land at full opacity and the
// overlap would be indistinguishable from the non-overlap parts (both fully
// saturated). With the layer, every covered pixel — overlap and non-overlap
// alike — reads back at ~half the rect color over the black clear. The pixel
// probe asserts (a) the group is faded (covered pixels are ~half intensity, not
// full) and (b) the overlap is NOT darker than the non-overlap (single blend).
// ---------------------------------------------------------------------------
DrawCommandList &opacity_scene() {
  static DrawCommandList list;
  list.reset();

  // Group-opacity layer over the whole composited subtree. Header rect is the
  // group's border-box; the executor sizes the transient to the full output so
  // children draw at absolute coords (this golden does not exercise box-clip).
  {
    DrawCommand c{};
    c.kind = DrawCommandKind::LayerPush;
    c.rect = {8, 8, 96, 72}; // group box (informational; executor sizes to output)
    c.payload.layer.opacity = 0.5f;
    list.push(c);
  }
  // Child A: opaque square (premultiplied == straight, a==255).
  {
    DrawCommand c{};
    c.kind = DrawCommandKind::Rect;
    c.rect = {16, 16, 48, 48};
    c.payload.rect.fill = {200, 80, 40, 255};
    c.payload.rect.corner_radius = 0.f;
    list.push(c);
  }
  // Child B: opaque square overlapping A's lower-right quadrant, SAME color so
  // a double-blend (if it happened) would still be detectable as a saturation
  // change at the overlap — but with correct single-flatten it is identical.
  {
    DrawCommand c{};
    c.kind = DrawCommandKind::Rect;
    c.rect = {40, 40, 48, 48};
    c.payload.rect.fill = {200, 80, 40, 255};
    c.payload.rect.corner_radius = 0.f;
    list.push(c);
  }
  {
    DrawCommand c{};
    c.kind = DrawCommandKind::LayerPop;
    list.push(c);
  }
  return list;
}

bool run_opacity_scene() {
  ui_test::GoldenContext ctx;
  if (!ctx.init()) {
    fprintf(stderr, "renderer_golden_tests: GoldenContext init failed\n");
    return false;
  }
  const DrawCommandList &list = opacity_scene();
  if (list.error_count != 0) {
    fprintf(stderr, "renderer_golden_tests: opacity scene overflowed\n");
    return false;
  }

  auto draw = [&](SDL_Renderer *r) {
    renderer::execute_draw_commands(r, list, /*fonts=*/nullptr);
  };

  // Independent of the golden bytes: prove group opacity actually composited.
  {
    ui_test::Image probe;
    if (!ctx.render_to_image(128, 96, draw, &probe)) {
      fprintf(stderr, "renderer_golden_tests (opacity): probe render failed\n");
      return false;
    }
    // Sample three regions of the fill color (200,80,40):
    //   only-A   : center of A's non-overlap area      (~24,24)
    //   only-B   : center of B's non-overlap area      (~80,80)
    //   overlap  : where A and B overlap (40,40)-(64,64) (~52,52)
    auto avg = [&](int x0, int y0, int x1, int y1, double out[3]) {
      double s[3] = {0, 0, 0};
      int n = 0;
      for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) {
          uint8_t p[4];
          probe.at(x, y, p);
          s[0] += p[0]; s[1] += p[1]; s[2] += p[2];
          ++n;
        }
      out[0] = s[0] / n; out[1] = s[1] / n; out[2] = s[2] / n;
    };
    double only_a[3], only_b[3], overlap[3];
    avg(20, 20, 30, 30, only_a);
    avg(74, 74, 84, 84, only_b);
    avg(48, 48, 58, 58, overlap);

    // (a) Faded: the red channel of covered pixels must be near 100 (=200*0.5),
    // NOT near 200. A wide window [70,130] absorbs rounding while still failing
    // hard if the layer were ignored (would be ~200) or fully transparent (~0).
    if (only_a[0] < 70.0 || only_a[0] > 130.0) {
      fprintf(stderr,
              "renderer_golden_tests (opacity): group not faded — only-A red "
              "%.1f (expected ~100; ~200 means the layer was ignored)\n",
              only_a[0]);
      return false;
    }
    // (b) No double-darkening: the overlap must match the non-overlap regions
    // within a tight tolerance on every channel. A double-blend would push the
    // overlap measurably away from the singly-blended regions.
    for (int ch = 0; ch < 3; ++ch) {
      double d_a = overlap[ch] - only_a[ch];
      double d_b = overlap[ch] - only_b[ch];
      if (d_a < 0) d_a = -d_a;
      if (d_b < 0) d_b = -d_b;
      if (d_a > 3.0 || d_b > 3.0) {
        fprintf(stderr,
                "renderer_golden_tests (opacity): overlap channel %d differs "
                "from non-overlap (overlap %.1f, only-A %.1f, only-B %.1f) — "
                "children double-blended inside the group\n",
                ch, overlap[ch], only_a[ch], only_b[ch]);
        return false;
      }
    }
  }

  ui_test::CompareReport rep;
  const bool ok = ui_test::render_and_compare(
      ctx, 128, 96, draw, "tests/fixtures/golden/new_ir_opacity.bmp", kTolerance,
      &rep);
  if (!ok)
    fprintf(stderr, "renderer_golden_tests (opacity): %s\n", rep.message.c_str());
  return ok;
}

// ---------------------------------------------------------------------------
// Scene 4: HiDPI device scale. Renders scene 1 at scale=2 into a 2x target
// (256x192). The headless tests always run at density 1, so this is the only
// coverage of the executor's point->device-pixel scaling. Two checks:
//   (a) semantic probe — the blue rounded fill (points {8,8,48,32}) reaches
//       device px ~(100,48), which is OUTSIDE the unscaled fill (right edge px
//       56) and only lit when geometry is scaled 2x; a far point stays bg.
//   (b) golden lock — pin the scaled raster so scaling regressions are caught.
// ---------------------------------------------------------------------------
bool run_scaled_scene() {
  ui_test::GoldenContext ctx;
  if (!ctx.init()) {
    fprintf(stderr, "renderer_golden_tests: GoldenContext init failed\n");
    return false;
  }
  const DrawCommandList &list = scene();
  if (list.error_count != 0) {
    fprintf(stderr, "renderer_golden_tests: scaled scene overflowed\n");
    return false;
  }
  auto draw = [&](SDL_Renderer *r) {
    renderer::execute_draw_commands(r, list, /*fonts=*/nullptr,
                                    /*textures=*/nullptr, /*scale=*/2.0f);
  };

  {
    ui_test::Image probe;
    if (!ctx.render_to_image(256, 192, draw, &probe)) {
      fprintf(stderr, "renderer_golden_tests (scaled): probe render failed\n");
      return false;
    }
    // (a) device px (100,48): inside the 2x-scaled blue fill (x in [16,112]),
    // but past the unscaled fill's right edge (56) — only blue if truly scaled.
    uint8_t in[4];
    probe.at(100, 48, in);
    const bool blue = in[2] > 150 && in[0] < 110 && in[1] > 80 && in[1] < 170;
    if (!blue) {
      fprintf(stderr,
              "renderer_golden_tests (scaled): expected scaled blue fill at "
              "device px (100,48), got rgba(%d,%d,%d,%d) — geometry not scaled\n",
              in[0], in[1], in[2], in[3]);
      return false;
    }
    // far empty region stays background (between the top shapes and gradient).
    uint8_t bg[4];
    probe.at(130, 94, bg);
    if (bg[0] > 20 || bg[1] > 20 || bg[2] > 20) {
      fprintf(stderr,
              "renderer_golden_tests (scaled): expected background at device px "
              "(130,94), got rgba(%d,%d,%d,%d)\n",
              bg[0], bg[1], bg[2], bg[3]);
      return false;
    }
  }

  ui_test::CompareReport rep;
  const bool ok = ui_test::render_and_compare(
      ctx, 256, 192, draw, "tests/fixtures/golden/new_ir_scaled.bmp", kTolerance,
      &rep);
  if (!ok)
    fprintf(stderr, "renderer_golden_tests (scaled): %s\n", rep.message.c_str());
  return ok;
}

} // namespace

int main() {
  if (!run_scene() || !run_image_scene() || !run_opacity_scene() ||
      !run_scaled_scene()) {
    fprintf(stderr, "renderer_golden_tests: FAIL\n");
    return 1;
  }
  printf("renderer_golden_tests: OK\n");
  return 0;
}
