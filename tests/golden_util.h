#pragma once

// golden_util.h — self-contained golden-image test harness for the SDL-free
// geometry/tessellation module (src/ui/runtime/geometry.h).
//
// The geometry module itself never touches SDL: it appends `ui::Vertex` /
// index data into a caller-owned `ui::MeshSink`. This harness is the *bridge*
// that turns that hermetic mesh into pixels so we can assert on rendered
// output. It:
//
//   1. Boots SDL in the same headless configuration the Python CLI smoke tests
//      use (SDL_VIDEODRIVER=dummy, SDL_RENDER_DRIVER=software — see
//      tests/ui_cli_smoke.py and src/platform/sdl/window.cpp). A renderer is
//      created exactly like platform::sdl::Window does:
//          SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE)
//          SDL_CreateRenderer(window, nullptr)
//      with the software driver selected by the hint above.
//
//   2. Renders a caller-provided draw routine into an offscreen
//      SDL_TEXTUREACCESS_TARGET texture and reads the pixels back via
//      SDL_RenderReadPixels (the same readback path control_mailbox.cpp uses
//      for screenshots), normalizing to tightly-packed 32-bit RGBA.
//
//   3. Compares against a golden BMP with a per-channel tolerance, writing
//      <name>_actual.bmp and <name>_diff.bmp on mismatch, and regenerating the
//      golden in place when the environment variable UI_GOLDEN_REGEN=1 is set.
//
// BMP I/O is hand-rolled (24/32-bit BI_RGB) so the harness pulls in no deps
// beyond SDL3 — no SDL3_image. Conventions: C++20, no exceptions, no RTTI,
// builds clean under -Wall -Wextra.
//
// Pixels in memory are always RGBA8888, 4 bytes/pixel, top row first, NO row
// padding. Colors are STRAIGHT (non-premultiplied) at the pixel level — the
// geometry module emits premultiplied vertex colors, but the software renderer
// composites onto an opaque-cleared target, so what we read back is ordinary
// straight RGBA suitable for byte-wise comparison.

#include <SDL3/SDL.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace ui_test {

// ---------------------------------------------------------------------------
// Image: tightly-packed top-down RGBA8888.
// ---------------------------------------------------------------------------
struct Image {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> rgba; // size == width*height*4, row-major, no padding

  bool valid() const {
    return width > 0 && height > 0 &&
           rgba.size() == static_cast<size_t>(width) * height * 4u;
  }
  size_t byte_size() const {
    return static_cast<size_t>(width) * height * 4u;
  }
  // Direct pixel accessor; out-of-range returns transparent black.
  void at(int x, int y, uint8_t out[4]) const {
    if (x < 0 || y < 0 || x >= width || y >= height) {
      out[0] = out[1] = out[2] = out[3] = 0;
      return;
    }
    const uint8_t *p = &rgba[(static_cast<size_t>(y) * width + x) * 4u];
    out[0] = p[0];
    out[1] = p[1];
    out[2] = p[2];
    out[3] = p[3];
  }
};

// ---------------------------------------------------------------------------
// Comparison report (populated by compare_bmp).
// ---------------------------------------------------------------------------
struct CompareReport {
  bool size_mismatch = false;
  bool missing_golden = false;
  bool regenerated = false;     // golden (re)written because UI_GOLDEN_REGEN=1
  int golden_width = 0;
  int golden_height = 0;
  int diff_pixels = 0;          // pixels exceeding tolerance on any channel
  int max_channel_delta = 0;    // largest abs per-channel delta observed
  int first_bad_x = -1;         // first failing pixel (top-down scan)
  int first_bad_y = -1;
  std::string actual_path;      // written on failure
  std::string diff_path;        // written on failure
  std::string message;          // human-readable summary
};

// ===========================================================================
// SDL headless setup
// ===========================================================================
//
// Mirrors tests/ui_cli_smoke.py (--video-driver dummy --render-driver software)
// and the create pattern in src/platform/sdl/window.cpp. The hints must be set
// before SDL_Init / SDL_CreateRenderer to take effect. Idempotent: SDL_Init is
// reference-counted, so multiple GoldenContext instances are safe.
class GoldenContext {
public:
  GoldenContext() = default;
  ~GoldenContext() { shutdown(); }

  GoldenContext(const GoldenContext &) = delete;
  GoldenContext &operator=(const GoldenContext &) = delete;

