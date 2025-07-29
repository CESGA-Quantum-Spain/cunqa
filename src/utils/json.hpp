#pragma once

#include <nlohmann/json.hpp>

namespace cunqa {
    using JSON = nlohmann::json;
    void write_on_file(JSON local_data, const std::string &filename);
}

