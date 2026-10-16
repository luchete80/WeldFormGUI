#pragma once

#include <nlohmann/json.hpp>

#include <istream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

inline nlohmann::json ParseJsonRejectDuplicateKeys(std::istream& input) {
  std::vector<std::set<std::string>> objectKeys;
  const auto callback = [&objectKeys](int, nlohmann::json::parse_event_t event,
                                       nlohmann::json& parsed) {
    if (event == nlohmann::json::parse_event_t::object_start) {
      objectKeys.emplace_back();
    } else if (event == nlohmann::json::parse_event_t::key) {
      const std::string key = parsed.get<std::string>();
      if (objectKeys.empty() || !objectKeys.back().insert(key).second)
        throw std::runtime_error("JSON object contains duplicate key: " + key);
    } else if (event == nlohmann::json::parse_event_t::object_end) {
      if (!objectKeys.empty()) objectKeys.pop_back();
    }
    return true;
  };
  return nlohmann::json::parse(input, callback);
}