  // Returns false (with a message on stderr) instead of throwing.
  bool init() {
    // Force the headless software path. SDL_SetHint also honors any value the
    // environment already provides, but we set it explicitly so the harness is
    // self-contained when run outside ctest.
    // SDL_HINT_OVERRIDE so these win even if an env var is already present
    // (plain SDL_SetHint will NOT override an existing env var/override hint).
    SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software", SDL_HINT_OVERRIDE);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
      fprintf(stderr, "golden_util: SDL_Init failed: %s\n", SDL_GetError());
      return false;
    }
    inited_ = true;

    // A real window is created (like platform::sdl::Window) so the renderer is
    // a genuine, fully-featured software renderer; all actual drawing targets
    // an offscreen texture, never the window backbuffer.
    window_ = SDL_CreateWindow("golden_util", 16, 16, SDL_WINDOW_RESIZABLE);
    if (!window_) {
      fprintf(stderr, "golden_util: SDL_CreateWindow failed: %s\n",
              SDL_GetError());
      return false;
    }
    renderer_ = SDL_CreateRenderer(window_, "software");
    if (!renderer_) {
      fprintf(stderr, "golden_util: SDL_CreateRenderer failed: %s\n",
              SDL_GetError());
      return false;
    }
    return true;
  }

  void shutdown() {
    if (renderer_) {
      SDL_DestroyRenderer(renderer_);
      renderer_ = nullptr;
    }
    if (window_) {
      SDL_DestroyWindow(window_);
      window_ = nullptr;
    }
    if (inited_) {
      SDL_Quit();
      inited_ = false;
    }
  }

  SDL_Renderer *renderer() const { return renderer_; }

  // Caller-provided draw routine: receives the active software renderer with an
  // offscreen RGBA target already bound and cleared. Draw whatever you like
  // (typically SDL_RenderGeometry with the geometry module's vertices). The
  // target/present/readback dance is handled by render_to_image.
  using DrawFn = std::function<void(SDL_Renderer *)>;

  // Renders `draw` into a `width`x`height` offscreen target and reads the
  // pixels back as tightly-packed RGBA8888 into `out`. `clear` is the straight
  // RGBA background the target is cleared to before `draw` runs (default opaque
  // black). Returns false on any SDL failure.
  bool render_to_image(int width, int height, const DrawFn &draw, Image *out,
                       uint8_t clear[4] = nullptr) {
    if (!renderer_ || width <= 0 || height <= 0 || !out)
      return false;

    SDL_Texture *target = SDL_CreateTexture(
        renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, width,
        height);
    if (!target) {
      fprintf(stderr, "golden_util: SDL_CreateTexture failed: %s\n",
              SDL_GetError());
      return false;
    }
    // Straight-alpha blending so a premultiplied-color mesh composites
    // correctly over the cleared, opaque target.
    SDL_SetTextureBlendMode(target, SDL_BLENDMODE_BLEND);

    bool ok = true;
    if (!SDL_SetRenderTarget(renderer_, target)) {
      fprintf(stderr, "golden_util: SDL_SetRenderTarget failed: %s\n",
              SDL_GetError());
      ok = false;
    }

    if (ok) {
      uint8_t cr = clear ? clear[0] : 0;
      uint8_t cg = clear ? clear[1] : 0;
      uint8_t cb = clear ? clear[2] : 0;
      uint8_t ca = clear ? clear[3] : 255;
      SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(renderer_, cr, cg, cb, ca);
      SDL_RenderClear(renderer_);

      if (draw)
        draw(renderer_);

      // Do NOT SDL_RenderPresent while a texture target is bound — SDL3 fails
      // that. SDL_RenderReadPixels reads the current render target directly.
      ok = read_target(width, height, out);
    }

    SDL_SetRenderTarget(renderer_, nullptr);
    SDL_DestroyTexture(target);
    return ok;
  }

