#include "server_provider.h"

#include "client/ui/app_shell/deferred_ui_mutation.h"
#include "client/ui/hooks/use_server.h"
#include "ui/runtime/react.h"

namespace shooter {

static ReactContext ServerContext = {};

::ui::UiElement ServerProvider(const ServerProviderValue &value,
                               ::ui::UiChildren children) {
  const ServerProviderValue *stored = ::ui::copy_value(value);
  if (!stored)
    return ::ui::empty();
  return ::ui::provider("ServerProvider", &ServerContext,
                        const_cast<ServerProviderValue *>(stored), children);
}

static ServerProviderValue *use_server_provider_value() {
  ServerProviderValue *value =
      static_cast<ServerProviderValue *>(use_context(&ServerContext));
  if (!value || !value->game) {
    react_report_error("shooter: missing ServerProvider\n");
    return nullptr;
  }
  return value;
}

ServerValue use_server() {
  ServerProviderValue *value = use_server_provider_value();
  if (!value)
    return {};
  ShooterGame *game = value->game;
  client::ui::internal::DeferredUiMutationSink mutations =
      client::ui::internal::use_deferred_ui_mutations();
  std::function<void()> reset_match = [game, mutations] {
    if (game && mutations) {
      mutations.submit([game] { game->reset(); });
    }
  };
  return {
      .game = game,
      .actions = {.reset_match = reset_match},
  };
}

} // namespace shooter
