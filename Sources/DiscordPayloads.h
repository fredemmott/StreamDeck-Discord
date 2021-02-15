#pragma once

#include <nlohmann/json.hpp>
#include "json_ext.h"

// Custom to_json/from_json are used when an std::optional field
// can be entirely missing from the json, not just set to null

namespace DiscordPayloads {
	enum class VoiceSettingsModeType {
		PUSH_TO_TALK,
		VOICE_ACTIVITY,
	};
	NLOHMANN_JSON_SERIALIZE_ENUM(
		VoiceSettingsModeType, {
		{ VoiceSettingsModeType::PUSH_TO_TALK, "PUSH_TO_TALK" },
		{ VoiceSettingsModeType::VOICE_ACTIVITY, "VOICE_ACTIVITY" },
	});
	struct VoiceSettingsMode {
		VoiceSettingsModeType type;
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
		VoiceSettingsMode, type
	);
	struct VoiceSettingsResponse {
		bool deaf;
		bool mute;
		VoiceSettingsMode mode;
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
		VoiceSettingsResponse, deaf, mute, mode
	);
	struct VoiceChannelSelect {
		std::optional<std::string> channel_id;
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
		VoiceChannelSelect, channel_id
	);

	struct Guild {
		std::string id;
		std::string name;
    std::optional<std::string> icon_url;
	};
	void to_json(nlohmann::json& j, const Guild& c);
	void from_json(const nlohmann::json& j, Guild& c);
	typedef Guild Server;

	struct GetGuildsResponse {
		std::vector<Guild> guilds;
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GetGuildsResponse, guilds);
	enum class ChannelType {
		GUILD_TEXT,
		DM,
		GUILD_VOICE,
		GROUP_DM,
		GUILD_CATEGORY,
		GUILD_NEWS,
		GUILD_STORE,
	};
	NLOHMANN_JSON_SERIALIZE_ENUM(ChannelType, {
		{ ChannelType::GUILD_TEXT, 0 },
		{ ChannelType::DM, 1 },
		{ ChannelType::GUILD_VOICE, 2 },
		{ ChannelType::GROUP_DM, 3 },
		{ ChannelType::GUILD_CATEGORY, 4 },
		{ ChannelType::GUILD_NEWS, 5 },
		{ ChannelType::GUILD_STORE, 6 },
	});
	struct Channel {
		std::string id;
		ChannelType type;
		std::optional<std::string> guild_id;
		std::optional<std::string> name;
    std::optional<std::string> parent_id;
	};
	void to_json(nlohmann::json& j, const Channel& c);
	void from_json(const nlohmann::json& j, Channel& c);

  struct GetChannelsResponse {
    std::vector<Channel> channels;
  };
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GetChannelsResponse, channels);
}
