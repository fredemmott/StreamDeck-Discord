#pragma once

#include "DiscordVoiceSettingsAction.h"

class DeafenOffAction final : public DiscordVoiceSettingsAction {
 public:
  using DiscordVoiceSettingsAction::DiscordVoiceSettingsAction;
  static const std::string ACTION_ID;

  virtual void KeyUp(const nlohmann::json&, DiscordClient& client) override {
    client.setIsDeafened(false);
  }

  virtual int GetDesiredState(
    const DiscordClient::VoiceSettings::Data& settings
  ) override {
    return settings.deaf ? 0 : 1;
  }
};
