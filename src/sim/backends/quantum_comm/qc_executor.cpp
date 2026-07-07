#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <thread> 
#include <chrono>

#include "qc_executor.hpp"
#include "sim/run_config.hpp"
#include "dynamic_circuit/instruction_type.hpp"
#include "dynamic_circuit/dynamic_circuit.hpp"
#include "utils/helpers/environment.hpp"
#include "utils/constants.hpp"

#include "comm_managers/qc_manager.hpp"
#include "comm_managers/cc_manager.hpp"

#include "logger.hpp"

namespace {

using MeasCounter = std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>;

enum class BlockType {
    NO_BLOCK,
    BY_ENT,
    BY_CC
};

struct QuantumTask {
    std::string id;
    cunqa::RunConfig config;
    std::unique_ptr<cunqa::DynamicCircuit> circuit;

    QuantumTask(std::string id, cunqa::RunConfig config, std::unique_ptr<cunqa::DynamicCircuit> circuit)
        : id(std::move(id))
        , config(std::move(config))
        , circuit(std::move(circuit))
    { };
};

struct InstructionStream 
{
    const std::vector<QuantumTask>& quantum_tasks;
    std::size_t num_quantum_tasks;
    std::vector<bool> finished;
    std::vector<BlockType> blocked;
    std::size_t pointed_quantum_task;
    std::vector<std::size_t> pointed_instruction;
    std::size_t steps_since_progress{0};

    InstructionStream(
        const std::vector<QuantumTask>& quantum_tasks_,
        const std::vector<std::size_t>& zero_clbits_,
        const std::vector<std::size_t>& zero_qubits_
    )
        : quantum_tasks{quantum_tasks_}
        , num_quantum_tasks{quantum_tasks_.size()}
        , finished(quantum_tasks_.size(), false)
        , blocked(quantum_tasks_.size(), BlockType::NO_BLOCK)
        , pointed_quantum_task{0}
        , pointed_instruction(quantum_tasks_.size(), 0)
    {
        for (std::size_t t = 0; t < num_quantum_tasks; t++) {
            for (auto& instr : quantum_tasks[t].circuit->instructions) {
                std::visit([&](auto& payload) {
                    using T = std::decay_t<decltype(payload)>;

                    if constexpr (!std::is_same_v<T, std::monostate>) {
                        if constexpr (requires { payload.qubits; }) {
                            for (auto& q : payload.qubits) {
                                q += zero_qubits_[t];
                            }
                        } else if constexpr (requires { payload.qubit; }) {
                            payload.qubit += zero_qubits_[t];
                        }

                        if constexpr (requires { payload.clbits; }) {
                            for (auto& c : payload.clbits)
                                c += zero_clbits_[t];
                        } else if constexpr (requires { payload.clbit; }) {
                            payload.clbit += zero_clbits_[t];
                        } else if constexpr (std::is_same_v<T, cunqa::Copy>) {
                            for (std::size_t i = 0; i < payload.l_clbits.size(); i++) {
                                payload.l_clbits[i] += zero_clbits_[t];
                                payload.r_clbits[i] += zero_clbits_[t];
                            }
                        }
                    }
                    if constexpr (std::is_same_v<T, cunqa::ClassicalComm>) {
                        if (instr.type == cunqa::InstructionType::SEND)
                            payload.qpus.insert(payload.qpus.begin(), quantum_tasks[t].config.qpu_id);
                        else
                            payload.qpus.push_back(quantum_tasks[t].config.qpu_id);
                        payload.qpus = std::vector<std::string>{std::accumulate(
                                payload.qpus.begin(),
                                payload.qpus.end(),
                                std::string{},
                                [](std::string acc, const std::string& s) {
                                    return acc.empty() ? s : acc + "|" + s;
                                }
                            )};
                    }
                    if constexpr (std::is_same_v<T, cunqa::GenEnt>) {
                        if (payload.tag == "NO_TAG") {
                            std::sort(payload.qpus.begin(), payload.qpus.end());
                            payload.tag = std::accumulate(
                                payload.qpus.begin(),
                                payload.qpus.end(),
                                std::string{},
                                [](std::string acc, const std::string& s) {
                                    return acc.empty() ? s : acc + "|" + s;
                                }
                            );
                        }
                    }
                }, instr.payload);
            }
        }
    }

