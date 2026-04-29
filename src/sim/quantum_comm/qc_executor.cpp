#include "executor.hpp"


namespace {

using MeasCounter = std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>;

std::string execute_shot_(
    const std::unique_ptr<cunqa::sim::Simulator>& sim,
    const InstructionStream& stream
)
{
    while (stream.has_next())
    {
        auto instr = stream.next();
        switch (instr.type)
        {
            case cunqa::CIF:
            {
                if (sim->creg[instr.clbits[0]]) {
                    stream.push(block);
                }
                break;
            }
            case cunqa::SEND:
                break;
            case cunqa::RECV:
                break;
            case cunqa::QSEND:
                break;
            case cunqa::QRECV:
                break;
            case cunqa::EXPOSE:
                break;
            case cunqa::RCONTROL:
                break;
            default:
                sim->apply_gate(instr);
        } // End switch
    };  
}

void update_meas(std::unique_ptr<Simulator> simulator, MeasCounter& meas_counter)
{
    // TODO 
}

} // End of anonymous namespace


namespace cunqa {
namespace sim {

QCExecutor::QCExecutor(std::unique_ptr<Simulator> simulator) : 
    simulator_{simulator},
{ }

JSON QCExecutor::execute(const std::vector<QuantumTask>& quantum_tasks)
{
    MeasCounter meas_counter;
    
    simulator_.update_config(quantum_task.config);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < shots; i++) {
        simulator_->initialize();
        execute_shot_(simulator_, stream);
        update_meas(simulator_, meas_counter);
        simulator_->clear();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    float time_taken = duration.count();

    JSON result_json = {
        {"counts", meas_counter},
        {"time_taken", time_taken}};
    return result_json;
}

} // End of sim namespace
} // End of cunqa namespace