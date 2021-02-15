#include "DiscordPayloads.h"

namespace DiscordPayloads {
  ///// Guild ///
	void to_json(nlohmann::json& j, const Guild& g) {
    j = { { "id", g.id }, { "name", g.name } };
    if (g.icon_url.has_value()) {
      j.emplace("icon_url", g.icon_url.value());
    }
  }
	void from_json(const nlohmann::json& j, Guild& g) {
    j.at("id").get_to(g.id);
    j.at("name").get_to(g.name);
    if (j.contains("icon_url")) {
      j.at("icon_url").get_to(g.icon_url);
    }
  }

  ///// Channel /////
  void to_json(nlohmann::json& j, const Channel& c) {
    j = { { "id", c.id }, { "type", c.type } };
    if (c.guild_id.has_value()) {
      j.emplace("guild_id", c.guild_id.value());
    }
    if (c.name.has_value()) {
      j.emplace("name", c.name.value());
    }
    if (c.parent_id.has_value()) {
      j.emplace("parent_id", c.parent_id.value());
    }
  };
  void from_json(const nlohmann::json& j, Channel& c) {
    j.at("id").get_to(c.id);
    j.at("type").get_to(c.type);
    if (j.contains("name")) {
      j.at("name").get_to(c.name);
    }
    if (j.contains("guild_id")) {
      j.at("guild_id").get_to(c.guild_id);
    }
    if (j.contains("parent_id")) {
      j.at("parent_id").get_to(c.parent_id);
    }
  }
}
