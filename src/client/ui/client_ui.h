#pragma once

#include <array>
#include <functional>
#include <memory>

#include "../../react.h"
#include "../../ui/focus/ui_focus.h"
#include "screen_stack.h"

namespace client::ui {

constexpr int CLIENT_UI_MAX_WRITES = 128;

using UiDeferredWrite = std::function<void()>;
using QueueUiWrite = std::function<void(UiDeferredWrite)>;

struct ScreenNavigator {
    UiScreenEntryId current_entry_id = 0;
    std::function<void(std::unique_ptr<UiScreen>)> push = {};
    std::function<void()> pop_current = {};
    std::function<void()> pop_top = {};
};

class ClientUi {
public:
    ClientUi();

    ScreenStack &screens() { return screens_; }
    const ScreenStack &screens() const { return screens_; }

    ::ui::UiFocusRuntime &focus_runtime() { return focus_; }

    void begin_frame(const ::ui::UiInputFrame &input);
    void build_visible_screens();
    void end_layout(const ::ui::UiInputFrame &input);

    bool push_screen(std::unique_ptr<UiScreen> screen);
    bool replace_top(std::unique_ptr<UiScreen> screen);
    bool queue_push_screen(std::unique_ptr<UiScreen> screen);
    bool queue_pop_current(UiScreenEntryId entry_id);
    bool queue_pop_top();
    bool queue_deferred_write(UiDeferredWrite write);
    int pending_write_count() const { return write_count_; }
    void drain_writes();

private:
    enum class WriteKind {
        Push,
        PopCurrent,
        PopTop,
        Deferred,
    };

    struct QueuedWrite {
        WriteKind kind = WriteKind::PopTop;
        UiScreenEntryId entry_id = 0;
        std::unique_ptr<UiScreen> screen = nullptr;
        UiDeferredWrite deferred = {};
    };

    bool queue_write(QueuedWrite write);
    void clear_writes();

    ScreenStack screens_;
    ::ui::UiFocusRuntime focus_ = {};
    std::array<QueuedWrite, CLIENT_UI_MAX_WRITES> writes_ = {};
    int write_count_ = 0;
};

ScreenNavigator use_screen_navigator();
QueueUiWrite use_ui_write_queue();

} // namespace client::ui
