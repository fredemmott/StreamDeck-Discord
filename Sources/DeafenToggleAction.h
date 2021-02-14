#pragma once

#include "DiscordVoiceSettingsAction.h"

class DeafenToggleAction final : public DiscordVoiceSettingsAction {
 public:
  using DiscordVoiceSettingsAction::DiscordVoiceSettingsAction;
  static const std::string ACTION_ID;
  virtual std::string GetActionID() const override { return ACTION_ID; }

 protected:
  virtual void KeyUp(DiscordClient& client) override {
    client.setIsDeafened(!client.getVoiceSettings()->deaf);
  }

  virtual int GetDesiredState(
    const DiscordClient::VoiceSettings::Data& settings
  ) override {
    return settings.deaf ? 1 : 0;
  }
};
