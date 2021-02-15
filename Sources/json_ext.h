#pragma once

#include <nlohmann/json.hpp>

#include <optional>

// Should be unneeded in a later release of nlohmann/json
// https://github.com/nlohmann/json/pull/2117
namespace nlohmann {
template <typename T>
struct adl_serializer<std::optional<T>> {
	static void to_json(json& j, const std::optional<T>& opt) {
		if (opt == std::nullopt) {
			j = nullptr;
		} else {
			j = *opt;
		}
	}

	static void from_json(const json& j, std::optional<T>& opt) {
		if (j.is_null()) {
			opt = std::nullopt;
		} else {
			opt = j.get<T>();
		}
	}
};
}// namespace nlohmann
