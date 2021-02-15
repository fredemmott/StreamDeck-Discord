#pragma once

#include "DiscordVoiceSettingsAction.h"

class SelfMuteOffAction final : public DiscordVoiceSettingsAction {
 public:
  using DiscordVoiceSettingsAction::DiscordVoiceSettingsAction;
  static const std::string ACTION_ID;

 protected:
  virtual void KeyUp(DiscordClient& client) override {
    client.setIsMuted(false);
  }

  virtual int GetDesiredState(
    const DiscordClient::VoiceSettings::Data& settings
  ) override {
    return (settings.deaf || settings.mute) ? 1 : 0;
  }
};