private:
  // Reads the currently-bound render target back into a packed RGBA image,
  // converting from whatever surface format the software renderer hands us.
  bool read_target(int width, int height, Image *out) {
    SDL_Rect rect{0, 0, width, height};
    SDL_Surface *surface = SDL_RenderReadPixels(renderer_, &rect);
    if (!surface) {
      fprintf(stderr, "golden_util: SDL_RenderReadPixels failed: %s\n",
              SDL_GetError());
      return false;
    }
    SDL_Surface *rgba = surface;
    if (surface->format != SDL_PIXELFORMAT_RGBA32) {
      rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
      SDL_DestroySurface(surface);
      if (!rgba) {
        fprintf(stderr, "golden_util: SDL_ConvertSurface failed: %s\n",
                SDL_GetError());
        return false;
      }
    }

    out->width = width;
    out->height = height;
    out->rgba.assign(static_cast<size_t>(width) * height * 4u, 0);

    const uint8_t *src = static_cast<const uint8_t *>(rgba->pixels);
    const int pitch = rgba->pitch; // bytes per row, may include padding
    for (int y = 0; y < height; ++y) {
      std::memcpy(&out->rgba[(static_cast<size_t>(y) * width) * 4u],
                  src + static_cast<size_t>(y) * pitch,
                  static_cast<size_t>(width) * 4u);
    }
    SDL_DestroySurface(rgba);
    return true;
  }

  bool inited_ = false;
  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
};

// ===========================================================================
// Hand-rolled BMP I/O (BI_RGB, bottom-up, 24- or 32-bit).
// ===========================================================================
//
// We write 32-bit BGRA so alpha round-trips for diff inspection; we read both
// 24- and 32-bit BI_RGB so externally-produced goldens still load. No external
// dependency, no SDL_image. All multi-byte fields are little-endian (BMP spec).

namespace detail {

inline void put_u16(std::vector<uint8_t> &b, uint16_t v) {
  b.push_back(static_cast<uint8_t>(v & 0xFF));
  b.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}
inline void put_u32(std::vector<uint8_t> &b, uint32_t v) {
  b.push_back(static_cast<uint8_t>(v & 0xFF));
  b.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
  b.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
  b.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}
inline void put_i32(std::vector<uint8_t> &b, int32_t v) {
  put_u32(b, static_cast<uint32_t>(v));
}
inline uint16_t get_u16(const uint8_t *p) {
  return static_cast<uint16_t>(p[0] | (p[1] << 8));
}
inline uint32_t get_u32(const uint8_t *p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}
inline int32_t get_i32(const uint8_t *p) {
  return static_cast<int32_t>(get_u32(p));
}

} // namespace detail

// Writes `img` to `path` as a 32-bit BGRA BI_RGB BMP. Returns false on I/O
// failure.
inline bool write_bmp(const std::string &path, const Image &img) {
  if (!img.valid())
    return false;

  const uint32_t row_bytes = static_cast<uint32_t>(img.width) * 4u; // 32bpp
  const uint32_t pixel_bytes = row_bytes * static_cast<uint32_t>(img.height);
  const uint32_t header_bytes = 14u + 40u; // BITMAPFILEHEADER + BITMAPINFOHEADER
  const uint32_t file_bytes = header_bytes + pixel_bytes;

  std::vector<uint8_t> out;
  out.reserve(file_bytes);

  // BITMAPFILEHEADER
  out.push_back('B');
  out.push_back('M');
  detail::put_u32(out, file_bytes);
  detail::put_u16(out, 0);
  detail::put_u16(out, 0);
  detail::put_u32(out, header_bytes); // pixel data offset

  // BITMAPINFOHEADER
  detail::put_u32(out, 40u);
  detail::put_i32(out, img.width);
  detail::put_i32(out, img.height); // positive => bottom-up
  detail::put_u16(out, 1);          // planes
  detail::put_u16(out, 32);         // bpp
  detail::put_u32(out, 0);          // BI_RGB
  detail::put_u32(out, pixel_bytes);
  detail::put_i32(out, 2835); // ~72 DPI
  detail::put_i32(out, 2835);
  detail::put_u32(out, 0);
  detail::put_u32(out, 0);

  // Pixel array: bottom-up rows, BGRA byte order.
  for (int y = img.height - 1; y >= 0; --y) {
    const uint8_t *row = &img.rgba[(static_cast<size_t>(y) * img.width) * 4u];
    for (int x = 0; x < img.width; ++x) {
      const uint8_t *px = row + static_cast<size_t>(x) * 4u;
      out.push_back(px[2]); // B
      out.push_back(px[1]); // G
      out.push_back(px[0]); // R
      out.push_back(px[3]); // A
    }
  }

  FILE *f = std::fopen(path.c_str(), "wb");
  if (!f)
    return false;
  size_t wrote = std::fwrite(out.data(), 1, out.size(), f);
  std::fclose(f);
  return wrote == out.size();
}

