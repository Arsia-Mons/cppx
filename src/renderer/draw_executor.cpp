#include "draw_executor.h"

#include "font_registry.h"
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

} // namespace

void execute_draw_commands(SDL_Renderer *renderer,
                           const ::ui::DrawCommandList &list,
                           FontRegistry *fonts) {
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
      // Image / Shadow / LayerPush / LayerPop / Custom: wired in P5/P6.
      break;
    }
  }
  SDL_SetRenderClipRect(renderer, nullptr);
}

} // namespace renderer
