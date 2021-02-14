#pragma once

#include "DiscordESDActionV2.h"

class DiscordVoiceSettingsAction : public DiscordESDActionV2 {
 public:
  using DiscordESDActionV2::DiscordESDActionV2;

 protected:
  virtual int GetDesiredState(
    const DiscordClient::VoiceSettings::Data& settings
  ) = 0;

  virtual int GetDesiredState(const DiscordClient& client) override final;
  virtual void Reconnected(DiscordClient& client) override final;
};
