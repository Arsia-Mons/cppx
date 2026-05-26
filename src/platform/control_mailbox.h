#pragma once

#include <SDL3/SDL.h>

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "../game/ui/game_ui_pipeline.h"
#include "../ui/focus/ui_focus.h"

namespace platform {

class ControlMailbox {
public:
    bool init(const char *dir);
    bool active() const { return active_; }
    void set_game_state_json_provider(std::function<std::string()> provider);

    void poll(::ui::UiInputFrame &ui_input,
              bool &running,
              SDL_Window *window,
              game::ui::GameUiPipeline &pipeline);

    bool apply_pointer_override(float &x, float &y, bool &down) const;
    void capture_after_render(SDL_Renderer *renderer, game::ui::GameUiPipeline &pipeline);
    void finish_frame(game::ui::GameUiPipeline &pipeline);
    void shutdown(void);

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
    std::string state_json(game::ui::GameUiPipeline &pipeline);
    bool save_screenshot(SDL_Renderer *renderer, const std::filesystem::path &out,
                         std::string *error);

    bool active_ = false;
    uint64_t frame_index_ = 0;
    std::filesystem::path dir_;
    std::filesystem::path requests_dir_;
    std::filesystem::path replies_dir_;
    std::filesystem::path artifacts_dir_;
    std::function<std::string()> game_state_json_provider_ = {};

    bool pointer_override_ = false;
    float pointer_x_ = -1000.0f;
    float pointer_y_ = -1000.0f;
    bool pointer_down_ = false;

    std::vector<PendingWait> pending_waits_;
    std::vector<PendingScreenshot> pending_screenshots_;
    std::vector<PendingCapture> pending_captures_;
};

} // namespace platform
