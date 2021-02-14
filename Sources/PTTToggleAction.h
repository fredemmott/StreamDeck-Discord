#pragma once

#include "DiscordVoiceSettingsAction.h"

class PTTToggleAction final : public DiscordVoiceSettingsAction{
 public:
  using DiscordVoiceSettingsAction::DiscordVoiceSettingsAction;
  static const std::string ACTION_ID;
  virtual std::string GetActionID() const override { return ACTION_ID; }

  virtual void KeyUp(DiscordClient& client) override {
    client.setIsPTT(client.getVoiceSettings()->mode.type != DiscordPayloads::VoiceSettingsModeType::PUSH_TO_TALK);
  }

  virtual int GetDesiredState(const DiscordClient::VoiceSettings::Data& state) override {
      return state.mode.type == DiscordPayloads::VoiceSettingsModeType::PUSH_TO_TALK ? 1 : 0;
  }
};
