#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dynamic_circuit/instruction.hpp"
#include "dynamic_circuit/instruction_type.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

// Simulated (logical) time of a vQPU. Every quantity here is expressed in
// seconds of *simulated* time and is completely independent of how long
// the simulation takes on the HPC.
//
// Timing is enabled by adding a "timing" block to the backend JSON:
//
//   "timing": {
//       "one_qubit_gate": 5e-8,     // default duration of 1-qubit gates
//       "multi_qubit_gate": 3e-7,   // default duration of gates on >= 2 qubits
//       "measure": 1e-6,
//       "reset": 1e-6,
//       "classical_latency": 0,     // latency of an incoming classical message
//       "entanglement": 0,          // duration of a GEN_ENT (EPR/GHZ generation)
//       "gate_durations": { "cx": 2.5e-7, "rz": 0 }   // per-gate overrides
//   }
//
// Every field is optional. Noise channels and save_state take no time unless
// they are overridden in "gate_durations".
class TimingModel {
public:
    TimingModel() { by_type_.fill(UNSET); }

    explicit TimingModel(const JSON& timing_json)
        : config_(timing_json) // Not braces: JSON{j} would wrap j in an array
    {
        by_type_.fill(UNSET);

        for (const auto& [key, value] : timing_json.items()) {
            if (std::find(KEYS.begin(), KEYS.end(), key) == KEYS.end())
                throw std::invalid_argument("timing: unknown field '" + key + "'.");
        }

        one_qubit_gate_ = read_duration_(timing_json, "one_qubit_gate", one_qubit_gate_);
        multi_qubit_gate_ = read_duration_(timing_json, "multi_qubit_gate", multi_qubit_gate_);
        classical_latency_ = read_duration_(timing_json, "classical_latency", classical_latency_);
        entanglement_ = read_duration_(timing_json, "entanglement", entanglement_);
        by_type_[index_(InstructionType::MEASURE)] = read_duration_(timing_json, "measure", 1e-6);
        by_type_[index_(InstructionType::RESET)] = read_duration_(timing_json, "reset", 1e-6);

        for (auto type : {InstructionType::AMPLITUDEDAMPINGNOISE, InstructionType::BITFLIPNOISE,
                          InstructionType::DEPHASINGNOISE, InstructionType::DEPOLARIZINGNOISE,
                          InstructionType::INDEPENDENTXZNOISE, InstructionType::TWOQUBITDEPOLARIZINGNOISE,
                          InstructionType::SAVE_STATE})
            by_type_[index_(type)] = 0.0;

        if (timing_json.contains("gate_durations")) {
            for (const auto& [gate, value] : timing_json.at("gate_durations").items())
                by_type_[index_(instruction_type_from_name(gate))] = check_duration_(gate, value.get<double>());
        }
    }

    double duration(InstructionType type, std::size_t n_qubits) const
    {
        auto d = by_type_[index_(type)];
        if (!std::isnan(d))
            return d;
        return n_qubits <= 1 ? one_qubit_gate_ : multi_qubit_gate_;
    }

    double classical_latency() const { return classical_latency_; }
    double entanglement_duration() const { return entanglement_; }

    JSON to_json() const { return config_; }

private:
    static constexpr double UNSET = std::numeric_limits<double>::quiet_NaN();
    static constexpr std::size_t N_TYPES = static_cast<std::size_t>(InstructionType::MEASURE) + 1;
    static constexpr std::array<std::string_view, 7> KEYS = {
        "one_qubit_gate", "multi_qubit_gate", "measure", "reset",
        "classical_latency", "entanglement", "gate_durations"
    };

    JSON config_ = JSON::object();
    std::array<double, N_TYPES> by_type_;
    double one_qubit_gate_ = 5e-8;
    double multi_qubit_gate_ = 3e-7;
    double classical_latency_ = 0.0;
    double entanglement_ = 0.0;

    static constexpr std::size_t index_(InstructionType type) { return static_cast<std::size_t>(type); }

