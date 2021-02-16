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
      ESDDebug("Spawning getDiscordGuilds");
      asio::co_spawn(
        *GetESD()->GetAsioContext(),
        [this]() -> asio::awaitable<void> {
          auto client = GetDiscordClient().lock();
          if (!client) {
            ESDDebug("No client");
            co_return;
          }
          ESDDebug("waiting for guilds");
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

		if (event == "getGuildChannels") {
      const auto guild_id = payload.at("guild_id").get<std::string>();
			ESDDebug("Spawning getGuildChannels");
			asio::co_spawn(
				*GetESD()->GetAsioContext(),
				[this, guild_id]() -> asio::awaitable<void> {
					auto client = GetDiscordClient().lock();
					if (!client) {
						ESDDebug("No client");
						co_return;
					}
					ESDDebug("waiting for channels");
					auto channels = co_await client->coGetChannels(guild_id);
					ESDDebug("got channels");
					SendToPropertyInspector({
						{ "event", "discordChannels" },
						{ "guild_id", guild_id },
            { "channels", channels },
					});
				},
				asio::detached
			);
			return;
		}

  }
};
