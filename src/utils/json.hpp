#pragma once

#include <nlohmann/json.hpp>

namespace cunqa {
    using JSON = nlohmann::json;
    void write_on_file(JSON local_data, const std::string &filename, const std::string& suffix = "");
    JSON read_from_file(const std::string &filename);
    void remove_from_file(const std::string &filename, const std::string &key);

}

