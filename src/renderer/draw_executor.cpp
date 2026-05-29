#include "draw_executor.h"

#include "font_registry.h"
#include "texture_registry.h"
#include "ui/runtime/geometry.h"

#include <math.h>
#include <string.h>

namespace renderer {

namespace {

// Per-frame scratch for one command's mesh + its SDL conversion. Single static
// instance (UI is single-threaded); sized for the worst single-command mesh
// (a full ring band ~ a few thousand verts).
constexpr int kVScratch = 8192;
constexpr int kIScratch = 16384;
struct Scratch {
  ::ui::Vertex v[kVScratch];
  uint16_t i[kIScratch];
  SDL_Vertex sv[kVScratch];
  int si[kIScratch];
};

void submit(SDL_Renderer *r, const ::ui::MeshSink &sink, Scratch &s) {
  if (sink.vcount <= 0 || sink.icount <= 0)
    return;
  for (int k = 0; k < sink.vcount; ++k) {
    const ::ui::Vertex &v = s.v[k];
    s.sv[k].position = {v.x, v.y};
    s.sv[k].color = {v.color.r / 255.f, v.color.g / 255.f, v.color.b / 255.f,
                     v.color.a / 255.f}; // premultiplied
    s.sv[k].tex_coord = {v.u, v.v};
  }
  for (int k = 0; k < sink.icount; ++k)
    s.si[k] = static_cast<int>(s.i[k]);
  SDL_RenderGeometry(r, nullptr, s.sv, sink.vcount, s.si, sink.icount);
}

SDL_Rect round_out(const ::ui::DrawRect &r) {
  const int x0 = static_cast<int>(floorf(r.x));
  const int y0 = static_cast<int>(floorf(r.y));
  const int x1 = static_cast<int>(ceilf(r.x + r.w));
  const int y1 = static_cast<int>(ceilf(r.y + r.h));
  return {x0, y0, x1 - x0, y1 - y0};
}

void render_text(SDL_Renderer *r, const ::ui::DrawCommandList &list,
                 const ::ui::DrawCommand &c, FontRegistry *fonts) {
  if (!fonts || !fonts->default_font())
    return;
  const ::ui::TextData &t = c.payload.text;
  if (t.text_len == 0)
    return;
  char buf[256];
  size_t n = t.text_len < 255 ? t.text_len : 255;
  memcpy(buf, &list.text_arena[t.text_off], n);
  buf[n] = '\0';

  TTF_Font *font = fonts->default_font();
  if (t.font_size > 0)
    TTF_SetFontSize(font, static_cast<float>(t.font_size));
  SDL_Color col = {t.color.r, t.color.g, t.color.b, t.color.a};
  SDL_Surface *surface = TTF_RenderText_Blended(font, buf, n, col);
  if (!surface)
    return;
  SDL_Texture *texture = SDL_CreateTextureFromSurface(r, surface);
  if (texture) {
    SDL_FRect dst = {c.rect.x, c.rect.y, static_cast<float>(surface->w),
                     static_cast<float>(surface->h)};
    SDL_RenderTexture(r, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
  }
  SDL_DestroySurface(surface);
}

// Draw a textured Image command (design §9.7). Three paths:
//   - nine-slice (any nine_slice width > 0): 9 sub-rects, corners 1:1, edges
//     stretched along one axis, center stretched both. Radius ignored (cut).
//   - rounded (corner_radius > 0.5, no nine-slice): §9.3 fill tessellation with
//     texture + per-vertex UVs; tint folded into the per-vertex (premultiplied)
//     color.
//   - plain: one SDL_RenderTexture with color/alpha mod from tint, restored to
//     white immediately after so the cached texture is left unmodulated.
// `tint` is premultiplied (transcriber's emit boundary). SDL color/alpha mod
// multiplies the sampled (already-premultiplied) texel — so the premultiplied
// tint multiplies straight through, which is the correct premultiplied tint.
void render_image(SDL_Renderer *r, const ::ui::DrawCommand &c,
                  TextureRegistry *textures, Scratch &s) {
  if (!textures)
    return;
  const ::ui::ImageData &img = c.payload.image;
  SDL_Texture *tex = textures->lookup(img.texture_id);
  if (!tex)
    return;

  float tw = 0.f, th = 0.f;
  SDL_GetTextureSize(tex, &tw, &th);
  if (tw <= 0.f || th <= 0.f)
    return;

  const ::ui::Color tint = img.tint;
  const ::ui::SideWidths &ns = img.nine_slice;
  const bool nine = ns.top > 0.f || ns.right > 0.f || ns.bottom > 0.f ||
                    ns.left > 0.f;

  if (nine) {
    // 9-patch: source insets in texture space, dest insets in dest space.
    // Corners 1:1 (source inset == dest inset); edges/center stretch.
    SDL_SetTextureColorMod(tex, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(tex, tint.a);

    const float sl = ns.left, sr = ns.right, st = ns.top, sb = ns.bottom;
    const float dx0 = c.rect.x, dy0 = c.rect.y;
    const float dx1 = c.rect.x + c.rect.w, dy1 = c.rect.y + c.rect.h;

    // Column x-edges (src then dst): [0, left, w-right, w].
    const float sx[4] = {0.f, sl, tw - sr, tw};
    const float sy[4] = {0.f, st, th - sb, th};
    const float dx[4] = {dx0, dx0 + sl, dx1 - sr, dx1};
    const float dy[4] = {dy0, dy0 + st, dy1 - sb, dy1};

    for (int row = 0; row < 3; ++row) {
      for (int col = 0; col < 3; ++col) {
        SDL_FRect src = {sx[col], sy[row], sx[col + 1] - sx[col],
                         sy[row + 1] - sy[row]};
        SDL_FRect dst = {dx[col], dy[row], dx[col + 1] - dx[col],
                         dy[row + 1] - dy[row]};
        if (src.w <= 0.f || src.h <= 0.f || dst.w <= 0.f || dst.h <= 0.f)
          continue;
        SDL_RenderTexture(r, tex, &src, &dst);
      }
    }

    SDL_SetTextureColorMod(tex, 255, 255, 255);
    SDL_SetTextureAlphaMod(tex, 255);
    return;
  }

  if (img.corner_radius > 0.5f) {
    // Rounded textured rect: §9.3 tessellation, texture + per-vertex UVs, tint
    // folded into per-vertex premultiplied color. The geometry module emits uv
    // 0; we re-derive uv from each vertex's position within the rect.
    ::ui::MeshSink sink{s.v, kVScratch, 0, s.i, kIScratch, 0};
    if (!::ui::tessellate_rect_fill(c.rect, img.corner_radius, tint, sink))
      return;
    const float rx = c.rect.x, ry = c.rect.y;
    const float rw = c.rect.w > 0.f ? c.rect.w : 1.f;
    const float rh = c.rect.h > 0.f ? c.rect.h : 1.f;
    for (int k = 0; k < sink.vcount; ++k) {
      const ::ui::Vertex &v = s.v[k];
      s.sv[k].position = {v.x, v.y};
      s.sv[k].color = {v.color.r / 255.f, v.color.g / 255.f, v.color.b / 255.f,
                       v.color.a / 255.f}; // tint, premultiplied
      s.sv[k].tex_coord = {(v.x - rx) / rw, (v.y - ry) / rh};
    }
    for (int k = 0; k < sink.icount; ++k)
      s.si[k] = static_cast<int>(s.i[k]);
    SDL_RenderGeometry(r, tex, s.sv, sink.vcount, s.si, sink.icount);
    return;
  }

  // Plain stretched textured rect.
  SDL_SetTextureColorMod(tex, tint.r, tint.g, tint.b);
  SDL_SetTextureAlphaMod(tex, tint.a);
  SDL_FRect dst = {c.rect.x, c.rect.y, c.rect.w, c.rect.h};
  SDL_RenderTexture(r, tex, nullptr, &dst);
  SDL_SetTextureColorMod(tex, 255, 255, 255);
  SDL_SetTextureAlphaMod(tex, 255);
}

} // namespace

void execute_draw_commands(SDL_Renderer *renderer,
                           const ::ui::DrawCommandList &list,
                           FontRegistry *fonts, TextureRegistry *textures) {
  if (!renderer)
    return;
  static Scratch scratch;
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND_PREMULTIPLIED);

  SDL_Rect clip_stack[16];
  int clip_depth = 0;

  for (int ci = 0; ci < list.count; ++ci) {
    const ::ui::DrawCommand &c = list.commands[ci];
    ::ui::MeshSink sink{scratch.v, kVScratch, 0, scratch.i, kIScratch, 0};
    switch (c.kind) {
    case ::ui::DrawCommandKind::Rect:
      if (c.payload.rect.fill.a > 0) {
        ::ui::tessellate_rect_fill(c.rect, c.payload.rect.corner_radius,
                                   c.payload.rect.fill, sink);
        submit(renderer, sink, scratch);
      }
      break;
    case ::ui::DrawCommandKind::Gradient: {
      const ::ui::GradientData &gd = c.payload.gradient;
      ::ui::Gradient g{};
      g.angle_deg = gd.angle_deg;
      g.stop_count = gd.stop_count;
      for (int k = 0; k < gd.stop_count && k < ::ui::UI_MAX_GRADIENT_STOPS; ++k)
        g.stops[k] = list.grad_arena[gd.stop_off + k];
      ::ui::gradient_fill_colors(c.rect, gd.corner_radius, g, sink);
      submit(renderer, sink, scratch);
      break;
    }
    case ::ui::DrawCommandKind::Border:
      ::ui::tessellate_frame(c.rect, c.payload.border.corner_radius,
                             c.payload.border.border, c.payload.border.outline,
                             sink);
      submit(renderer, sink, scratch);
      break;
    case ::ui::DrawCommandKind::Shadow: {
      const ::ui::ShadowData &sd = c.payload.shadow;
      if (sd.color.a > 0) {
        ::ui::Shadow shadow{};
        shadow.color = sd.color;   // premultiplied at emit
        shadow.offset = sd.offset;
        shadow.blur = sd.blur;
        shadow.spread = sd.spread;
        ::ui::tessellate_shadow(c.rect, sd.corner_radius, shadow, sink);
        submit(renderer, sink, scratch);
      }
      break;
    }
    case ::ui::DrawCommandKind::Image:
      render_image(renderer, c, textures, scratch);
      break;
    case ::ui::DrawCommandKind::Text:
      render_text(renderer, list, c, fonts);
      break;
    case ::ui::DrawCommandKind::ClipPush: {
      SDL_Rect cr = round_out(c.rect);
      if (clip_depth > 0)
        SDL_GetRectIntersection(&clip_stack[clip_depth - 1], &cr, &cr);
      if (clip_depth < 16)
        clip_stack[clip_depth++] = cr;
      SDL_SetRenderClipRect(renderer, &cr);
      break;
    }
    case ::ui::DrawCommandKind::ClipPop:
      if (clip_depth > 0)
        --clip_depth;
      SDL_SetRenderClipRect(renderer,
                            clip_depth > 0 ? &clip_stack[clip_depth - 1] : nullptr);
      break;
    default:
      // LayerPush / LayerPop / Custom: wired in P6.
      break;
    }
  }
  SDL_SetRenderClipRect(renderer, nullptr);
}

} // namespace renderer
