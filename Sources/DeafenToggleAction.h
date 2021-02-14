#pragma once

#include "DiscordESDActionV2.h"
#include <StreamDeckSDK/ESDConnectionManager.h>
#include <StreamDeckSDK/ESDLogger.h>

class DeafenToggleAction final : public DiscordESDActionV2 {
 public:
  using DiscordESDActionV2::DiscordESDActionV2;
  static const std::string ACTION_ID;
  virtual std::string GetActionID() const override { return ACTION_ID; }

 protected:
  virtual void KeyUp(DiscordClient& client) override {
    const auto& settings = client.getVoiceSettings();
    ESDDebug("KeyUp");
    client.setIsDeafened(!settings->deaf);
  }

  virtual int GetDesiredState(const DiscordClient& client) override {
      const auto& settings = client.getVoiceSettings();
      ESDDebug("Setting settings");
      return settings->deaf ? 1 : 0;
  }

  virtual void Reconnected(DiscordClient& client) {
    client.getVoiceSettings().subscribe(
      [this](const auto& settings) {
        ESDDebug("In reconnected callback");
        GetESD()->SetState(
          settings.deaf ? 1 : 0,
          GetContext()
        );
      }
    );
  }
};
