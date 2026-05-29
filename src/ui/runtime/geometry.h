#pragma once

// geometry.h — SDL-FREE tessellation math for the retained-UI renderer.
//
// This module turns resolved paint (VisualStyle pieces: fills, frames,
// gradients, shadows) into triangle meshes that the SDL backend can hand to
// SDL_RenderGeometry. It contains ZERO SDL types and ZERO game vocabulary so
// the math stays hermetically unit-testable (see tests/ui_geometry_tests.cpp).
//
// Topology follows docs/retained-ui/styling-render-system-design.md §9.3–§9.8:
//   - rounded fill = center quad + 4 corner fans + 4 edge quads (§9.3),
//   - frame = per-side border bands + signed-offset outline ring, same arc
//     tessellation, 45° radial corner color split (§9.4, §9.11),
//   - linear gradient = same fill tessellation, per-vertex color projected onto
//     the gradient axis, interpolated in premultiplied space (§9.6),
//   - drop shadow = solid inner quad + blur-wide skirt fading to (0,0,0,0)
//     (§9.8).
//
// Colors are PREMULTIPLIED at this layer (per the IR boundary, design §8.4):
// callers pass already-premultiplied Color and (0,0,0,0) is the only
// transparent. We never re-multiply.
//
// Output convention: the caller owns fixed-capacity buffers wrapped in a
// MeshSink. Tessellators APPEND to those buffers and return false on overflow
// (no truncation, no clamp, no exceptions, no allocation). On a false return
// the sink may be partially populated — the caller treats the whole frame as
// failed (design §1.4 "overflow is failure, never silent").

#include "../style/visual_style.h" // ui::Color, Vec2, SideWidths/Colors, Border, Outline, Gradient, Shadow

#include <stdint.h>

namespace ui {

// Axis-aligned rectangle in UI points. Top-left origin, +y down.
struct DrawRect {
  float x = 0.f;
  float y = 0.f;
  float w = 0.f;
  float h = 0.f;
};

// One output vertex. position in UI points, premultiplied color, uv in [0,1]
// (only meaningful for textured paths; fills/frames/shadows leave it 0).
struct Vertex {
  float x = 0.f;
  float y = 0.f;
  Color color{}; // PREMULTIPLIED
  float u = 0.f;
  float v = 0.f;
};

// Caller-owned, fixed-capacity append target. The tessellators only ever push
// through quad()/tri(); they never touch the raw arrays directly. Both helpers
// return false (and append NOTHING) if the requested vertices/indices would not
// fit — so a false return leaves the counts on a triangle boundary.
struct MeshSink {
  Vertex *verts = nullptr;
  int vcap = 0;
  int vcount = 0;
  uint16_t *idx = nullptr;
  int icap = 0;
  int icount = 0;

  // Append a quad as two triangles (a,b,c) + (a,c,d). 4 verts, 6 indices.
  // Winding is consistent across the module (matters for batching, not culling:
  // SDL_RenderGeometry has no backface cull).
  bool quad(const Vertex &a, const Vertex &b, const Vertex &c, const Vertex &d);
  // Append a single triangle (a,b,c). 3 verts, 3 indices.
  bool tri(const Vertex &a, const Vertex &b, const Vertex &c);
};

// Segments per 90° corner arc for a given radius (design §9.3):
//   max(16, ceil(radius * 0.5)), capped at 64.
int corner_segments(float radius);

// Solid (or single-color) rounded-rect fill (design §9.3).
//   corner_radius <= 0.5 => a single quad (2 triangles).
//   otherwise => center quad + 4 corner fans + 4 edge quads.
// Every emitted vertex carries `fill` (already premultiplied).
bool tessellate_rect_fill(const DrawRect &rect, float corner_radius, Color fill,
                          MeshSink &sink);

// Fused frame: per-side border bands + signed-offset outline ring (design
// §9.4, §9.11). Border bands are inset from the border-box edge by each side's
// width; corners use the same seg-tessellated arc, split radially between
// adjacent side colors at the 45° bisector. The outline ring is the border-box
// grown by `outline.offset` on every side (negative grows inward), a
// `outline.width`-thick band with corner radius corner_radius+offset (clamped
// at 0). A side/ring is emitted only when its width>0 and color.a>0.
bool tessellate_frame(const DrawRect &rect, float corner_radius,
                      const Border &border, const Outline &outline,
                      MeshSink &sink);

// Linear gradient fill (design §9.6): same tessellation as tessellate_rect_fill,
// but each vertex's color is computed by projecting the vertex onto the gradient
// axis d=(cos(angle_deg), sin(angle_deg)), normalizing to t in [0,1] over the
// rect's projected extent, then lerping the bracketing stops IN PREMULTIPLIED
// SPACE. stop_count==0 emits nothing and returns true (no work, not an error).
bool gradient_fill_colors(const DrawRect &rect, float corner_radius,
                          const Gradient &gradient, MeshSink &sink);

// Drop shadow (design §9.8): the border-box translated by shadow.offset and
// expanded by shadow.spread on every side, drawn as a solid inner quad at full
// `shadow.color` surrounded by a `shadow.blur`-wide skirt fading to
// premultiplied-transparent (0,0,0,0). When corner_radius>0 the skirt corners
// reuse the seg-arc tessellation. color.a==0 emits nothing and returns true.
bool tessellate_shadow(const DrawRect &rect, float corner_radius,
                       const Shadow &shadow, MeshSink &sink);

} // namespace ui
