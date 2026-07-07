// simulator_factory.hpp
#pragma once

#include <memory>
#include <string>

#include "sim/backend.hpp"
#include "sim/simulator.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

std::unique_ptr<Backend> make_backend(
    std::unique_ptr<Simulator> simulator,
    const std::string& backend_type,
    const JSON& backend_json
);

}
}