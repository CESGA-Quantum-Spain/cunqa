// simulator_factory.hpp
#pragma once

#include <memory>
#include <string>

#include "sim/simulator.hpp"

namespace cunqa {
namespace sim {

std::unique_ptr<Simulator> make_simulator(const std::string& simulator_name);

}
}