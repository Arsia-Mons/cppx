#include "client_ui.h"

#include "internal/deferred_ui_mutation.h"

namespace client::ui {

struct ScreenContextValue {
    ClientUi *client_ui = nullptr;
    UiScreenEntryId current_entry_id = 0;
    bool is_top = false;
};

static ReactContext ScreenContext = {};

ClientUi::ClientUi() {
    ::ui::retained::focus_init(&retained_focus_);
}

void ClientUi::begin_frame(const ::ui::UiInputFrame &input) {
    (void)input;
    clear_mutations();
}

void ClientUi::build_visible_screens() {
    ::ui::Span<UiScreen *> visible = screens_.visible_screens();
    for (int i = 0; i < visible.count; ++i) {
        UiScreen *screen = visible[i];
        if (!screen)
            continue;
        auto build_screen = [&] {
            ScreenContextValue context = {
                .client_ui = this,
                .current_entry_id = screen->entry_id(),
                .is_top = i == visible.count - 1,
            };
            REACT_PROVIDER_ENTER_KEY("ScreenProvider", screen->entry_id());
            PROVIDE(&ScreenContext, &context) {
                screen->build_ui();
            }
            REACT_PROVIDER_EXIT();
        };

        switch (screen->kind()) {
        case ScreenKind::Normal:
            build_screen();
            break;
        case ScreenKind::Overlay:
            build_screen();
            break;
        }
    }
}

void ClientUi::end_layout(const ::ui::UiInputFrame &input) {
    UiScreen *top = screens_.top();
    if (input.cancel_pressed && top && top->kind() == ScreenKind::Overlay) {
        queue_pop_current(top->entry_id());
    }
}

bool ClientUi::update_retained_runtime(
    const ::ui::retained::FlexLayoutAdapter &layout,
    ::ui::retained::LayoutViewport viewport,
    const ::ui::retained::InputFrame &input) {
    if (!::ui::retained::compute_flex_layout(layout, retained_tree_, viewport))
        return false;
    if (!::ui::retained::focus_update(&retained_focus_, retained_tree_, input))
        return false;
    ::ui::retained::NodeId focused =
        ::ui::retained::focus_changed_id(retained_focus_);
    if (focused != 0) {
        retained_tree_.invoke_focus(focused);
    }
    ::ui::retained::NodeId confirmed =
        ::ui::retained::focus_confirmed_id(retained_focus_);
    if (confirmed != 0) {
        retained_tree_.invoke_confirm(confirmed);
    }
    return ::ui::retained::build_draw_list(retained_tree_,
                                           &retained_draw_list_);
}

bool ClientUi::push_screen(std::unique_ptr<UiScreen> screen) {
    return screens_.push(std::move(screen));
}

bool ClientUi::replace_top(std::unique_ptr<UiScreen> screen) {
    return screens_.replace_top(std::move(screen));
}

bool ClientUi::queue_push_screen(std::unique_ptr<UiScreen> screen) {
    if (!screen)
        return false;
    return queue_mutation({
        .kind = MutationKind::Push,
        .screen = std::move(screen),
    });
}

bool ClientUi::queue_reset_to_screen(std::unique_ptr<UiScreen> screen) {
    if (!screen)
        return false;
    return queue_mutation({
        .kind = MutationKind::ResetTo,
        .screen = std::move(screen),
    });
}

bool ClientUi::queue_pop_current(UiScreenEntryId entry_id) {
    return queue_mutation({
        .kind = MutationKind::PopCurrent,
        .entry_id = entry_id,
    });
}

bool ClientUi::queue_pop_top() {
    return queue_mutation({ .kind = MutationKind::PopTop });
}

bool ClientUi::queue_deferred_mutation(DeferredUiMutation mutation) {
    if (!mutation)
        return false;
    return queue_mutation({
        .kind = MutationKind::Deferred,
        .deferred = std::move(mutation),
    });
}

bool ClientUi::queue_mutation(QueuedMutation mutation) {
    if (mutation_count_ >= CLIENT_UI_MAX_QUEUED_MUTATIONS)
        return false;
    mutations_[mutation_count_++] = std::move(mutation);
    return true;
}

void ClientUi::drain_deferred_mutations() {
    for (int i = 0; i < mutation_count_; ++i) {
        QueuedMutation &mutation = mutations_[i];
        switch (mutation.kind) {
        case MutationKind::Push:
            screens_.push(std::move(mutation.screen));
            break;
        case MutationKind::ResetTo:
            screens_.reset_to(std::move(mutation.screen));
            break;
        case MutationKind::PopCurrent:
            screens_.pop_entry(mutation.entry_id);
            break;
        case MutationKind::PopTop:
            screens_.pop_top();
            break;
        case MutationKind::Deferred:
            if (mutation.deferred)
                mutation.deferred();
            break;
        }
    }
    clear_mutations();
}

void ClientUi::clear_mutations() {
    for (int i = 0; i < mutation_count_; ++i) {
        mutations_[i] = {};
    }
    mutation_count_ = 0;
}

ScreenNavigator use_screen_navigator() {
    ScreenContextValue *context =
        static_cast<ScreenContextValue *>(use_context(&ScreenContext));
    if (!context || !context->client_ui) {
        react_report_error(
            "client/ui: missing ScreenProvider for use_screen_navigator\n");
        return {};
    }

    ClientUi *client_ui = context->client_ui;
    UiScreenEntryId entry_id = context->current_entry_id;
    return {
        .current_entry_id = entry_id,
        .push =
            [client_ui](std::unique_ptr<UiScreen> screen) {
                client_ui->queue_push_screen(std::move(screen));
            },
        .reset_to =
            [client_ui](std::unique_ptr<UiScreen> screen) {
                client_ui->queue_reset_to_screen(std::move(screen));
            },
        .pop_current = [client_ui,
                        entry_id] { client_ui->queue_pop_current(entry_id); },
        .pop_top = [client_ui] { client_ui->queue_pop_top(); },
    };
}

bool use_screen_is_top() {
    ScreenContextValue *context =
        static_cast<ScreenContextValue *>(use_context(&ScreenContext));
    if (!context) {
        react_report_error("client/ui: missing ScreenProvider for use_screen_is_top\n");
        return false;
    }
    return context->is_top;
}

namespace internal {

bool DeferredUiMutationSink::submit(DeferredUiMutation mutation) const {
    if (!client_ui || !mutation)
        return false;
    return client_ui->queue_deferred_mutation(std::move(mutation));
}

DeferredUiMutationSink use_deferred_ui_mutations() {
    ScreenContextValue *context =
        static_cast<ScreenContextValue *>(use_context(&ScreenContext));
    if (!context || !context->client_ui) {
        react_report_error(
            "client/ui: missing ScreenProvider for use_deferred_ui_mutations\n");
        return {};
    }

    return { .client_ui = context->client_ui };
}

} // namespace internal

} // namespace client::ui
