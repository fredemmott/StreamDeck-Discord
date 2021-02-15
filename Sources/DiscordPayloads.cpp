#pragma once

#include <nlohmann/json.hpp>
#include "json_ext.h"

namespace DiscordPayloads {
	void to_json(nlohmann::json& j, const Channel& c) {
		j = { { "id", c.id }, { "type", c.type } };
		if (c.guild_id.has_value()) {
			j["guild_id"] = c.guild_id.value();
		}
		if (c.name.has_value()) {
			j["name"] = c.name.value();
		}
	};
	void from_json(const nlohmann::json& j, Channel& c) {
		j.at("id").get_to(c.id);
		j.at("type").get_to(c.type);
		if (j.contains("name")) {
			c.name = j.at("name").get<std::string>();
		}
		if (j.contains("guild_id")) {
			c.name = j.at("guild_id").get<std::string>();
		}
	}
}
