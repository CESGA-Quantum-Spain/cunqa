#include "CircuitSimulator.hpp"
#include "StochasticNoiseSimulator.hpp"
#include "quantum_computation_adapter.hpp"
#include "classical_channel.hpp"

#include "utils/json.hpp"


namespace cunqa {
namespace sim {

class CircuitSimulatorAdapter : public CircuitSimulator<dd::DDPackageConfig>
{
public:

    // Constructors
    CircuitSimulatorAdapter(std::unique_ptr<QuantumComputationAdapter>&& qc_) : CircuitSimulator(std::unique_ptr<QuantumComputationAdapter>(std::move(qc_)))
    {}

    inline void initializeSimulationAdapter(std::size_t nQubits)
    {
        initializeSimulation(nQubits);
    }

    inline void applyOperationToStateAdapter(std::unique_ptr<qc::Operation>&& op)
    {
        applyOperationToState(op);
    }

    inline char measureAdapter(dd::Qubit i) 
    {
        return measure(i);
    }

    JSON simulate(std::size_t shots, comm::ClassicalChannel* classical_channel = nullptr); // TODO: override?

private:

    void apply_gate_(const JSON& instruction, std::unique_ptr<qc::StandardOperation>&& std_op, std::map<std::size_t, bool>& classic_reg, std::map<std::size_t, bool>& r_classic_reg);
    std::string execute_shot_(comm::ClassicalChannel* classical_channel, const std::vector<QuantumTask>& quantum_tasks);
    void generate_entanglement_(const int& n_qubits);
    
};

} // End of sim namespace
} // End of cunqa namespace