    inline void reset()
    {
        std::fill(finished.begin(), finished.end(), false);
        std::fill(blocked.begin(), blocked.end(), BlockType::NO_BLOCK);
        pointed_quantum_task = 0;
        steps_since_progress = 0;
        std::fill(pointed_instruction.begin(), pointed_instruction.end(), 0);
    }

    inline void complete_and_advance()
    {
        const auto total = quantum_tasks[pointed_quantum_task].circuit->instructions.size();

        if (blocked[pointed_quantum_task] == BlockType::NO_BLOCK) {
            pointed_instruction[pointed_quantum_task]++;
            steps_since_progress = 0; // made progress
        } else {
            if (++steps_since_progress >= static_cast<std::size_t>(std::count(finished.begin(), finished.end(), false)))
                throw std::runtime_error(
                    "Deadlock: every pending quantum task is blocked "
                    "(circular SEND/RECV or unsatisfiable entanglement).");
        }

        if (pointed_instruction[pointed_quantum_task] >= total)
            finished[pointed_quantum_task] = true;

        if (!all_finished()) {
            do {
                pointed_quantum_task = (pointed_quantum_task + 1) % num_quantum_tasks;
            } while (finished[pointed_quantum_task]);
        }
    }

    inline const cunqa::Instruction& get_current() noexcept
    {
        return quantum_tasks[pointed_quantum_task]
                    .circuit->instructions[pointed_instruction[pointed_quantum_task]];
    }

    inline cunqa::InstructionType skip_next() 
    {
        pointed_instruction[pointed_quantum_task]++;
        if (pointed_instruction[pointed_quantum_task] == quantum_tasks[pointed_quantum_task].circuit->instructions.size())
            throw std::runtime_error("You reached the end of the instruction stream skipping.");
        
        return quantum_tasks[pointed_quantum_task]
                    .circuit->instructions[pointed_instruction[pointed_quantum_task]].type;
    }

    inline void block_current(BlockType block_type) noexcept
    {
        blocked[pointed_quantum_task] = block_type;
    }

    inline void unblock_current() noexcept
    {
        blocked[pointed_quantum_task] = BlockType::NO_BLOCK;
    }

    inline bool is_current_blocked() const noexcept
    {
        return (blocked[pointed_quantum_task] != BlockType::NO_BLOCK);
    }