// Reads a 24- or 32-bit uncompressed (BI_RGB) BMP into `out` as packed RGBA.
// Returns false if the file is missing, malformed, or an unsupported variant.
inline bool load_bmp(const std::string &path, Image *out) {
  if (!out)
    return false;

  FILE *f = std::fopen(path.c_str(), "rb");
  if (!f)
    return false;
  std::fseek(f, 0, SEEK_END);
  long len = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (len < 54) {
    std::fclose(f);
    return false;
  }
  std::vector<uint8_t> buf(static_cast<size_t>(len));
  size_t got = std::fread(buf.data(), 1, buf.size(), f);
  std::fclose(f);
  if (got != buf.size())
    return false;

  if (buf[0] != 'B' || buf[1] != 'M')
    return false;

  const uint32_t data_off = detail::get_u32(&buf[10]);
  const uint32_t dib_size = detail::get_u32(&buf[14]);
  if (dib_size < 40)
    return false; // only BITMAPINFOHEADER+ supported

  const int32_t w = detail::get_i32(&buf[18]);
  const int32_t h_raw = detail::get_i32(&buf[22]);
  const uint16_t bpp = detail::get_u16(&buf[28]);
  const uint32_t compression = detail::get_u32(&buf[30]);
  if (w <= 0 || h_raw == 0)
    return false;
  if (compression != 0)
    return false; // BI_RGB only
  if (bpp != 24 && bpp != 32)
    return false;

  const bool bottom_up = h_raw > 0;
  const int height = bottom_up ? h_raw : -h_raw;
  const int width = w;

  const int bytes_pp = bpp / 8;
  // BMP rows are padded to 4-byte boundaries.
  const size_t row_stride =
      ((static_cast<size_t>(width) * bytes_pp + 3u) / 4u) * 4u;
  if (data_off + row_stride * static_cast<size_t>(height) > buf.size())
    return false;

  out->width = width;
  out->height = height;
  out->rgba.assign(static_cast<size_t>(width) * height * 4u, 0);

  for (int dy = 0; dy < height; ++dy) {
    // Destination row dy (top-down). Source row depends on orientation.
    const int src_row = bottom_up ? (height - 1 - dy) : dy;
    const uint8_t *src =
        &buf[data_off + static_cast<size_t>(src_row) * row_stride];
    uint8_t *dst = &out->rgba[(static_cast<size_t>(dy) * width) * 4u];
    for (int x = 0; x < width; ++x) {
      const uint8_t *s = src + static_cast<size_t>(x) * bytes_pp;
      dst[0] = s[2];                       // R (from B)
      dst[1] = s[1];                       // G
      dst[2] = s[0];                       // B (from R)
      dst[3] = (bytes_pp == 4) ? s[3] : 255; // A
      dst += 4;
    }
  }
  return true;
}

// ===========================================================================
// Comparison + golden regeneration
// ===========================================================================

namespace detail {

inline bool regen_enabled() {
  const char *v = std::getenv("UI_GOLDEN_REGEN");
  return v && v[0] == '1' && v[1] == '\0';
}

// "foo/bar.bmp" + "_actual" -> "foo/bar_actual.bmp"
inline std::string with_suffix(const std::string &path,
                               const std::string &suffix) {
  size_t slash = path.find_last_of("/\\");
  size_t dot = path.find_last_of('.');
  if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
    return path + suffix; // no extension
  return path.substr(0, dot) + suffix + path.substr(dot);
}

} // namespace detail

