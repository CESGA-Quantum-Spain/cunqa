#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <iterator>
#include <limits>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "qc_engine.hpp"
#include "dynamic_circuit/instruction_type.hpp"
#include "sim/noise/channel_applier.hpp"
#include "sim/noise/channels.hpp"
#include "sim/random_delay.hpp"
#include "sim/timing.hpp"

#include "comm_managers/qc_manager.hpp"
#include "comm_managers/cc_manager.hpp"

#include "logger.hpp"

namespace {

using namespace cunqa;
using namespace cunqa::sim;

using MeasCounter = std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>;

enum class BlockType {
    NO_BLOCK,
    BY_ENT,
    BY_CC
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

// Shot without simulated time: ideal, instantaneous entanglement.
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
                    double send_time_i;
                    for (std::size_t i=0; i<payload.clbits.size(); i++) {
                        if (cc_manager.recv(payload.qpus[0], value_i, send_time_i)) {
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


// ---------------------------------------------------------------------------------------
// Simulated time
// ---------------------------------------------------------------------------------------

// Random delays (averaged strategy) that a simulated time depends on: ids of
// TimedShot::delays_, sorted. Empty with the exact strategy, where delays are sampled.
using DelayIds = std::vector<std::uint32_t>;
const DelayIds NO_DELAYS;

DelayIds with_delay(DelayIds ids, std::uint32_t id)
{
    ids.insert(std::upper_bound(ids.begin(), ids.end(), id), id);
    return ids;
}

DelayIds with_delays(const DelayIds& a, const DelayIds& b)
{
    DelayIds ids;
    std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(ids));
    return ids;
}

// The latest of several times and the random delays of the one that sets it. As in
// SimClock, the first one considered wins ties.
struct Latest {
    double time = -std::numeric_limits<double>::infinity();
    const DelayIds* delays = &NO_DELAYS;

    void consider(double t, const DelayIds& t_delays)
    {
        if (t > time) {
            time = t;
            delays = &t_delays;
        }
    }
};

struct TaskInfo {
    std::size_t vqpu;
    std::string qpu_id;
    std::size_t first_qubit;
    std::size_t num_data_qubits;
    std::size_t num_qubits;
    std::size_t first_clbit;
    std::size_t num_clbits;
};

struct TimedMessage {
    bool value;
    double send_time;
    DelayIds delays;
};

struct EntanglementRequest {
    std::size_t task;
    std::size_t qubit;
    double time;
    DelayIds delays;
};

template <typename Payload>
std::vector<std::size_t> qubits_of(const Payload& payload)
{
    if constexpr (requires { payload.qubits; })
        return std::vector<std::size_t>(payload.qubits.begin(), payload.qubits.end());
    else if constexpr (requires { payload.qubit; })
        return {payload.qubit};
    else
        return {};
}

// One shot of the quantum-communications scheme in simulated time. The clocks (one per
// vQPU) give the timeline; with a noise model (applier), the time a qubit waits is turned
// into relaxation through their idle hook, and entangled pairs come from the generator of
// each link.
class TimedShot {
public:
    TimedShot(
        Simulator& simulator,
        const CommModel& model,
        const std::vector<TaskInfo>& tasks,
        std::vector<SimClock>& clocks,
        const std::vector<const noise::Coherence*>& coherence,
        noise::ChannelApplier* applier,
        Rng& rng
    )
        : sim_{simulator}
        , model_{model}
        , tasks_{tasks}
        , clocks_{clocks}
        , coherence_{coherence}
        , applier_{applier}
        , rng_{rng}
        , noisy_{applier != nullptr}
        , averaged_{applier != nullptr && model.strategy == WaitStrategy::AVERAGED}
        , qubit_delays_(coherence.size())
        , condition_delays_(tasks.size())
        , requests_(tasks.size(), 0)
        , generation_time_(tasks.size(), 0.0)
    {
        if (noisy_) {
            for (auto& clock : clocks_)
                clock.idle_hook = [this](std::size_t qubit, double start, double duration) {
                    idle_(qubit, start, duration);
                };
        }
    }

    ~TimedShot()
    {
        for (auto& clock : clocks_)
            clock.idle_hook = nullptr;
    }

    TimedShot(const TimedShot&) = delete;
    TimedShot& operator=(const TimedShot&) = delete;

    void run(InstructionStream& stream)
    {
        while (!stream.all_finished()) {
            const auto& instr = stream.get_current();
            const auto task = stream.pointed_quantum_task;

            std::visit([&](const auto& payload) {
                using T = std::decay_t<decltype(payload)>;
                const auto& type = instr.type;

                if constexpr (std::is_same_v<T, ClassicalIf>) {
                    if (type == InstructionType::CIF) {
                        conditional_(stream, task, payload);
                    } else {
                        clock_(task).end_conditional();
                        condition_delays_[task].clear();
                    }
                } else if constexpr (std::is_same_v<T, ClassicalComm>) {
                    if (type == InstructionType::SEND)
                        send_(task, payload);
                    else
                        recv_(stream, task, payload);
                } else if constexpr (std::is_same_v<T, GenEnt>) {
                    request_entanglement_(task, payload);
                } else if constexpr (std::is_same_v<T, std::monostate>) {
                    throw std::runtime_error("Empty circuit received.");
                } else {
                    const auto qubits = qubits_of(payload);
                    for (const auto q : qubits) {
                        if (pending_.contains(q)) {
                            stream.block_current(BlockType::BY_ENT);
                            return;
                        }
                    }
                    if (stream.is_current_blocked())
                        stream.unblock_current();
                    operation_(task, type, payload, qubits);
                }
            }, instr.payload);
            stream.complete_and_advance();
        }
    }

    std::size_t requests(std::size_t task) const { return requests_[task]; }
    double generation_time(std::size_t task) const { return generation_time_[task]; }

private:
    Simulator& sim_;
    const CommModel& model_;
    const std::vector<TaskInfo>& tasks_;
    std::vector<SimClock>& clocks_;
    const std::vector<const noise::Coherence*>& coherence_;
    noise::ChannelApplier* applier_;
    Rng& rng_;
    const bool noisy_;
    const bool averaged_;

    std::unordered_map<std::string, std::deque<TimedMessage>> messages_;
    std::unordered_map<std::string, std::vector<EntanglementRequest>> waiting_;
    std::unordered_set<std::size_t> pending_;

    // Averaged strategy: the random delays of the shot, and those that the time at which
    // each qubit and clbit is available, and each open condition, depend on.
    std::vector<RandomDelayPtr> delays_;
    std::vector<DelayIds> qubit_delays_;
    std::unordered_map<std::size_t, DelayIds> clbit_delays_;
    std::vector<DelayIds> condition_delays_;
    // Delays of the time up to which the clock is about to report idle qubits.
    const DelayIds* to_delays_ = &NO_DELAYS;

    std::vector<std::size_t> requests_;
    std::vector<double> generation_time_;

    SimClock& clock_(std::size_t task) { return clocks_[tasks_[task].vqpu]; }
    const VQPUModel& vqpu_(std::size_t task) const { return model_.vqpus[tasks_[task].vqpu]; }

    const DelayIds& clbit_delays_of_(std::size_t clbit) const
    {
        const auto it = clbit_delays_.find(clbit);
        return it != clbit_delays_.end() ? it->second : NO_DELAYS;
    }

    // Relaxation of a qubit idling from `from` to `to`.
    noise::Relaxation idle_relaxation_(
        const noise::Coherence& coherence,
        double from, const DelayIds& from_delays,
        double to, const DelayIds& to_delays
    ) const
    {
        const double length = std::max(0.0, to - from);
        if (from_delays.empty() && to_delays.empty())
            return noise::relaxation(coherence, length);

        // Random delays that stretch the interval (only its end depends on them) and that
        // shrink it (only its start does). Delays shared by both ends cancel out.
        DelayIds stretch, shrink;
        std::set_difference(to_delays.begin(), to_delays.end(), from_delays.begin(), from_delays.end(),
                            std::back_inserter(stretch));
        std::set_difference(from_delays.begin(), from_delays.end(), to_delays.begin(), to_delays.end(),
                            std::back_inserter(shrink));

        // Deterministic part of the interval, with every random delay removed.
        double deterministic = length;
        for (const auto id : stretch)
            deterministic -= delays_[id]->mean();
        for (const auto id : shrink)
            deterministic += delays_[id]->mean();

        auto by_mean = [this](std::uint32_t a, std::uint32_t b) { return delays_[a]->mean() < delays_[b]->mean(); };

        auto decay = [&](double rate) {
            if (rate == 0.0)
                return 1.0;
            if (stretch.empty() && shrink.empty())
                return std::exp(-rate * std::max(0.0, deterministic));

            if (stretch.empty()) {
                // Interval max(0, d - sum Y): the largest delay is kept random, the rest at
                // their mean.
                const auto main = *std::max_element(shrink.begin(), shrink.end(), by_mean);
                double offset = deterministic;
                for (const auto id : shrink)
                    if (id != main)
                        offset -= delays_[id]->mean();
                return delays_[main]->deficit_laplace(rate, offset);
            }

            // Interval max(0, d + sum X - sum Y), with the shrinking delays at their mean.
            double offset = deterministic;
            for (const auto id : shrink)
                offset -= delays_[id]->mean();
            if (offset >= 0.0) {
                double value = std::exp(-rate * offset);
                for (const auto id : stretch)
                    value *= delays_[id]->laplace(rate);
                return value;
            }
            const auto main = *std::max_element(stretch.begin(), stretch.end(), by_mean);
            for (const auto id : stretch)
                if (id != main)
                    offset += delays_[id]->mean();
            return delays_[main]->excess_laplace(rate, -offset);
        };

        return {decay(coherence.rate1()), decay(coherence.rate2())};
    }

    // Idle hook of the clocks: the qubit waited `duration` from `start`.
    void idle_(std::size_t qubit, double start, double duration)
    {
        const auto* coherence = coherence_[qubit];
        if (!coherence || !coherence->decoheres())
            return;
        applier_->relaxation(qubit, averaged_
            ? idle_relaxation_(*coherence, start, qubit_delays_[qubit], start + duration, *to_delays_)
            : noise::relaxation(*coherence, duration));
    }

    // A random delay added to a time: sampled with the exact strategy, its mean (and
    // remembered) with the averaged one.
    double delayed_(double time, DelayIds& delays, const RandomDelayPtr& delay)
    {
        if (!delay)
            return time;
        if (!averaged_)
            return time + bin_delay(delay->sample(rng_), model_.time_bin);
        if (!delay->is_deterministic()) {
            delays = with_delay(std::move(delays), static_cast<std::uint32_t>(delays_.size()));
            delays_.push_back(delay);
        }
        return time + delay->mean();
    }

    template <typename Payload>
    void operation_(std::size_t task, InstructionType type, const Payload& payload, const std::vector<std::size_t>& qubits)
    {
        auto& clock = clock_(task);

        DelayIds start_delays;
        std::vector<DelayIds> copied_delays;
        if (averaged_) {
            Latest start;
            start.consider(clock.condition_time(), condition_delays_[task]);
            for (const auto q : qubits)
                start.consider(clock.qubit_ready(q), qubit_delays_[q]);
            start_delays = *start.delays;
            if constexpr (std::is_same_v<Payload, Copy>) {
                for (const auto c : payload.r_clbits) {
                    Latest copied;
                    copied.consider(clock.condition_time(), condition_delays_[task]);
                    copied.consider(clock.clbit_time(c), clbit_delays_of_(c));
                    copied_delays.push_back(*copied.delays);
                }
            }
        }

        // The clock goes first: the idle noise of the qubits comes before the operation.
        to_delays_ = &start_delays;
        clock.apply(type, payload);
        to_delays_ = &NO_DELAYS;
        sim_.apply_gate(type, payload);

        const auto& vqpu = vqpu_(task);
        if constexpr (std::is_same_v<Payload, Measure>) {
            if (noisy_ && vqpu.device && applier_->flip(vqpu.device->readout_error(payload.qubit - vqpu.first_qubit)))
                sim_.creg[payload.clbit] = !sim_.creg[payload.clbit];
            if (averaged_)
                clbit_delays_[payload.clbit] = start_delays;
        } else if constexpr (std::is_same_v<Payload, Copy>) {
            if (averaged_)
                for (std::size_t i = 0; i < payload.l_clbits.size(); ++i)
                    clbit_delays_[payload.l_clbits[i]] = copied_delays[i];
        } else if constexpr (!std::is_same_v<Payload, Reset>) {
            if (noisy_ && vqpu.device)
                gate_error_(vqpu, type, qubits);
        }

        if (averaged_)
            for (const auto q : qubits)
                qubit_delays_[q] = start_delays;
    }

    // Error of the gate in the noise properties of the vQPU: depolarizing, then relaxation
    // over the gate duration.
    void gate_error_(const VQPUModel& vqpu, InstructionType type, const std::vector<std::size_t>& qubits)
    {
        std::vector<std::size_t> local(qubits);
        for (auto& q : local)
            q -= vqpu.first_qubit;
        const auto* gate_noise = vqpu.device->gate(std::string(instruction_type_name(type)), local);
        if (!gate_noise)
            return;

        applier_->depolarizing(qubits, gate_noise->depolarizing);
        if (gate_noise->relaxation) {
            for (const auto q : qubits)
                if (const auto* coherence = coherence_[q])
                    applier_->relaxation(q, noise::relaxation(*coherence, gate_noise->duration));
        }
    }

    void conditional_(InstructionStream& stream, std::size_t task, const ClassicalIf& payload)
    {
        const bool result = std::accumulate(
            payload.clbits.begin() + 1, payload.clbits.end(),
            sim_.creg[payload.clbits[0]],
            [&](bool acc, std::size_t clbit) {
                return cif_ops[payload.operation](acc, sim_.creg[clbit]);
            });

        auto& clock = clock_(task);
        DelayIds decided_delays;
        if (averaged_) {
            Latest decided;
            decided.consider(clock.condition_time(), condition_delays_[task]);
            for (const auto c : payload.clbits)
                decided.consider(clock.clbit_time(c), clbit_delays_of_(c));
            decided_delays = *decided.delays;
        }

        if (result != payload.condition) {
            // Nested blocks are not supported: this stops at the first ENDCIF.
            std::vector<std::size_t> qubits;
            InstructionType skipped;
            do {
                skipped = stream.skip_next();
                if (skipped != InstructionType::ENDCIF) {
                    const auto skipped_qubits = instruction_qubits(stream.get_current().payload);
                    qubits.insert(qubits.end(), skipped_qubits.begin(), skipped_qubits.end());
                }
            } while (skipped != InstructionType::ENDCIF);

            // The qubits of the block wait for the decision, as if it had been applied.
            std::vector<std::size_t> waiting;
            if (averaged_) {
                const double decided = clock.decision_time(payload.clbits);
                for (const auto q : qubits)
                    if (decided > clock.qubit_ready(q))
                        waiting.push_back(q);
            }
            to_delays_ = &decided_delays;
            clock.skip_conditional(payload.clbits, qubits);
            to_delays_ = &NO_DELAYS;
            for (const auto q : waiting)
                qubit_delays_[q] = decided_delays;
        } else {
            clock.begin_conditional(payload.clbits);
            if (averaged_)
                condition_delays_[task] = decided_delays;
        }
    }

    void send_(std::size_t task, const ClassicalComm& payload)
    {
        auto& clock = clock_(task);
        auto& queue = messages_[payload.qpus[0]];
        for (const auto c : payload.clbits) {
            TimedMessage message{static_cast<bool>(sim_.creg[c]), clock.classical_time({c}), {}};
            if (averaged_) {
                Latest sent;
                sent.consider(clock.condition_time(), condition_delays_[task]);
                sent.consider(clock.clbit_time(c), clbit_delays_of_(c));
                message.delays = *sent.delays;
            }
            queue.push_back(std::move(message));
        }
    }

    void recv_(InstructionStream& stream, std::size_t task, const ClassicalComm& payload)
    {
        auto& queue = messages_[payload.qpus[0]];
        if (queue.size() < payload.clbits.size()) {
            stream.block_current(BlockType::BY_CC);
            return;
        }

        // The bits of a RECV travel in one message, so they share its jitter.
        auto& clock = clock_(task);
        DelayIds jitter_delays;
        const double jitter = delayed_(0.0, jitter_delays, clock.model().classical_jitter());
        for (const auto c : payload.clbits) {
            auto message = std::move(queue.front());
            queue.pop_front();
            sim_.creg[c] = message.value;
            sim_.save_clbit[c] = false;
            if (averaged_) {
                const auto arrival_delays = with_delays(message.delays, jitter_delays);
                Latest arrival;
                arrival.consider(clock.condition_time(), condition_delays_[task]);
                arrival.consider(message.send_time + clock.model().classical_latency() + jitter, arrival_delays);
                clbit_delays_[c] = *arrival.delays;
            }
            clock.receive_clbit(c, message.send_time, jitter);
        }
        if (stream.is_current_blocked())
            stream.unblock_current();
    }

    void request_entanglement_(std::size_t task, const GenEnt& payload)
    {
        // The pair is requested on demand: once the communication qubit is free and the
        // vQPU has finished the work issued before the request (its data qubits), so the
        // consumer waits for the whole generation. Requesting earlier in the circuit
        // prefetches the pair, which then waits in memory instead.
        auto& clock = clock_(task);
        const auto& info = tasks_[task];
        Latest requested;
        requested.consider(clock.condition_time(), condition_delays_[task]);
        requested.consider(clock.qubit_ready(payload.qubit), qubit_delays_[payload.qubit]);
        for (std::size_t q = info.first_qubit; q < info.first_qubit + info.num_data_qubits; ++q)
            requested.consider(clock.qubit_ready(q), qubit_delays_[q]);

        auto& waiting = waiting_[payload.tag];
        waiting.push_back({task, payload.qubit, requested.time, *requested.delays});
        if (waiting.size() == payload.qpus.size()) {
            deliver_(waiting);
            waiting.clear();
        } else {
            pending_.insert(payload.qubit);
        }
    }

    // Generates the entangled state requested by every participant: a Bell pair for two of
    // them, a GHZ state assembled from a star of links rooted at the first one otherwise.
    void deliver_(std::vector<EntanglementRequest> participants)
    {
        std::sort(participants.begin(), participants.end(),
            [this](const EntanglementRequest& a, const EntanglementRequest& b) {
                return tasks_[a.task].qpu_id < tasks_[b.task].qpu_id;
            });

        const auto& root = participants.front();
        const auto& root_id = tasks_[root.task].qpu_id;
        const std::size_t n = participants.size();

        // Generation starts when every participant has asked for it, and takes the longest
        // fixed "entanglement" duration of their timing blocks plus the attempts of the
        // generator of each link.
        Latest requested;
        double fixed = 0.0;
        for (const auto& participant : participants) {
            requested.consider(participant.time, participant.delays);
            fixed = std::max(fixed, clock_(participant.task).model().entanglement_duration());
        }
        const double start = requested.time;
        const DelayIds start_delays = *requested.delays;

        std::vector<noise::BellCoefficients> bell(n, {1.0, 0.0, 0.0, 0.0});
        std::vector<double> link_ready(n, start + fixed);
        double ready = start + fixed;
        DelayIds ready_delays = start_delays;

        if (!averaged_) {
            for (std::size_t k = 1; k < n; ++k) {
                const auto draw = model_.epr(root_id, tasks_[participants[k].task].qpu_id).sample(rng_);
                link_ready[k] = start + fixed + bin_delay(draw.delay, model_.time_bin);
                bell[k] = draw.bell;
                ready = std::max(ready, link_ready[k]);
            }
        } else {
            std::vector<RandomDelayPtr> generation;
            for (std::size_t k = 1; k < n; ++k) {
                const auto& epr = model_.epr(root_id, tasks_[participants[k].task].qpu_id);
                generation.push_back(epr.delay());
                bell[k] = epr.mean_bell();
            }
            if (!generation.empty())
                ready = delayed_(ready, ready_delays, (generation.size() == 1)
                    ? generation.front()
                    : DiscreteDelay::max_of(generation));
        }

        // The communication qubits are busy generating the state until it is ready.
        to_delays_ = &start_delays;
        for (const auto& participant : participants)
            clock_(participant.task).occupy(participant.qubit, start, ready);
        to_delays_ = &NO_DELAYS;

        // Heralded state: the memories are re-initialized and loaded with it.
        if (noisy_ && sim_.supports_reset()) {
            for (const auto& participant : participants)
                applier_->reset(participant.qubit);
        }
        sim_.apply_gate(InstructionType::H, OneQubitNoParam{root.qubit});
        for (std::size_t k = 1; k < n; ++k)
            sim_.apply_gate(InstructionType::CX, TwoQubitNoParam{{root.qubit, participants[k].qubit}});

        if (noisy_) {
            for (std::size_t k = 1; k < n; ++k)
                applier_->pauli(participants[k].qubit, noise::bell_to_pauli(bell[k]));

            // A GHZ state is complete when its slowest link heralds; the pairs that arrived
            // earlier have been decohering meanwhile.
            if (n > 2 && !averaged_) {
                const double first = *std::min_element(link_ready.begin() + 1, link_ready.end());
                for (std::size_t k = 0; k < n; ++k) {
                    const auto* coherence = coherence_[participants[k].qubit];
                    const double waited = ready - (k == 0 ? first : link_ready[k]);
                    if (coherence && waited > 0.0)
                        applier_->relaxation(participants[k].qubit, noise::relaxation(*coherence, waited));
                }
            }
        }

        for (const auto& participant : participants) {
            if (averaged_)
                qubit_delays_[participant.qubit] = ready_delays;
            pending_.erase(participant.qubit);
            requests_[participant.task]++;
            generation_time_[participant.task] += ready - start;
        }
    }
};

noise::ChannelMode resolve_channel_mode(const CommModel& model, const Simulator& simulator)
{
    switch (model.noise_mode) {
        case NoiseMode::EXACT:
            return noise::ChannelMode::EXACT;
        case NoiseMode::PAULI:
            return noise::ChannelMode::PAULI;
        default:
            return simulator.supports_kraus() ? noise::ChannelMode::EXACT : noise::ChannelMode::PAULI;
    }
}

JSON run_comm_model(const std::vector<QuantumTask>& quantum_tasks)
{
    JSON run_model;
    for (const auto& task : quantum_tasks) {
        const auto& specifics = task.config.simulator_specifics;
        const JSON model = specifics.contains("comm_model") ? specifics.at("comm_model") : JSON();
        if (&task == &quantum_tasks.front())
            run_model = model;
        else if (model != run_model)
            throw std::runtime_error("Every quantum task with quantum communications has to "
                                     "be run with the same comm_model.");
    }
    return run_model;
}

} // End of anonymous namespace


namespace cunqa {
namespace sim {

QCEngine::QCEngine(std::shared_ptr<Simulator> simulator, CommModel model)
    : simulator_{std::move(simulator)}
    , model_{std::move(model)}
{
    if (!model_.enabled && model_.any_timed() && !model_.all_timed())
        LOGGER_WARN("Simulated time disabled: only some of the vQPUs define a \"timing\" block.");
}

std::vector<JSON> QCEngine::run(std::vector<QuantumTask>& quantum_tasks)
{
    if (quantum_tasks.empty())
        return {};

    const auto model = model_.with_run_overrides(run_comm_model(quantum_tasks));

    // Layout of every task in the shared simulator.
    std::vector<TaskInfo> tasks;
    std::vector<std::size_t> zero_qubits, zero_clbits;
    std::size_t accumulated_clbits = 0;
    for (const auto& quantum_task : quantum_tasks) {
        const auto it = std::find_if(model.vqpus.begin(), model.vqpus.end(),
            [&](const VQPUModel& vqpu) { return vqpu.id == quantum_task.config.qpu_id; });
        if (it == model.vqpus.end())
            throw std::runtime_error("Quantum task for unknown vQPU " + quantum_task.config.qpu_id + ".");

        tasks.push_back({
            static_cast<std::size_t>(it - model.vqpus.begin()),
            it->id,
            it->first_qubit,
            it->num_data_qubits,
            it->num_data_qubits + it->num_comm_qubits,
            accumulated_clbits,
            static_cast<std::size_t>(quantum_task.config.num_clbits)
        });
        zero_qubits.push_back(it->first_qubit);
        zero_clbits.push_back(accumulated_clbits);
        accumulated_clbits += quantum_task.config.num_clbits;
    }

    std::vector<std::vector<std::size_t>> communication_qubits;
    for (const auto& vqpu : model.vqpus) {
        std::vector<std::size_t> comm_qubits(vqpu.num_comm_qubits);
        std::iota(comm_qubits.begin(), comm_qubits.end(), vqpu.first_qubit + vqpu.num_data_qubits);
        communication_qubits.push_back(std::move(comm_qubits));
    }

    InstructionStream stream(quantum_tasks, zero_clbits, zero_qubits);
    MeasCounter meas_counter;

    // Seeded runs give every shot its own seed, drawn from the run seed: re-seeding the
    // simulator with the same value would repeat the same outcomes in every shot.
    const int run_seed = simulator_->config.seed;
    Rng rng(run_seed != NO_SEED ? static_cast<std::uint64_t>(run_seed) : std::random_device{}());
    auto next_shot_seed = [&]() {
        if (run_seed != NO_SEED)
            simulator_->config.seed = static_cast<int>(rng() % 2147483647ULL);
    };

    // Simulated time needs a clock on every vQPU, as they share messages and entangled
    // states. The communication model works in simulated time, so with it the vQPUs
    // without a timing block use the default durations.
    const bool timed = model.enabled || model.all_timed();
    std::vector<SimClock> clocks;
    if (timed) {
        for (const auto& vqpu : model.vqpus)
            clocks.emplace_back(vqpu.timing.value_or(TimingModel(JSON::object())),
                                vqpu.num_data_qubits + vqpu.num_comm_qubits, vqpu.first_qubit);
    }
    std::vector<TimingStats> timing_stats(timed ? tasks.size() : 0);
    std::vector<std::size_t> requests(tasks.size(), 0);
    std::vector<double> generation_time(tasks.size(), 0.0);

    const auto shots = static_cast<std::size_t>(simulator_->config.shots);
    auto start = std::chrono::high_resolution_clock::now();
    if (!timed) {
        for (std::size_t i = 0; i < shots; i++) {
            next_shot_seed();
            simulator_->initialize();

            execute_shot_(simulator_, stream, communication_qubits);
            stream.reset();

            update_measures_(simulator_, quantum_tasks, zero_clbits, meas_counter);
            simulator_->clear();
        }
    } else {
        // Coherence of every qubit of the shared simulator.
        const auto& last = model.vqpus.back();
        std::vector<const noise::Coherence*> coherence(
            last.first_qubit + last.num_data_qubits + last.num_comm_qubits, nullptr);
        std::unique_ptr<noise::ChannelApplier> applier;
        if (model.enabled) {
            double shortest_t2 = noise::NO_DECOHERENCE;
            for (const auto& vqpu : model.vqpus) {
                for (std::size_t q = 0; q < vqpu.num_data_qubits + vqpu.num_comm_qubits; ++q) {
                    coherence[vqpu.first_qubit + q] = vqpu.coherence(q);
                    if (coherence[vqpu.first_qubit + q])
                        shortest_t2 = std::min(shortest_t2, coherence[vqpu.first_qubit + q]->T2);
                }
            }
            if (model.time_bin > 0.01 * shortest_t2)
                LOGGER_WARN("comm_model.time_bin ({} s) is not much smaller than the shortest T2 ({} s): "
                            "binning the waiting times will bias the idle noise.", model.time_bin, shortest_t2);

            applier = std::make_unique<noise::ChannelApplier>(*simulator_, rng, resolve_channel_mode(model, *simulator_));
            if (!simulator_->supports_reset())
                LOGGER_WARN("{} cannot reset qubits: communication qubits are assumed to be in |0> "
                            "when an entangled pair is delivered.", simulator_->get_name());
        }

        for (std::size_t i = 0; i < shots; i++) {
            next_shot_seed();
            simulator_->initialize();
            for (auto& clock : clocks)
                clock.reset();

            TimedShot shot(*simulator_, model, tasks, clocks, coherence, applier.get(), rng);
            shot.run(stream);
            stream.reset();

            update_measures_(simulator_, quantum_tasks, zero_clbits, meas_counter);
            for (std::size_t t = 0; t < tasks.size(); ++t) {
                timing_stats[t].add_shot(clocks[tasks[t].vqpu]);
                requests[t] += shot.requests(t);
                generation_time[t] += shot.generation_time(t);
            }
            simulator_->clear();
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    simulator_->config.seed = run_seed;

    std::vector<JSON> results;
    for (std::size_t t = 0; t < quantum_tasks.size(); ++t) {
        JSON result = {
            {"counts", meas_counter[quantum_tasks[t].config.qpu_id]},
            {"time_taken", duration.count()}
        };
        if (timed)
            result["simulated_time"] = timing_stats[t].to_json();
        if (model.enabled) {
            result["comm_stats"] = {
                {"entanglement_requests_per_shot", shots ? static_cast<double>(requests[t]) / shots : 0.0},
                {"mean_generation_time", requests[t] ? generation_time[t] / requests[t] : 0.0}
            };
        }
        results.push_back(std::move(result));
    }
    return results;
}

} // End of sim namespace
} // End of cunqa namespace
