#include "CircuitSimulator.hpp"

#include "quantum_computation_adapter.hpp"
#include "classical_channel/classical_channel.hpp"
#include "backends/backend.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {


class MunichSimulatorAdapter : public CircuitSimulator
{
public:

    // Constructors
    MunichSimulatorAdapter() = default;
    MunichSimulatorAdapter(std::unique_ptr<QuantumComputationAdapter>&& qc_) : 
        CircuitSimulator(std::unique_ptr<QuantumComputationAdapter>(std::move(qc_)))
    {}

    inline void initializeSimulationAdapter(std::size_t nQubits) { initializeSimulation(nQubits); }
    inline void applyOperationToStateAdapter(std::unique_ptr<qc::Operation>&& op) { applyOperationToState(op); }
    inline char measureAdapter(dd::Qubit i) { return measure(i); }
    inline void resetStateAdapter(const int n_qubits) 
    { 
        qc::Targets target_qubits(n_qubits); 
        reset(new qc::NonUnitaryOperation(target_qubits));
    }

    JSON simulate(const Backend* backend);
    JSON simulate(comm::ClassicalChannel* classical_channel = nullptr, const bool allows_qc = false);
private:

    std::unordered_map<std::string, std::string> execute_shot_(
        std::vector<StructuredQuantumTask>& st_qtasks, 
        comm::ClassicalChannel* classical_channel,
        const bool allows_qc,
        const size_t& n_comm_qubits
    );
    
};

} // End of sim namespace
} // End of cunqa namespace