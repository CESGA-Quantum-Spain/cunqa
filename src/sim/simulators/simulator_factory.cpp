// simulator_factory.cpp

#include "simulator_factory.hpp"
#include "AER/aer_simulator_adapter.hpp"
// #include "CUNQA/cunqa_simulator_adapter.hpp"
#include "Maestro/maestro_simulator_adapter.hpp"
#include "Munich/munich_simulator_adapter.hpp"
#include "Qsim/qsim_simulator_adapter.hpp"
#include "QuEST/quest_simulator_adapter.hpp"
#include "Qulacs/qulacs_simulator_adapter.hpp"

#include <stdexcept>

namespace cunqa {
namespace sim {

std::unique_ptr<Simulator> make_simulator(const std::string& simulator_name)
{
    if (simulator_name == "Aer") {
        auto simulator = std::make_unique<AerSimulatorAdapter>();
        return simulator;
    // } else if (simulator_name == "Cunqa") {
    //     auto simulator = std::make_unique<CunqaSimulatorAdapter>();
    //     return simulator;
    } else if (simulator_name == "Maestro") {
        auto simulator = std::make_unique<MaestroSimulatorAdapter>();
        return simulator;
    } else if (simulator_name == "Munich") {
        auto simulator = std::make_unique<MunichSimulatorAdapter>();
        return simulator;
    } else if (simulator_name == "Qsim") {
        auto simulator = std::make_unique<QsimSimulatorAdapter>();
        return simulator;
    } else if (simulator_name == "Quest") {
        auto simulator = std::make_unique<QuestSimulatorAdapter>();
        return simulator;
    } else if (simulator_name == "Qulacs") {
        auto simulator = std::make_unique<QulacsSimulatorAdapter>();
        return simulator;
    } else {
        throw std::invalid_argument(
            "Unknown simulator adapter: " + simulator_name
        );
    }
}

}
}