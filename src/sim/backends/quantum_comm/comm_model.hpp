#pragma once

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "sim/noise/channels.hpp"
#include "sim/noise/device_noise.hpp"
#include "sim/random_delay.hpp"
#include "sim/timing.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

// How the random delays of a shot (EPR generation times, classical jitter) are treated.
enum class WaitStrategy {
    // Every shot samples its own delays, so each shot is one realization of the timeline.
    // Sampled delays can be binned (time_bin) to emulate grouping shots by realization.
    EXACT,
    // Delays are not sampled: the timeline uses their mean and every random wait is turned
    // into the averaged relaxation channel E[R_T], built from the Laplace transform of the
    // waiting time. Exact for one qubit, it misses the correlations of waits shared by
    // several qubits.
    AVERAGED
};

enum class NoiseMode {
    AUTO,   // EXACT when the simulator supports Kraus channels, PAULI otherwise
    EXACT,
    PAULI
};

// One pair delivered by an entanglement generator: its generation time and the Bell
// coefficients {Phi+, Psi+, Psi-, Phi-} of the state at heralding.
struct EprDraw {
    double delay;
    noise::BellCoefficients bell;
};

// Source of entangled pairs of a link. The parametric source models a heralded generator
// (N ~ Geom(p) attempts of duration attempt_time, fixed Bell-diagonal state); the tabulated
// one replays (delay, Bell coefficients) samples precomputed offline, e.g. with a network
// simulator such as Q2NS or NetSquid.
class EprSource {
public:
    static std::shared_ptr<const EprSource> from_json(const JSON& epr);

    EprDraw sample(Rng& rng) const;

    const RandomDelayPtr& delay() const { return delay_; }
    const noise::BellCoefficients& mean_bell() const { return mean_bell_; }

private:
    RandomDelayPtr delay_;
    noise::BellCoefficients mean_bell_{1.0, 0.0, 0.0, 0.0};

    // Tabulated sources keep their samples, (delay, weight) and Bell coefficients, in the
    // given order, so that a draw returns the delay and the state of the same sample.
    std::shared_ptr<const DiscreteDelay> table_;
    std::vector<std::pair<double, double>> rows_;
    std::vector<double> cumulative_;
    std::vector<noise::BellCoefficients> table_bell_;
};

// Timing and noise of one vQPU of the quantum-communications scheme.
struct VQPUModel {
    std::string id;
    std::size_t first_qubit = 0;
    std::size_t num_data_qubits = 0;
    std::size_t num_comm_qubits = 0;

    std::optional<TimingModel> timing;  // the "timing" block of the backend
    std::optional<noise::Coherence> data_qubits;
    std::optional<noise::Coherence> comm_qubits;
    std::shared_ptr<const noise::DeviceNoiseModel> device;

    bool is_comm_qubit(std::size_t local_qubit) const { return local_qubit >= num_data_qubits; }

    // Coherence of a local qubit: the communication-qubit times when given, then the noise
    // properties, then the default data-qubit times.
    const noise::Coherence* coherence(std::size_t local_qubit) const;
};

// Communication noise model of the vQPUs that share the quantum-communications executor:
// the entanglement generator of every link and the decoherence of the idle qubits, on top
// of the simulated time of their "timing" blocks. It is assembled from the "comm_model"
// (and "noise_model") sections of the backend of every vQPU, optionally overridden for a
// single run by a "comm_model" run parameter.
struct CommModel {
    // Whether the noise model is used: some backend has a comm_model or a noise_model, or
    // the run has a comm_model.
    bool enabled = false;
    WaitStrategy strategy = WaitStrategy::EXACT;
    double time_bin = 0.0;
    NoiseMode noise_mode = NoiseMode::AUTO;

    std::vector<VQPUModel> vqpus;

    // From the backend of every vQPU, as published in qpus.json. The noise properties are
    // read from disk here.
    static CommModel from_backends(const std::vector<std::pair<std::string, JSON>>& backends);

    // Applies the "comm_model" run parameter (a null JSON keeps the deployment model).
    CommModel with_run_overrides(const JSON& run_comm_model) const;

    const EprSource& epr(const std::string& qpu_a, const std::string& qpu_b) const;

    bool all_timed() const;
    bool any_timed() const;

private:
    // Network-level parameters as given in the backends, to be resolved with the run ones.
    JSON network_json_ = JSON::object();
    std::map<std::string, std::shared_ptr<const EprSource>> links_;
    std::shared_ptr<const EprSource> default_epr_;

    void build_network_(const JSON& network);
};

std::string link_key(const std::string& qpu_a, const std::string& qpu_b);

} // End of sim namespace
} // End of cunqa namespace
