#include "executor.hpp"


namespace {

std::string execute_shot_(
    const std::unique_ptr<cunqa::sim::Simulator>& sim,
    const InstructionStream& stream
)
{
    while (stream.has_next())
    {
        auto instr = stream.next();
        switch (instr.tag)
        {
            case InstructionTag::CIF:
                if (sim->creg[instr.clbits[0]])
                    stream.push(block);
                break;
            case InstructionTag::SEND:
                for (int i=0; i<= instr.clbits.size(); i++)
                    classical_channel->send_measure(sim->creg[clbits[i]], instr.qpus[i]);
                break;
            case InstructionTag::RECV:
                for (int i=0; i<= instr.clbits.size(); i++)
                    sim->creg[instr.clbits[i]] = classical_channel->recv_measure(instr.qpus[i]);
                break;
            default:
                sim->apply_gate(instr);
        } // End switch
    };  
}

} // End of anonymous namespace


namespace cunqa {
namespace sim {

CCExecutor::CCExecutor(std::unique_ptr<Simulator> simulator) : 
    simulator_{simulator},
    classical_channel_{std::getenv("SLURM_JOB_ID") + "_"s + std::getenv("SLURM_TASK_PID")}
{
    classical_channel_.publish();
};

JSON CCExecutor::execute(const QuantumTask& quantum_task) 
{
    std::map<std::string, std::size_t> meas_counter;
    
    for(const auto& qpu_id: quantum_task.sending_to)
        classical_channel_.connect(qpu_id);
    simulator_.set_config(quantum_task.config);
    
    auto start = std::chrono::high_resolution_clock::now()
    reg_t qubit_ids;
    for (std::size_t i = 0; i < shots; i++) {
        simulator_.initialize();
        meas_counter[execute_shot_(&state, qc.quantum_tasks, classical_channel, allows_qc, n_comm_qubits)]++;
        simulator_.clear();
    } // End all shots
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