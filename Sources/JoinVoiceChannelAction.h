#pragma once

#include "DiscordESDAction.h"
#include <StreamDeckSDK/ESDConnectionManager.h>
#include <StreamDeckSDK/ESDLogger.h>

class JoinVoiceChannelAction final : public DiscordESDAction {
 public:
  static const std::string ACTION_ID;
  using DiscordESDAction::DiscordESDAction;

 protected:
  virtual void KeyUp(DiscordClient& client) override final {
  }

  virtual int GetDesiredState(const DiscordClient& client) override final {
    return 0;
  }

  virtual void Reconnected(DiscordClient& client) override final {
    client.getCurrentVoiceChannel().subscribe(
      [this](const auto& settings) {
      }
    );
  }

  virtual void SendToPlugin(const nlohmann::json& payload) override final {
    ESDDebug("Received from PI: {}", payload.dump());
    if (!payload.contains("event")) {
      return;
    }
    auto event = payload.at("event").get<std::string>();

    if (event == "getDiscordGuilds") {
      ESDDebug("Spawning GetDiscordGuilds");
      asio::co_spawn(
        *GetESD()->GetAsioContext(),
        [this]() -> asio::awaitable<void> {
          auto client = GetDiscordClient().lock();
          if (!client) {
            ESDDebug("No client");
            co_return;
          }
          ESDDebug("waiting for uguilds");
          auto guilds = co_await client->coGetGuilds();
          ESDDebug("got guilds");
          SendToPropertyInspector({
            { "event", "discordGuilds" },
            { "guilds", guilds }
          });
        },
        asio::detached
      );
      return;
    }
  }
};
