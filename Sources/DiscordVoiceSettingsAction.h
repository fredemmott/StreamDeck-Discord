#pragma once

#include "DiscordESDAction.h"

class DiscordVoiceSettingsAction : public DiscordESDAction {
 public:
  using DiscordESDAction::DiscordESDAction;

 protected:
  virtual int GetDesiredState(
    const DiscordClient::VoiceSettings::Data& settings
  ) = 0;

  virtual int GetDesiredState(const DiscordClient& client) override final;
  virtual void Reconnected(DiscordClient& client) override final;
};
