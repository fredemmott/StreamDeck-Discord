#pragma once

#include "DiscordESDAction.h"
#include <StreamDeckSDK/ESDConnectionManager.h>

class HangupAction final : public DiscordESDAction {
 public:
  static const std::string ACTION_ID;
  virtual std::string GetActionID() const override { return ACTION_ID; }
  using DiscordESDAction::DiscordESDAction;

 protected:
  virtual void KeyUp(DiscordClient& client) override final {
    client.setCurrentVoiceChannel(std::string());
  }

  virtual int GetDesiredState(const DiscordClient& client) override final {
    return client.getCurrentVoiceChannel()->channel_id.has_value() ? 1 : 0;
  }

  virtual void Reconnected(DiscordClient& client) override final {
    client.getCurrentVoiceChannel().subscribe(
      [this](const auto& settings) {
        GetESD()->SetState(
          settings.channel_id.has_value() ? 1 : 0,
          GetContext()
        );
      }
    );
  }
};