    static double check_duration_(const std::string& key, double value)
    {
        if (!(value >= 0.0) || !std::isfinite(value))
            throw std::invalid_argument("timing: '" + key + "' must be a finite, non-negative duration (s).");
        return value;
    }

    static double read_duration_(const JSON& j, const std::string& key, double fallback)
    {
        return j.contains(key) ? check_duration_(key, j.at(key).get<double>()) : fallback;
    }
};


// Logical clock of one vQPU during one shot. Each qubit and clbit keeps the
// simulated time at which it becomes available, and operations are scheduled
// as soon as possible (ASAP): an operation starts when all of its operands are
// ready, so independent gates run in parallel. The vQPU clock (now()) is the
// latest of those times.
//
// Waiting is tracked per qubit: whenever an operation starts later than the
// moment its qubit became available, the gap is accumulated as idle time and
// reported to idle_hook, so a noise layer can turn it into idle noise. Idle
// time after the last operation of a qubit is not counted.
class SimClock {
public:
    // (qubit, idle start time, idle duration)
    using IdleHook = std::function<void(std::size_t, double, double)>;
    IdleHook idle_hook;

    SimClock(TimingModel model, std::size_t n_qubits, std::size_t qubit_offset = 0)
        : model_{std::move(model)}
        , qubit_offset_{qubit_offset}
        , qubit_ready_(n_qubits, 0.0)
        , idle_(n_qubits, 0.0)
    { }

    void reset()
    {
        std::fill(qubit_ready_.begin(), qubit_ready_.end(), 0.0);
        std::fill(idle_.begin(), idle_.end(), 0.0);
        clbit_ready_.clear();
        condition_time_ = 0.0;
        now_ = 0.0;
    }

    const TimingModel& model() const { return model_; }

    // Advance the clock for an instruction executed on the simulator.
    template <typename Payload>
    void apply(InstructionType type, const Payload& payload)
    {
        if constexpr (std::is_same_v<Payload, Copy>) {
            for (std::size_t i = 0; i < payload.l_clbits.size(); i++)
                set_clbit_time(payload.l_clbits[i], std::max(condition_time_, clbit_time(payload.r_clbits[i])));
        } else if constexpr (std::is_same_v<Payload, Measure>) {
            std::array<std::size_t, 1> qubits{payload.qubit};
            set_clbit_time(payload.clbit, occupy_(type, qubits));
        } else if constexpr (requires { payload.qubits; }) {
            occupy_(type, payload.qubits);
        } else if constexpr (requires { payload.qubit; }) {
            std::array<std::size_t, 1> qubits{payload.qubit};
            occupy_(type, qubits);
        }
    }

    // Schedule a whole circuit given as instructions JSON, without simulating
    // it. Only valid for circuits without control flow or communications,
    // whose schedule does not depend on the measurement outcomes.
    void schedule(const JSON& instructions)
    {
        auto as_indices = [](const JSON& j) {
            return j.is_array() ? j.get<std::vector<std::size_t>>() : std::vector<std::size_t>{j.get<std::size_t>()};
        };

        reset();
        for (const auto& instruction : instructions) {
            const auto type = instruction_type_from_name(instruction.at("name").get<std::string>());
            if (type == InstructionType::MEASURE) {
                apply(type, Measure{
                    as_indices(instruction.at("qubits")).at(0),
                    as_indices(instruction.at("clbits")).at(0),
                    true
                });
            } else if (type == InstructionType::COPY) {
                apply(type, Copy{
                    instruction.at("l_clbits").get<std::vector<std::size_t>>(),
                    instruction.at("r_clbits").get<std::vector<std::size_t>>()
                });
            } else if (auto it = instruction.find("qubits"); it != instruction.end()) {
                apply(type, MultiNoParam{as_indices(*it)});
            }
        }
    }

    // Operations inside a CIF block cannot start before the condition is known.
    void begin_conditional(const std::vector<std::size_t>& clbits) { condition_time_ = classical_time(clbits); }
    void end_conditional() { condition_time_ = 0.0; }

