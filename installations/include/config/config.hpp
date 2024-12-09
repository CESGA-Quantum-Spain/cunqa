#pragma once

#include <nlohmann/json.hpp>
#include "utils/helpers.hpp"

using json = nlohmann::json;

namespace config {

class Config {
public:
    virtual ~Config() = default;

    virtual void load(const json& config) = 0;
    virtual json dump() const = 0;
};

}