// Compares `actual` against the golden BMP at `golden_path` with a per-channel
// absolute `tolerance`. Returns true when every pixel matches within tolerance
// on all four channels.
//
// On failure: writes <golden>_actual.bmp (the rendered image) and
// <golden>_diff.bmp (per-pixel max-channel-delta amplified to white-on-black),
// and fills `*report`.
//
// When UI_GOLDEN_REGEN=1 is set, the golden is (re)written from `actual` and
// the function returns true (report->regenerated == true). This also creates a
// brand-new golden the first time a test is added.
inline bool compare_bmp(const Image &actual, const std::string &golden_path,
                        int tolerance, CompareReport *report) {
  CompareReport local;
  CompareReport &r = report ? *report : local;
  r = CompareReport{};

  if (!actual.valid()) {
    r.message = "actual image is invalid";
    return false;
  }

  // Regeneration short-circuits all comparison.
  if (detail::regen_enabled()) {
    if (!write_bmp(golden_path, actual)) {
      r.message = "UI_GOLDEN_REGEN=1 but failed to write golden: " + golden_path;
      return false;
    }
    r.regenerated = true;
    r.message = "regenerated golden: " + golden_path;
    return true;
  }

  Image golden;
  const bool loaded = load_bmp(golden_path, &golden);
  if (!loaded) {
    r.missing_golden = true;
    r.actual_path = detail::with_suffix(golden_path, "_actual");
    write_bmp(r.actual_path, actual);
    r.message = "missing golden: " + golden_path +
                " (wrote " + r.actual_path +
                "; set UI_GOLDEN_REGEN=1 to create it)";
    return false;
  }

  r.golden_width = golden.width;
  r.golden_height = golden.height;
  if (golden.width != actual.width || golden.height != actual.height) {
    r.size_mismatch = true;
    r.actual_path = detail::with_suffix(golden_path, "_actual");
    write_bmp(r.actual_path, actual);
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "size mismatch: golden %dx%d vs actual %dx%d (wrote %s)",
                  golden.width, golden.height, actual.width, actual.height,
                  r.actual_path.c_str());
    r.message = buf;
    return false;
  }

  // Per-channel tolerance compare, building a diff image as we go.
  Image diff;
  diff.width = actual.width;
  diff.height = actual.height;
  diff.rgba.assign(actual.byte_size(), 0);

  if (tolerance < 0)
    tolerance = 0;

  for (int y = 0; y < actual.height; ++y) {
    for (int x = 0; x < actual.width; ++x) {
      const size_t i = (static_cast<size_t>(y) * actual.width + x) * 4u;
      int worst = 0;
      for (int c = 0; c < 4; ++c) {
        int d = static_cast<int>(actual.rgba[i + c]) -
                static_cast<int>(golden.rgba[i + c]);
        if (d < 0)
          d = -d;
        if (d > worst)
          worst = d;
      }
      if (worst > r.max_channel_delta)
        r.max_channel_delta = worst;

      if (worst > tolerance) {
        ++r.diff_pixels;
        if (r.first_bad_x < 0) {
          r.first_bad_x = x;
          r.first_bad_y = y;
        }
        // Amplify the delta so even small mismatches are visible; opaque.
        int amp = worst * 4;
        uint8_t v = static_cast<uint8_t>(amp > 255 ? 255 : amp);
        diff.rgba[i + 0] = v;
        diff.rgba[i + 1] = v;
        diff.rgba[i + 2] = v;
        diff.rgba[i + 3] = 255;
      } else {
        diff.rgba[i + 3] = 255; // matched pixels stay black, opaque
      }
    }
  }

  if (r.diff_pixels == 0) {
    r.message = "match";
    return true;
  }

  r.actual_path = detail::with_suffix(golden_path, "_actual");
  r.diff_path = detail::with_suffix(golden_path, "_diff");
  write_bmp(r.actual_path, actual);
  write_bmp(r.diff_path, diff);

  char buf[384];
  std::snprintf(
      buf, sizeof(buf),
      "%d pixel(s) exceed tolerance %d (max delta %d, first at %d,%d); "
      "wrote %s and %s",
      r.diff_pixels, tolerance, r.max_channel_delta, r.first_bad_x,
      r.first_bad_y, r.actual_path.c_str(), r.diff_path.c_str());
  r.message = buf;
  return false;
}

// Convenience: render `draw` to an offscreen target and compare against the
// golden in one call. `ctx` must already be init()'d. Returns true on
// match/regeneration. The optional `clear` overrides the (opaque black)
// background.
inline bool render_and_compare(GoldenContext &ctx, int width, int height,
                               const GoldenContext::DrawFn &draw,
                               const std::string &golden_path, int tolerance,
                               CompareReport *report,
                               uint8_t clear[4] = nullptr) {
  Image actual;
  if (!ctx.render_to_image(width, height, draw, &actual, clear)) {
    if (report) {
      *report = CompareReport{};
      report->message = "render_to_image failed";
    }
    return false;
  }
  return compare_bmp(actual, golden_path, tolerance, report);
}

} // namespace ui_test
