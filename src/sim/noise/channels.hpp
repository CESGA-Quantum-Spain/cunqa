#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>

namespace cunqa {
namespace noise {

// Probabilities of the single-qubit Paulis, in the order {I, X, Y, Z}.
using PauliProbabilities = std::array<double, 4>;

// Coefficients of a Bell-diagonal state, in the order {Phi+, Psi+, Psi-, Phi-}.
using BellCoefficients = std::array<double, 4>;

inline constexpr double NO_DECOHERENCE = std::numeric_limits<double>::infinity();

// Coherence times of a qubit, in seconds. An infinite time disables that decay.
struct Coherence {
    double T1 = NO_DECOHERENCE;
    double T2 = NO_DECOHERENCE;

    Coherence() = default;
    Coherence(double t1, double t2) : T1{t1}, T2{t2}
    {
        if (!(T1 > 0.0) || !(T2 > 0.0))
            throw std::invalid_argument("T1 and T2 must be positive.");
        // Physical constraint T2 <= 2 T1, enforced as qiskit-aer does.
        T2 = std::min(T2, 2.0 * T1);
    }

    bool decoheres() const { return std::isfinite(T1) || std::isfinite(T2); }
    double rate1() const { return std::isfinite(T1) ? 1.0 / T1 : 0.0; }
    double rate2() const { return std::isfinite(T2) ? 1.0 / T2 : 0.0; }
};

// Zero-temperature thermal relaxation, described by how much of the excited population
// (amplitude_decay = exp(-t/T1)) and of the coherences (coherence_decay = exp(-t/T2))
// survive. Averaged channels are described the same way, with the expectations of those
// exponentials over the random idle time.
struct Relaxation {
    double amplitude_decay = 1.0;
    double coherence_decay = 1.0;

    bool is_identity() const { return amplitude_decay >= 1.0 && coherence_decay >= 1.0; }
};

inline Relaxation relaxation(const Coherence& coherence, double time)
{
    return {std::exp(-time * coherence.rate1()), std::exp(-time * coherence.rate2())};
}

// Pauli twirl of a relaxation channel: the Pauli channel with the same diagonal Pauli
// transfer matrix diag(1, c, c, a), i.e. px = py = (1 - a)/4 and pz = (1 + a - 2c)/4.
inline PauliProbabilities pauli_twirl(const Relaxation& r)
{
    const double a = r.amplitude_decay;
    const double c = r.coherence_decay;
    PauliProbabilities p{(1.0 + a + 2.0 * c) / 4.0, (1.0 - a) / 4.0, (1.0 - a) / 4.0, (1.0 + a - 2.0 * c) / 4.0};
    for (auto& prob : p)
        prob = std::max(0.0, prob);
    return p;
}

// A Bell-diagonal pair is an ideal |Phi+> pair with a Pauli channel on one of its halves:
// X turns it into Psi+, Y into Psi- and Z into Phi-.
inline PauliProbabilities bell_to_pauli(const BellCoefficients& bell)
{
    return {bell[0], bell[1], bell[2], bell[3]};
}

// Werner pair of fidelity F with respect to Phi+.
inline BellCoefficients werner(double fidelity)
{
    if (!(fidelity >= 0.0) || fidelity > 1.0)
        throw std::invalid_argument("The pair fidelity must be in [0, 1].");
    const double other = (1.0 - fidelity) / 3.0;
    return {fidelity, other, other, other};
}

inline BellCoefficients normalized_bell(BellCoefficients bell)
{
    double total = 0.0;
    for (double coefficient : bell) {
        if (!(coefficient >= 0.0))
            throw std::invalid_argument("Bell coefficients must be non-negative.");
        total += coefficient;
    }
    if (std::abs(total - 1.0) > 1e-6)
        throw std::invalid_argument("Bell coefficients must add up to 1, they add up to " +
                                    std::to_string(total) + ".");
    for (auto& coefficient : bell)
        coefficient /= total;
    return bell;
}

// Total probability of a non-identity Pauli in an n-qubit depolarizing channel with the
// qiskit-aer parametrization (param = 4^n / (4^n - 1) is the completely depolarizing one).
inline double depolarizing_error_probability(double param, std::size_t num_qubits)
{
    const double dim2 = std::pow(4.0, static_cast<double>(num_qubits));
    return std::clamp(param * (dim2 - 1.0) / dim2, 0.0, 1.0);
}

} // End of noise namespace
} // End of cunqa namespace
