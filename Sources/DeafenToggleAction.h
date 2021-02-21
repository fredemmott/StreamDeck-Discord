#pragma once

#include "DiscordVoiceSettingsAction.h"

class DeafenToggleAction final : public DiscordVoiceSettingsAction {
 public:
  using DiscordVoiceSettingsAction::DiscordVoiceSettingsAction;
  static const std::string ACTION_ID;

 protected:
  virtual void KeyUp(const nlohmann::json&, DiscordClient& client) override {
    client.setIsDeafened(!client.getVoiceSettings()->deaf);
  }

  virtual int GetDesiredState(
    const DiscordClient::VoiceSettings::Data& settings
  ) override {
    return settings.deaf ? 1 : 0;
  }
};
