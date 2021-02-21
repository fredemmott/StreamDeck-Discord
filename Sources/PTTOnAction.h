#pragma once

#include "DiscordVoiceSettingsAction.h"

class PTTOnAction final : public DiscordVoiceSettingsAction{
 public:
  using DiscordVoiceSettingsAction::DiscordVoiceSettingsAction;
  static const std::string ACTION_ID;

  virtual void KeyUp(const nlohmann::json&, DiscordClient& client) override {
    client.setIsPTT(true);
  }

  virtual int GetDesiredState(const DiscordClient::VoiceSettings::Data& state) override {
      return state.mode.type == DiscordPayloads::VoiceSettingsModeType::PUSH_TO_TALK ? 0 : 1;
  }
};