    // Earliest time at which a classical operation on these clbits can happen.
    double classical_time(const std::vector<std::size_t>& clbits) const
    {
        double t = condition_time_;
        for (auto c : clbits)
            t = std::max(t, clbit_time(c));
        return t;
    }

    // A message sent at send_time arrives after the modeled latency.
    void receive_clbit(std::size_t clbit, double send_time)
    {
        set_clbit_time(clbit, std::max(condition_time_, send_time + model_.classical_latency()));
    }

    double clbit_time(std::size_t clbit) const
    {
        auto it = clbit_ready_.find(clbit);
        return it == clbit_ready_.end() ? 0.0 : it->second;
    }

    void set_clbit_time(std::size_t clbit, double t)
    {
        clbit_ready_[clbit] = t;
        now_ = std::max(now_, t);
    }

    bool owns_qubit(std::size_t qubit) const
    {
        return qubit >= qubit_offset_ && qubit - qubit_offset_ < qubit_ready_.size();
    }

    double qubit_time(std::size_t qubit) const { return std::max(condition_time_, qubit_ready_[local_(qubit)]); }

    // Keep `qubit` busy in [start, end), with start >= qubit_time(qubit). Used
    // for operations spanning several vQPUs, such as entanglement generation.
    void occupy(std::size_t qubit, double start, double end)
    {
        auto q = local_(qubit);
        add_idle_(q, start);
        qubit_ready_[q] = end;
        now_ = std::max(now_, end);
    }

    double now() const { return now_; }
    const std::vector<double>& idle_times() const { return idle_; }

private:
    TimingModel model_;
    std::size_t qubit_offset_;
    std::vector<double> qubit_ready_;
    std::vector<double> idle_;
    std::unordered_map<std::size_t, double> clbit_ready_;
    double condition_time_ = 0.0;
    double now_ = 0.0;

    std::size_t local_(std::size_t qubit) const
    {
        if (!owns_qubit(qubit))
            throw std::out_of_range("SimClock: qubit " + std::to_string(qubit) + " does not belong to this vQPU.");
        return qubit - qubit_offset_;
    }

    void add_idle_(std::size_t q, double start)
    {
        const double idle = start - qubit_ready_[q];
        if (idle > 0.0) {
            idle_[q] += idle;
            if (idle_hook)
                idle_hook(q + qubit_offset_, qubit_ready_[q], idle);
        }
    }

    template <typename Qubits>
    double occupy_(InstructionType type, const Qubits& qubits)
    {
        double start = condition_time_;
        for (auto q : qubits)
            start = std::max(start, qubit_ready_[local_(q)]);

        const double end = start + model_.duration(type, std::size(qubits));
        for (auto q : qubits)
            occupy(q, start, end);
        return end;
    }
};


// Simulated-time statistics accumulated over the shots of an execution.
class TimingStats {
public:
    void add_shot(const SimClock& clock)
    {
        const double t = clock.now();
        min_ = shots_ == 0 ? t : std::min(min_, t);
        max_ = shots_ == 0 ? t : std::max(max_, t);
        sum_ += t;

        const auto& idle = clock.idle_times();
        idle_sum_.resize(idle.size(), 0.0);
        for (std::size_t q = 0; q < idle.size(); q++)
            idle_sum_[q] += idle[q];
        shots_++;
    }

    JSON to_json() const
    {
        const double n = shots_ > 0 ? static_cast<double>(shots_) : 1.0;
        std::vector<double> idle_mean(idle_sum_.size());
        std::transform(idle_sum_.begin(), idle_sum_.end(), idle_mean.begin(), [n](double s) { return s / n; });
        return {
            {"unit", "s"},
            {"mean", sum_ / n},
            {"min", min_},
            {"max", max_},
            {"idle_mean", idle_mean}
        };
    }

private:
    std::size_t shots_ = 0;
    double sum_ = 0.0;
    double min_ = 0.0;
    double max_ = 0.0;
    std::vector<double> idle_sum_;
};

} // End of sim namespace
} // End of cunqa namespace
