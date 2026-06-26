#pragma once

#include "aer_adapters/aer_internal_fwd.hpp" 
#include "quantum_task.hpp"
#include "backends/cc_backend.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "classical_channel/classical_channel.hpp"

#include "utils/json.hpp"
#include "logger.hpp"

namespace cunqa {
namespace sim {

class AerCCSimulator final : public SimulatorStrategy<CCBackend> {
public:
    AerCCSimulator();
    AerCCSimulator(const JSON& backend_json);
    ~AerCCSimulator() = default;

    inline std::string get_name() const override {return "Aer";}
    JSON execute(const CCBackend& backend, const QuantumTask& circuit) override;

private:
    comm::ClassicalChannel classical_channel;
    aer_detail::NoiseModelPtr noise_model;
};


} // End namespace sim
} // End namespace cunqa