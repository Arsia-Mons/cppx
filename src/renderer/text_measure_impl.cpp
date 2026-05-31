#include "text_measure_impl.h"

#include "font_registry.h"
#include "ui/style/text_measure.h"

#include <SDL3_ttf/SDL_ttf.h>

#include <string.h>

namespace renderer {

namespace {

// The FontRegistry the measurer reads. Set once by install_text_measurer; the
// measurer is a free function (the seam is a plain function pointer), so the
// registry is reached through this file-static. App owns the registry for the
// whole process, so this never dangles.
FontRegistry *g_fonts = nullptr;

// Resolve the TTF_Font for a query. v1 has one face (default_font); the size is
// applied per-query via TTF_SetFontSize, matching the executor's text path so
// measure == paint. Returns null if no font is available.
TTF_Font *font_for(const ::ui::TextMetricsQuery &q) {
  if (!g_fonts)
    return nullptr;
  TTF_Font *font = g_fonts->default_font();
  if (!font)
    return nullptr;
  if (q.font_size > 0)
    TTF_SetFontSize(font, static_cast<float>(q.font_size));
  return font;
}

float line_box_height(TTF_Font *font, const ::ui::TextMetricsQuery &q) {
  if (q.line_height > 0.0f)
    return q.line_height;
  int skip = TTF_GetFontLineSkip(font);
  return skip > 0 ? static_cast<float>(skip) : 16.0f;
}

float advance_of(TTF_Font *font, const char *utf8, uint32_t len) {
  if (len == 0)
    return 0.0f;
  int w = 0, h = 0;
  if (!TTF_GetStringSize(font, utf8, len, &w, &h))
    return 0.0f;
  return static_cast<float>(w);
}

// Bake alignment x relative to the content box. With no box width (wrap_width<=0
// in non-wrap mode the transcriber still passes the laid-out content width), a
// non-positive box width falls back to left alignment (x=0).
float aligned_x(::ui::TextAlign align, float line_w, float box_w) {
  if (box_w <= 0.0f)
    return 0.0f;
  switch (align) {
  case ::ui::TextAlign::Center:
    return (box_w - line_w) * 0.5f;
  case ::ui::TextAlign::Right:
    return box_w - line_w;
  case ::ui::TextAlign::Left:
  default:
    return 0.0f;
  }
}

// Append a line run; returns false if the line cap is already full.
bool push_line(::ui::TextMetricsResult &out, uint32_t slice_off,
               uint32_t slice_len, float x, float y, float w, float h) {
  if (out.line_count >= ::ui::UI_MAX_TEXT_LINES)
    return false;
  ::ui::LineRun &run = out.lines[out.line_count++];
  run.slice_offset = slice_off;
  run.slice_len = slice_len;
  run.x = x;
  run.y = y;
  run.w = w;
  run.h = h;
  if (w > out.width)
    out.width = w;
  return true;
}

// Word-wrap utf8 greedily at wrap_width. Breaks at ASCII spaces (the existing
// fonts/fixtures are space-separated); a single word wider than the box occupies
// its own line (no mid-word splitting in v1). Trailing spaces are folded into
// the line break (not rendered, not measured into advance).
::ui::TextMetricsResult measure_wrapped(TTF_Font *font,
                                        const ::ui::TextMetricsQuery &q,
                                        float line_h, float box_w) {
  ::ui::TextMetricsResult out = {};
  const char *s = q.utf8;
  const uint32_t n = q.len;
  uint32_t line_start = 0;
  float y = 0.0f;

  uint32_t i = 0;
  while (line_start < n) {
    // Find the longest [line_start, break_end) that fits box_w, breaking on the
    // last space that still fits; if no space fits, take at least one word.
    uint32_t last_fit_end = line_start;   // end of content that fits (excl trailing space)
    uint32_t scan = line_start;
    uint32_t next_line_start = n;          // where the following line begins
    bool placed = false;
    while (scan <= n) {
      // Identify the next word boundary: advance to end of current word.
      uint32_t word_start = scan;
      while (word_start < n && s[word_start] == ' ')
        ++word_start;
      uint32_t word_end = word_start;
      while (word_end < n && s[word_end] != ' ')
        ++word_end;
      if (word_end == word_start) {
        // No more words.
        break;
      }
      float w = advance_of(font, s + line_start, word_end - line_start);
      if (w <= box_w || !placed) {
        last_fit_end = word_end;
        placed = true;
        scan = word_end;
        if (word_end >= n) {
          next_line_start = n;
          break;
        }
      } else {
        // This word overflows; the line ends at last_fit_end. The next line
        // starts at the first non-space at or after last_fit_end.
        next_line_start = last_fit_end;
        while (next_line_start < n && s[next_line_start] == ' ')
          ++next_line_start;
        break;
      }
      // Skip following spaces to find the next word's start for the next loop.
      if (scan >= n) {
        next_line_start = n;
        break;
      }
    }
    if (!placed) {
      // Only whitespace remained; stop.
      break;
    }
    uint32_t slice_len = last_fit_end - line_start;
    float line_w = advance_of(font, s + line_start, slice_len);
    float x = aligned_x(q.align, line_w, box_w);
    if (!push_line(out, line_start, slice_len, x, y, line_w, line_h)) {
      out.overflowed = true;
      break;
    }
    y += line_h;
    line_start = next_line_start;
    ++i;
    if (i > n) // safety against any pathological non-advance
      break;
  }
  out.height = static_cast<float>(out.line_count) * line_h;
  return out;
}

::ui::TextMetricsResult measure(const ::ui::TextMetricsQuery &q) {
  ::ui::TextMetricsResult out = {};
  TTF_Font *font = font_for(q);
  const char *utf8 = q.utf8 ? q.utf8 : "";
  uint32_t len = q.len;
  if (!font) {
    // No font: report zero-width single line at the natural-ish height so layout
    // does not collapse to nothing.
    float h = q.line_height > 0.0f ? q.line_height : 16.0f;
    push_line(out, 0, len, 0.0f, 0.0f, 0.0f, h);
    out.height = h;
    return out;
  }
  float line_h = line_box_height(font, q);
  float box_w = q.wrap_width;

  if (q.wrap == ::ui::TextWrap::Words && box_w > 0.0f && len > 0) {
    return measure_wrapped(font, q, line_h, box_w);
  }

  // Single line.
  float w = advance_of(font, utf8, len);
  float x = aligned_x(q.align, w, box_w);
  push_line(out, 0, len, x, 0.0f, w, line_h);
  out.height = line_h;
  return out;
}

} // namespace

void install_text_measurer(FontRegistry *fonts) {
  g_fonts = fonts;
  ::ui::set_text_measurer(fonts ? &measure : nullptr);
}

} // namespace renderer
