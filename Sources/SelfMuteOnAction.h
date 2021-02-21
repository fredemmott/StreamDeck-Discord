#pragma once

#include "DiscordVoiceSettingsAction.h"

class SelfMuteOnAction final : public DiscordVoiceSettingsAction {
 public:
  using DiscordVoiceSettingsAction::DiscordVoiceSettingsAction;
  static const std::string ACTION_ID;

 protected:
  virtual void KeyUp(const nlohmann::json&, DiscordClient& client) override {
    client.setIsMuted(true);
  }

  virtual int GetDesiredState(
    const DiscordClient::VoiceSettings::Data& settings
  ) override {
    return (settings.deaf || settings.mute) ? 0 : 1;
  }
};