    inline bool all_finished() const
    {
        return std::all_of(finished.begin(), finished.end(), [](bool b) { return b; });
    }
};

void execute_shot_(
    std::shared_ptr<cunqa::sim::Simulator> simulator, 
    InstructionStream& stream,
    const std::vector<std::vector<std::size_t>> communication_qubits_
)
{
    cunqa::QuantumCommManager qc_manager{communication_qubits_};
    cunqa::ClassicalCommManager cc_manager;
    while(!stream.all_finished()) {
        const auto& instr = stream.get_current();
        
        std::visit([&](const auto& payload) {
            using T = std::decay_t<decltype(payload)>;
            const auto& type = instr.type;

            if constexpr (std::is_same_v<T, cunqa::ClassicalIf>) {
                if (type == cunqa::InstructionType::CIF){
                    // Operates on the values provided, with the specified operation. 
                    //If condition is 0, the operation negates the second operand (check cif_ops)
                    bool result = std::accumulate(
                        payload.clbits.begin() + 1, payload.clbits.end(), 
                        simulator->creg[payload.clbits[0]], // Starting value
                        [&](bool acc, std::size_t clbit) { 
                            return cunqa::cif_ops[payload.operation](acc, simulator->creg[clbit]); 
                        });

                    // If the condition is not met, skip every gate until the
                    // matching ENDCIF arrives.
                    // NOTE: nesting of CIF blocks is NOT supported. This stops at
                    // the first ENDCIF, so a nested block would be mis-handled.
                    // Circuits must not contain nested CIF blocks.
                    if (result != payload.condition){
                        cunqa::InstructionType type;
                        do {
                            type = stream.skip_next();
                        } while (type != cunqa::InstructionType::ENDCIF);
                    }
                } 
            } else if constexpr (std::is_same_v<T, cunqa::ClassicalComm>) {
                if (type == cunqa::InstructionType::SEND) {
                    for (std::size_t i=0; i<payload.clbits.size(); i++) 
                        cc_manager.send(simulator->creg[payload.clbits[i]], payload.qpus[0]);
                } else {
                    bool value_i;
                    for (std::size_t i=0; i<payload.clbits.size(); i++) {
                        if (cc_manager.recv(payload.qpus[0], value_i)) {
                            simulator->creg[payload.clbits[i]] = value_i;
                            simulator->save_clbit[payload.clbits[i]] = false;
                            if (stream.is_current_blocked())
                                stream.unblock_current();
                        } else
                            stream.block_current(BlockType::BY_CC);
                    }
                }
            } else if constexpr (std::is_same_v<T, cunqa::GenEnt>) {
                qc_manager.get_ghz(simulator, payload);
            } else if constexpr (std::is_same_v<T, std::monostate>) {
                throw std::runtime_error("Empty circuit received.");
            }
            else {
                if constexpr (requires { payload.qubits; }) {
                    for (auto& q : payload.qubits) {
                        if (qc_manager.is_pending.contains(q) && qc_manager.is_pending.at(q)) {
                            stream.block_current(BlockType::BY_ENT);
                            return;
                        }
                    }
                } else if constexpr (requires { payload.qubit; }) {
                    if (qc_manager.is_pending.contains(payload.qubit) && qc_manager.is_pending.at(payload.qubit)) {
                        stream.block_current(BlockType::BY_ENT);
                        return;
                    }
                } 
                if (stream.is_current_blocked())
                    stream.unblock_current();
                simulator->apply_gate(type, payload);
            }
                
        }, instr.payload);
        stream.complete_and_advance();
    }
}

void update_measures_(
    std::shared_ptr<cunqa::sim::Simulator> simulator,
    const std::vector<QuantumTask>& quantum_tasks,
    const std::vector<std::size_t>& zero_clbits,
    MeasCounter& meas_counter
)
{
    for (std::size_t t = 0; t < quantum_tasks.size(); ++t) {
        std::string result;

        const auto begin = zero_clbits[t];
        const auto end = begin + quantum_tasks[t].config.num_clbits;

        for (std::size_t c = begin; c < end; ++c) {
            auto it = simulator->creg.find(c);
            if (it != simulator->creg.end()) {
                if (simulator->save_clbit.at(c))
                    result = (it->second ? '1' : '0') + result;
            }
        }
        if (result != "")
            meas_counter[quantum_tasks[t].config.qpu_id][result]++;
    }
}

template <typename Predicate>
void poll_until_(Predicate&& ready,
                 const char* what,
                 std::size_t max_attempts = 600,
                 std::chrono::milliseconds interval = std::chrono::milliseconds{100})
{
    for (std::size_t attempt = 0; attempt < max_attempts; ++attempt) {
        if (ready())
            return;
        std::this_thread::sleep_for(interval);
    }
    throw std::runtime_error(std::string("Timed out waiting for ") + what);
}

} // End of anonymous namespace


