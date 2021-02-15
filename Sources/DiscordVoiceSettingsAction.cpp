#include "DiscordVoiceSettingsAction.h"

#include <StreamDeckSDK/ESDConnectionManager.h>

int DiscordVoiceSettingsAction::GetDesiredState(const DiscordClient& client) {
  return GetDesiredState(*client.getVoiceSettings().get());
}

void DiscordVoiceSettingsAction::Reconnected(DiscordClient& client) {
  auto state = client.getState();
  client.getVoiceSettings().subscribe(
    [this](const auto& settings) {
      GetESD()->SetState(
        GetDesiredState(settings),
        GetContext()
      );
    }
  );
}
