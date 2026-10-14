#pragma once

#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

#include "sim/simulator.hpp"
#include "dynamic_circuit/instruction.hpp"

#include "channels.hpp"

namespace cunqa {
namespace noise {

enum class ChannelMode {
    // Relaxation as its exact channel: a reset mixture when T2 <= T1, amplitude damping
    // (Kraus) plus dephasing otherwise.
    EXACT,
    // Every channel replaced by its Pauli twirl, so that stabilizer simulators can run it.
    PAULI
};

// Applies noise channels to a simulator, one trajectory per shot. State-independent
// mixtures (Pauli channels, the reset mixture) are sampled here, so they only need X, Y,
// Z and reset; the only channel that needs the simulator's help is amplitude damping.
class ChannelApplier {
public:
    ChannelApplier(sim::Simulator& simulator, std::mt19937_64& rng, ChannelMode mode)
        : simulator_{simulator}
        , rng_{rng}
        , mode_{mode}
    {
        if (mode == ChannelMode::EXACT && !simulator.supports_kraus())
            throw std::invalid_argument(
                "The exact noise mode needs Kraus channels, which " + simulator.get_name() +
                " does not support with this configuration. Use the \"pauli\" noise mode.");
    }

    ChannelMode mode() const { return mode_; }

    void pauli(std::size_t qubit, const PauliProbabilities& probabilities)
    {
        const double u = uniform_(rng_);
        double acc = probabilities[0];
        for (int pauli = 1; pauli < 4; ++pauli) {
            if (u < acc)
                return;
            acc += probabilities[pauli];
            if (u < acc) {
                apply_pauli_(qubit, pauli);
                return;
            }
        }
    }

    // n-qubit depolarizing channel with the qiskit-aer parametrization.
    void depolarizing(const std::vector<std::size_t>& qubits, double param)
    {
        if (param <= 0.0 || qubits.empty())
            return;
        if (uniform_(rng_) >= depolarizing_error_probability(param, qubits.size()))
            return;

        // Uniformly random non-identity Pauli string, one base-4 digit per qubit.
        const std::uint64_t num_strings = std::uint64_t{1} << (2 * qubits.size());
        std::uint64_t label = std::uniform_int_distribution<std::uint64_t>(1, num_strings - 1)(rng_);
        for (const auto qubit : qubits) {
            apply_pauli_(qubit, static_cast<int>(label & 3));
            label >>= 2;
        }
    }

    void relaxation(std::size_t qubit, const Relaxation& r)
    {
        if (r.is_identity())
            return;

        if (mode_ == ChannelMode::PAULI) {
            pauli(qubit, pauli_twirl(r));
            return;
        }

        const double a = r.amplitude_decay;
        const double c = r.coherence_decay;
        if (c <= a) {
            // T2 <= T1: mixture of identity, Z and reset to |0>.
            const double p_reset = 1.0 - a;
            const double p_z = 0.5 * (a - c);
            const double u = uniform_(rng_);
            if (u < p_reset)
                reset(qubit);
            else if (u < p_reset + p_z)
                apply_pauli_(qubit, 3);
        } else {
            // T1 < T2 <= 2 T1: amplitude damping, which keeps sqrt(a) of the coherences,
            // followed by the dephasing that brings them down to c.
            if (a < 1.0) {
                const double gamma = 1.0 - a;
                simulator_.apply_kraus({qubit}, {
                    {1.0, 0.0, 0.0, std::sqrt(a)},
                    {0.0, std::sqrt(gamma), 0.0, 0.0}
                });
            }
            const double p_z = 0.5 * (1.0 - c / std::sqrt(a));
            if (p_z > 0.0 && uniform_(rng_) < p_z)
                apply_pauli_(qubit, 3);
        }
    }

    void reset(std::size_t qubit)
    {
        simulator_.apply_gate(InstructionType::RESET, Reset{{qubit}});
    }

    bool flip(double probability)
    {
        return probability > 0.0 && uniform_(rng_) < probability;
    }

private:
    sim::Simulator& simulator_;
    std::mt19937_64& rng_;
    ChannelMode mode_;
    std::uniform_real_distribution<double> uniform_{0.0, 1.0};

    void apply_pauli_(std::size_t qubit, int pauli)
    {
        switch (pauli) {
            case 1: simulator_.apply_gate(InstructionType::X, OneQubitNoParam{qubit}); break;
            case 2: simulator_.apply_gate(InstructionType::Y, OneQubitNoParam{qubit}); break;
            case 3: simulator_.apply_gate(InstructionType::Z, OneQubitNoParam{qubit}); break;
            default: break;
        }
    }
};

} // End of noise namespace
} // End of cunqa namespace