namespace cunqa {
namespace sim {

QCExecutor::QCExecutor(
    std::shared_ptr<Simulator> simulator,
    const std::size_t& n_qpus
) 
    : simulator_{simulator}
    , classical_channel_{get_env_variable("SLURM_JOB_ID") + "_executor"}
{
    auto slurm_job_id = get_env_variable("SLURM_JOB_ID");

    poll_until_([&] {
        qpus_ids_.clear();
        const auto communications = read_file(COMM_FILEPATH);
        for (const auto& [qpu_id, value] : communications.items())
            if (slurm_job_id == qpu_id.substr(0, qpu_id.find('_')))
                qpus_ids_.push_back(qpu_id);
        return qpus_ids_.size() == n_qpus;
    }, "QPU communication entries");

    classical_channel_.publish();
    for (const auto& qpu_id: qpus_ids_) {
        classical_channel_.connect(qpu_id);
        classical_channel_.send_info(std::string("ready"), qpu_id);
    }

    JSON backends;
    poll_until_([&] {
        backends.clear();
        const auto all_backends = read_file(QPUS_FILEPATH);
        for (const auto& [qpu_id, value] : all_backends.items()) {
            if (std::find(qpus_ids_.begin(), qpus_ids_.end(), qpu_id) != qpus_ids_.end())
                backends[qpu_id] = value;
        }
        return backends.size() == qpus_ids_.size();
    }, "QPUs backend definition");
    
    std::size_t accumulated_qubits{0};
    for (const auto& qpu_id : qpus_ids_) {
        zero_qubits_.push_back(accumulated_qubits);

        const std::size_t n_computation_qubits = backends.at(qpu_id).at("backend").at("num_qubits")[0];
        const std::size_t n_communication_qubits = backends.at(qpu_id).at("backend").at("num_qubits")[1];

        const std::size_t comm_start = accumulated_qubits + n_computation_qubits;
        accumulated_qubits = comm_start + n_communication_qubits;

        std::vector<std::size_t> comm_qubits(n_communication_qubits);
        std::iota(comm_qubits.begin(), comm_qubits.end(), comm_start);
        communication_qubits_.push_back(std::move(comm_qubits));
    }
    simulator->num_qubits = accumulated_qubits;
};

void QCExecutor::run()
{
    while (true) { 
        std::vector<std::size_t> zero_clbits_;
        std::vector<QuantumTask> quantum_tasks;
        {
            std::size_t accumulated_clbits{0};
            for(const auto& qpu_id : qpus_ids_) {
                auto message = JSON::parse(classical_channel_.recv_info(qpu_id));
                if(!message.empty()) {
                    RunConfig run_config(message.at("config"));
                    auto dynamic_circuit = std::make_unique<cunqa::DynamicCircuit>(message.at("instructions"));
                    const auto num_clbits = run_config.num_clbits;
                    QuantumTask quantum_task(
                        message.at("id").get<std::string>(), 
                        run_config, 
                        std::move(dynamic_circuit)
                    );
                    quantum_tasks.push_back(std::move(quantum_task));
                    zero_clbits_.push_back(accumulated_clbits);
                    accumulated_clbits += num_clbits;
                }
            }
        }
        
        if (quantum_tasks.empty()) {
            LOGGER_INFO("Nothing to run: empty circuits sent to executor.");
            continue;   // nothing to run this round
        }

        {
            std::vector<RunConfig> run_configs;
            for (const auto& quantum_task: quantum_tasks)
                run_configs.push_back(quantum_task.config);
            simulator_->config = RunConfig::combine_configs(run_configs);
        }

        MeasCounter meas_counter;
        InstructionStream stream(quantum_tasks, zero_clbits_, zero_qubits_);

        auto start = std::chrono::high_resolution_clock::now();
        for (std::size_t i=0; i<simulator_->config.shots; i++) {
            simulator_->initialize();

            execute_shot_(simulator_, stream, communication_qubits_);
            stream.reset();

            update_measures_(simulator_, quantum_tasks, zero_clbits_, meas_counter);
            simulator_->clear();
        } // End of all shots
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = end - start;
        float time_taken = duration.count();

        for(const auto& quantum_task: quantum_tasks) {
            JSON qpu_result = {
                {"counts", meas_counter[quantum_task.config.qpu_id]},
                {"time_taken", time_taken}
            };
            classical_channel_.send_info(qpu_result.dump(), quantum_task.config.qpu_id);
        }
    }
}

} // End of sim namespace
} // End of cunqa namespace