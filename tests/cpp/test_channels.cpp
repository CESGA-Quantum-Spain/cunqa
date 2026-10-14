#include <cmath>

#include <gtest/gtest.h>

#include "sim/noise/channels.hpp"
#include "sim/noise/device_noise.hpp"

using namespace cunqa;
using namespace cunqa::noise;

TEST(Channels, RelaxationDecaysWithTime)
{
    const Coherence coherence(10e-6, 15e-6);
    const auto r = relaxation(coherence, 5e-6);
    EXPECT_NEAR(r.amplitude_decay, std::exp(-0.5), 1e-12);
    EXPECT_NEAR(r.coherence_decay, std::exp(-5.0 / 15.0), 1e-12);
    EXPECT_TRUE(relaxation(coherence, 0.0).is_identity());
    EXPECT_TRUE(relaxation(Coherence{}, 1.0).is_identity());
}

TEST(Channels, CoherenceClipsT2ToTwiceT1)
{
    const Coherence coherence(10e-6, 50e-6);
    EXPECT_DOUBLE_EQ(coherence.T2, 20e-6);
    EXPECT_THROW(Coherence(-1.0, 1.0), std::invalid_argument);
}

TEST(Channels, PauliTwirlOfRelaxation)
{
    const double a = 0.7, c = 0.6;
    const auto p = pauli_twirl({a, c});
    EXPECT_NEAR(p[1], (1.0 - a) / 4.0, 1e-12);
    EXPECT_NEAR(p[2], (1.0 - a) / 4.0, 1e-12);
    EXPECT_NEAR(p[3], (1.0 + a - 2.0 * c) / 4.0, 1e-12);
    EXPECT_NEAR(p[0] + p[1] + p[2] + p[3], 1.0, 1e-12);

    // The twirled channel keeps the diagonal of the Pauli transfer matrix: diag(1, c, c, a).
    EXPECT_NEAR(p[0] + p[1] - p[2] - p[3], c, 1e-12);
    EXPECT_NEAR(p[0] - p[1] + p[2] - p[3], c, 1e-12);
    EXPECT_NEAR(p[0] - p[1] - p[2] + p[3], a, 1e-12);
}

TEST(Channels, WernerPairIsDepolarizedHalf)
{
    const auto bell = werner(0.85);
    const auto p = bell_to_pauli(bell);
    EXPECT_DOUBLE_EQ(p[0], 0.85);
    for (int k = 1; k < 4; ++k)
        EXPECT_NEAR(p[k], 0.05, 1e-12);
    EXPECT_THROW(werner(1.2), std::invalid_argument);
}

TEST(Channels, BellCoefficientsMustBeADistribution)
{
    EXPECT_NO_THROW(normalized_bell({0.7, 0.2, 0.06, 0.04}));
    EXPECT_THROW(normalized_bell({0.7, 0.2, 0.2, 0.04}), std::invalid_argument);
    EXPECT_THROW(normalized_bell({1.1, -0.1, 0.0, 0.0}), std::invalid_argument);
}

TEST(Channels, DepolarizingErrorProbability)
{
    EXPECT_NEAR(depolarizing_error_probability(4.0 / 3.0, 1), 1.0, 1e-12);
    EXPECT_NEAR(depolarizing_error_probability(0.1, 1), 0.075, 1e-12);
    EXPECT_NEAR(depolarizing_error_probability(0.1, 2), 0.1 * 15.0 / 16.0, 1e-12);
}

namespace {

JSON noise_properties()
{
    return JSON::parse(R"json({
        "Qubits": {
            "q[0]": {"T1 (s)": 50e-6, "T2 (s)": 40e-6, "Drive Frequency (Hz)": 5e9,
                     "Readout fidelity (RB)": 0.97, "Readout duration (s)": 1e-6},
            "q[1]": {"T1 (s)": 60e-6, "T2 (s)": 70e-6, "Drive Frequency (Hz)": 5e9,
                     "Readout fidelity (RB)": 0.95}
        },
        "Q1Gates": {
            "q[0]": {"SX": {"Gate duration (s)": 5e-8, "Fidelity(RB)": 0.999},
                     "X": {"Gate duration (s)": 0.0, "Fidelity(RB)": 0.99}}
        },
        "Q2Gates(RB)": {
            "0-1": {"CX": {"Control": 0, "Target": 1, "Duration (s)": 4e-7, "Fidelity(RB)": 0.98}}
        }
    })json");
}

} // End of anonymous namespace

TEST(DeviceNoise, ReadsQubitProperties)
{
    const DeviceNoiseModel device(noise_properties(), true, true, true);
    ASSERT_NE(device.coherence(0), nullptr);
    EXPECT_DOUBLE_EQ(device.coherence(0)->T1, 50e-6);
    EXPECT_DOUBLE_EQ(device.coherence(0)->T2, 40e-6);
    EXPECT_EQ(device.coherence(2), nullptr);
    EXPECT_NEAR(device.readout_error(1), 0.05, 1e-12);
    EXPECT_EQ(device.readout_duration(0).value(), 1e-6);
    EXPECT_FALSE(device.readout_duration(1).has_value());
}

TEST(DeviceNoise, GateErrorWithoutRelaxationIsPureDepolarizing)
{
    const DeviceNoiseModel device(noise_properties(), true, true, true);
    const auto* x = device.gate("x", {0});
    ASSERT_NE(x, nullptr);
    EXPECT_FALSE(x->relaxation);
    // With no relaxation, the RB error e of a single-qubit gate is depolarizing with
    // parameter 2 e, i.e. a non-identity Pauli with probability 3/2 e.
    EXPECT_NEAR(x->depolarizing, 2.0 * 0.01, 1e-12);
    EXPECT_EQ(device.gate("x", {1}), nullptr);
}

TEST(DeviceNoise, GateErrorSubtractsRelaxationInfidelity)
{
    const DeviceNoiseModel device(noise_properties(), true, true, true);
    const auto* cx = device.gate("cx", {0, 1});
    ASSERT_NE(cx, nullptr);
    EXPECT_TRUE(cx->relaxation);
    EXPECT_DOUBLE_EQ(cx->duration, 4e-7);

    // Same expression as qiskit-aer's _device_depolarizing_error.
    const auto r0 = relaxation(*device.coherence(0), 4e-7);
    const auto r1 = relaxation(*device.coherence(1), 4e-7);
    const double process = (1 + r0.amplitude_decay + 2 * r0.coherence_decay) / 4
                         * (1 + r1.amplitude_decay + 2 * r1.coherence_decay) / 4;
    const double relax_fidelity = (4 * process + 1) / 5;
    const double expected = 4 * (0.02 - (1 - relax_fidelity)) / (4 * relax_fidelity - 1);
    EXPECT_NEAR(cx->depolarizing, expected, 1e-12);

    EXPECT_EQ(device.gate("cx", {1, 0}), nullptr);
}

TEST(DeviceNoise, FlagsDisableEachPart)
{
    const DeviceNoiseModel device(noise_properties(), false, false, false);
    EXPECT_EQ(device.coherence(0), nullptr);
    EXPECT_DOUBLE_EQ(device.readout_error(0), 0.0);
    const auto* cx = device.gate("cx", {0, 1});
    ASSERT_NE(cx, nullptr);
    EXPECT_FALSE(cx->relaxation);
    EXPECT_DOUBLE_EQ(cx->depolarizing, 0.0);
    EXPECT_DOUBLE_EQ(cx->duration, 4e-7);
}
