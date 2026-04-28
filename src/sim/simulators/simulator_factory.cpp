// simulator_factory.cpp

#include "simulator_factory.hpp"
#include "AER/aer_simulator_adapter.hpp"

#include <stdexcept>

namespace cunqa {
namespace sim {

std::unique_ptr<Simulator> make_simulator(const std::string& simulator_name)
{
    if (simulator_name == "Aer") {
        auto simulator = std::make_unique<AerSimulatorAdapter>();
        return simulator;
    } else {
        throw std::invalid_argument(
            "Unknown simulator adapter: " + simulator_name
        );
    }
}

}
}