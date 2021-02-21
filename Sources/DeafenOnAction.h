#pragma once

#include "DiscordVoiceSettingsAction.h"

class DeafenOnAction final : public DiscordVoiceSettingsAction {
 public:
  using DiscordVoiceSettingsAction::DiscordVoiceSettingsAction;
  static const std::string ACTION_ID;

  virtual void KeyUp(const nlohmann::json&, DiscordClient& client) override {
    client.setIsDeafened(true);
  }

  virtual int GetDesiredState(
    const DiscordClient::VoiceSettings::Data& settings
  ) override {
    return settings.deaf ? 1 : 0;
  }
};
