#pragma once

#include <SDL3/SDL.h>

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "../client/ui/ui_pipeline.h"
#include "../ui/input.h"

namespace platform {

class ControlMailbox {
public:
    bool init(const char *dir);
    bool active() const { return active_; }
    void set_game_state_json_provider(std::function<std::string()> provider);
    // Optional readback of the active render-mode slug (e.g. "sdf"), surfaced in
    // state/inspect replies so headless scripts can confirm a switch took. The
    // mailbox stays renderer-agnostic — app/ supplies the slug via the closure.
    void set_render_mode_provider(std::function<std::string()> provider);

    void poll(::ui::UiInputFrame &ui_input,
              bool &running,
              SDL_Window *window,
              client::ui::UiPipeline &pipeline);

    bool apply_pointer_override(float &x, float &y, bool &down) const;
    void capture_after_render(SDL_Renderer *renderer, client::ui::UiPipeline &pipeline);
    void finish_frame(client::ui::UiPipeline &pipeline);
    void shutdown(void);

    // A render-mode change requested via the "render_mode" op since the last
    // call. Returns true and writes the requested slug (e.g. "sdf") into *out,
    // then clears it; false if none pending. The mailbox stays renderer-agnostic
    // (it forwards the raw slug; app/ maps it to a RenderMode).
    bool take_pending_render_mode(std::string *out);

private:
    struct PendingWait {
        int id = 0;
        uint64_t target_frame = 0;
    };

    struct PendingScreenshot {
        int id = 0;
        std::filesystem::path out;
    };

    struct PendingCapture {
        int id = 0;
        std::filesystem::path dir;
        int remaining = 0;
        int index = 0;
        std::vector<std::string> frames;
    };

    void write_ready(void);
    void write_reply(int id, bool ok, const std::string &body);
    void write_error(int id, const char *code, const std::string &message);
    std::string state_json(client::ui::UiPipeline &pipeline);
    bool save_screenshot(SDL_Renderer *renderer, const std::filesystem::path &out,
                         std::string *error);

    bool active_ = false;
    uint64_t frame_index_ = 0;
    std::filesystem::path dir_;
    std::filesystem::path requests_dir_;
    std::filesystem::path replies_dir_;
    std::filesystem::path artifacts_dir_;
    std::function<std::string()> game_state_json_provider_ = {};
    std::function<std::string()> render_mode_provider_ = {};

    bool pointer_override_ = false;
    float pointer_x_ = -1000.0f;
    float pointer_y_ = -1000.0f;
    bool pointer_down_ = false;

    std::vector<PendingWait> pending_waits_;
    std::vector<PendingScreenshot> pending_screenshots_;
    std::vector<PendingCapture> pending_captures_;
    std::string pending_render_mode_; // raw slug from a "render_mode" op; "" = none
};

} // namespace platform
