#pragma once

#include "DiscordVoiceSettingsAction.h"

class SelfMuteToggleAction final : public DiscordVoiceSettingsAction {
 public:
  using DiscordVoiceSettingsAction::DiscordVoiceSettingsAction;
  static const std::string ACTION_ID;

 protected:
  virtual void KeyUp(DiscordClient& client) override {
    auto& settings = client.getVoiceSettings();
    client.setIsMuted(!(settings->deaf || settings->mute));
  }

  virtual int GetDesiredState(
    const DiscordClient::VoiceSettings::Data& settings
  ) override {
    return (settings.deaf || settings.mute) ? 1 : 0;
  }
};
