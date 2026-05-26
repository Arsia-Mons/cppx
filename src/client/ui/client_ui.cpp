#include "client_ui.h"

namespace client::ui {

struct ScreenContext {
    ClientUi *client_ui = nullptr;
    UiScreenEntryId current_entry_id = 0;
};

static ReactContext ScreenContextValue = {};

ClientUi::ClientUi() {
    ::ui::ui_focus_init(&focus_);
}

void ClientUi::begin_frame(const ::ui::UiInputFrame &input) {
    ::ui::ui_focus_set_current(&focus_);
    ::ui::ui_focus_begin_frame(input);
    clear_writes();
}

void ClientUi::build_visible_screens() {
    ::ui::Span<UiScreen *> visible = screens_.visible_screens();
    for (UiScreen *screen : visible) {
        if (!screen) continue;
        ScreenContext context = {
            .client_ui = this,
            .current_entry_id = screen->entry_id(),
        };
        REACT_PROVIDER_ENTER_KEY("ScreenProvider", screen->entry_id());
        PROVIDE(&ScreenContextValue, &context) {
            screen->build_ui();
        }
        REACT_PROVIDER_EXIT();
    }
}

void ClientUi::end_layout(const ::ui::UiInputFrame &input) {
    ::ui::ui_focus_set_current(&focus_);
    ::ui::ui_focus_end_layout(input);
}

bool ClientUi::push_screen(std::unique_ptr<UiScreen> screen) {
    return screens_.push(std::move(screen));
}

bool ClientUi::replace_top(std::unique_ptr<UiScreen> screen) {
    return screens_.replace_top(std::move(screen));
}

bool ClientUi::queue_push_screen(std::unique_ptr<UiScreen> screen) {
    if (!screen) return false;
    return queue_write({
        .kind = WriteKind::Push,
        .screen = std::move(screen),
    });
}

bool ClientUi::queue_pop_current(UiScreenEntryId entry_id) {
    return queue_write({
        .kind = WriteKind::PopCurrent,
        .entry_id = entry_id,
    });
}

bool ClientUi::queue_pop_top() {
    return queue_write({ .kind = WriteKind::PopTop });
}

bool ClientUi::queue_write(QueuedWrite write) {
    if (write_count_ >= CLIENT_UI_MAX_WRITES) return false;
    writes_[write_count_++] = std::move(write);
    return true;
}

void ClientUi::drain_writes() {
    for (int i = 0; i < write_count_; ++i) {
        QueuedWrite &write = writes_[i];
        switch (write.kind) {
            case WriteKind::Push:
                screens_.push(std::move(write.screen));
                break;
            case WriteKind::PopCurrent:
                screens_.pop_entry(write.entry_id);
                break;
            case WriteKind::PopTop:
                screens_.pop_top();
                break;
        }
    }
    clear_writes();
}

void ClientUi::clear_writes() {
    for (int i = 0; i < write_count_; ++i) {
        writes_[i] = {};
    }
    write_count_ = 0;
}

ScreenNavigator use_screen_navigator() {
    ScreenContext *context =
        static_cast<ScreenContext *>(use_context(&ScreenContextValue));
    if (!context || !context->client_ui) return {};

    ClientUi *client_ui = context->client_ui;
    UiScreenEntryId entry_id = context->current_entry_id;
    return {
        .current_entry_id = entry_id,
        .push = [client_ui](std::unique_ptr<UiScreen> screen) {
            client_ui->queue_push_screen(std::move(screen));
        },
        .pop_current = [client_ui, entry_id] {
            client_ui->queue_pop_current(entry_id);
        },
        .pop_top = [client_ui] {
            client_ui->queue_pop_top();
        },
    };
}

} // namespace client::ui
