#pragma once

#include <array>
#include <functional>
#include <memory>

#include "../../react.h"
#include "../../ui/focus/ui_focus.h"
#include "navigation/screen_stack.h"

namespace client::ui {

constexpr int CLIENT_UI_MAX_QUEUED_MUTATIONS = 128;

using DeferredUiMutation = std::function<void()>;

struct ScreenNavigator {
    UiScreenEntryId current_entry_id = 0;
    std::function<void(std::unique_ptr<UiScreen>)> push = {};
    std::function<void(std::unique_ptr<UiScreen>)> reset_to = {};
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
    bool queue_reset_to_screen(std::unique_ptr<UiScreen> screen);
    bool queue_pop_current(UiScreenEntryId entry_id);
    bool queue_pop_top();
    bool queue_deferred_mutation(DeferredUiMutation mutation);
    int pending_mutation_count() const { return mutation_count_; }
    void drain_deferred_mutations();

private:
    enum class MutationKind {
        Push,
        ResetTo,
        PopCurrent,
        PopTop,
        Deferred,
    };

    struct QueuedMutation {
        MutationKind kind = MutationKind::PopTop;
        UiScreenEntryId entry_id = 0;
        std::unique_ptr<UiScreen> screen = nullptr;
        DeferredUiMutation deferred = {};
    };

    bool queue_mutation(QueuedMutation mutation);
    void clear_mutations();

    ScreenStack screens_;
    ::ui::UiFocusRuntime focus_ = {};
    std::array<QueuedMutation, CLIENT_UI_MAX_QUEUED_MUTATIONS> mutations_ = {};
    int mutation_count_ = 0;
};

ScreenNavigator use_screen_navigator();

} // namespace client::ui
