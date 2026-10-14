#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "utils/json.hpp"

#include "channels.hpp"

namespace cunqa {
namespace noise {

// Noise of a gate on a given tuple of qubits: a depolarizing error followed by thermal
// relaxation over the gate duration. The depolarizing parameter only accounts for the part
// of the RB infidelity that relaxation does not explain, as qiskit-aer's
// basic_device_gate_errors does (and so does AER_noise_helpers.hpp for native execution).
struct GateNoise {
    double duration = 0.0;
    double depolarizing = 0.0;
    bool relaxation = false;
};

// Per-vQPU noise built from a noise_properties JSON (see noise_properties_example.rst), for
// simulators that apply instructions one by one. Qubit indices are local to the vQPU.
class DeviceNoiseModel {
public:
    DeviceNoiseModel() = default;

    DeviceNoiseModel(
        const JSON& noise_properties,
        bool thermal_relaxation,
        bool readout_error,
        bool gate_error
    )
    {
        if (noise_properties.contains("Qubits")) {
            for (const auto& [key, qubit] : noise_properties.at("Qubits").items()) {
                const auto q = qubit_index_(key);
                if (thermal_relaxation)
                    coherence_[q] = Coherence(qubit.at("T1 (s)").get<double>(), qubit.at("T2 (s)").get<double>());
                if (readout_error)
                    readout_error_[q] = 1.0 - qubit.at("Readout fidelity (RB)").get<double>();
                if (qubit.contains("Readout duration (s)"))
                    readout_duration_[q] = qubit.at("Readout duration (s)").get<double>();
            }
        }

        if (noise_properties.contains("Q1Gates")) {
            for (const auto& [key, gates] : noise_properties.at("Q1Gates").items()) {
                const auto q = qubit_index_(key);
                for (const auto& [name, gate] : gates.items()) {
                    gates_[{lowercase_(name), {q}}] = gate_noise_(
                        {q},
                        gate.at("Gate duration (s)").get<double>(),
                        1.0 - gate.at("Fidelity(RB)").get<double>(),
                        thermal_relaxation,
                        gate_error
                    );
                }
            }
        }

        if (noise_properties.contains("Q2Gates(RB)")) {
            for (const auto& [pair, gates] : noise_properties.at("Q2Gates(RB)").items()) {
                for (const auto& [name, gate] : gates.items()) {
                    const std::vector<std::size_t> qubits{
                        gate.at("Control").get<std::size_t>(),
                        gate.at("Target").get<std::size_t>()
                    };
                    gates_[{lowercase_(name), qubits}] = gate_noise_(
                        qubits,
                        gate.at("Duration (s)").get<double>(),
                        1.0 - gate.at("Fidelity(RB)").get<double>(),
                        thermal_relaxation,
                        gate_error
                    );
                }
            }
        }
    }

    const Coherence* coherence(std::size_t qubit) const
    {
        const auto it = coherence_.find(qubit);
        return it != coherence_.end() ? &it->second : nullptr;
    }

    double readout_error(std::size_t qubit) const
    {
        const auto it = readout_error_.find(qubit);
        return it != readout_error_.end() ? it->second : 0.0;
    }

    std::optional<double> readout_duration(std::size_t qubit) const
    {
        const auto it = readout_duration_.find(qubit);
        return it != readout_duration_.end() ? std::optional<double>{it->second} : std::nullopt;
    }

    // Noise of the gate `name` (lowercase CUNQA name) on exactly these qubits, in order.
    const GateNoise* gate(const std::string& name, const std::vector<std::size_t>& qubits) const
    {
        const auto it = gates_.find({name, qubits});
        return it != gates_.end() ? &it->second : nullptr;
    }

private:
    std::map<std::size_t, Coherence> coherence_;
    std::map<std::size_t, double> readout_error_;
    std::map<std::size_t, double> readout_duration_;
    std::map<std::pair<std::string, std::vector<std::size_t>>, GateNoise> gates_;

    static std::size_t qubit_index_(const std::string& key)
    {
        static const std::regex pattern(R"(q\[(\d+)\])");
        std::smatch match;
        if (std::regex_search(key, match, pattern))
            return std::stoul(match[1].str());
        throw std::invalid_argument("Invalid qubit key in the noise properties: " + key);
    }

    static std::string lowercase_(std::string name)
    {
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return name;
    }

    GateNoise gate_noise_(
        const std::vector<std::size_t>& qubits,
        double duration,
        double error,
        bool thermal_relaxation,
        bool gate_error
    ) const
    {
        GateNoise noise;
        noise.duration = duration;

        const double dim = std::pow(2.0, static_cast<double>(qubits.size()));
        double relax_fidelity = 1.0;

        const bool known_coherence = std::all_of(qubits.begin(), qubits.end(),
            [this](std::size_t q) { return coherence_.contains(q); });
        if (thermal_relaxation && duration > 0.0 && known_coherence) {
            // Process fidelity of the tensor product of the relaxation channels.
            double process_fidelity = 1.0;
            for (const auto q : qubits) {
                const auto r = relaxation(coherence_.at(q), duration);
                process_fidelity *= (1.0 + r.amplitude_decay + 2.0 * r.coherence_decay) / 4.0;
            }
            relax_fidelity = (dim * process_fidelity + 1.0) / (dim + 1.0);
            noise.relaxation = true;
        }

        const double relax_infidelity = 1.0 - relax_fidelity;
        if (gate_error && error > relax_infidelity) {
            const double e = std::min(error, dim / (dim + 1.0));
            const double max_param = dim * dim / (dim * dim - 1.0);
            const double param = dim * (e - relax_infidelity) / (dim * relax_fidelity - 1.0);
            noise.depolarizing = std::clamp(param, 0.0, max_param);
        }
        return noise;
    }
};

} // End of noise namespace
} // End of cunqa namespace
