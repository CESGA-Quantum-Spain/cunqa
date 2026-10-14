// Physics of the quantum-communications engine: simulated time of the vQPUs, entanglement
// generation and consumption, idle decoherence and the per-shot treatment of random waits.
//
// Every expected value is computed analytically; the tolerances are a few standard
// deviations of the binomial estimate, with fixed seeds. The noise tests make every
// operation instantaneous (ZERO_DURATIONS), so that qubits only wait for entangled pairs
// and classical messages.

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "test_support.hpp"
#include "sim/random_delay.hpp"

using namespace test;

namespace {

// E[exp(-s N tau)] for N ~ Geom(p) attempts.
double geometric_laplace(double p, double tau, double s)
{
    return cunqa::sim::GeometricAttemptsDelay(p, tau).laplace(s);
}

struct InputState {
    const char* name;
    std::vector<std::string> prepare;   // gates applied to |0>
    std::vector<std::string> undo;      // gates mapping the state back to |0>
};

const std::vector<InputState> PAULI_EIGENSTATES = {
    {"0", {}, {}},
    {"1", {"x"}, {"x"}},
    {"+", {"h"}, {"h"}},
    {"-", {"x", "h"}, {"h", "x"}},
    {"+i", {"h", "s"}, {"sdg", "h"}},
    {"-i", {"h", "sdg"}, {"s", "h"}},
};

// Teleports `state` from A's data qubit to B's, undoes the preparation at B and returns the
// probability of reading 0, i.e. the fidelity of the teleported state.
double teledata_fidelity(const InputState& state, const RunOptions& options)
{
    Circuit a("A", 1, 1, 2), b("B", 1, 1, 3);
    for (const auto& gate : state.prepare)
        a.gate(gate, 0);
    qsend(a, 0, a.comm(), 0, 1, "B", "teledata");
    qrecv(b, 0, b.comm(), 0, 1, "A", "teledata");
    for (const auto& gate : state.undo)
        b.gate(gate, 0);
    b.measure(0, 2);
    return probability(run({a, b}, options)[1], "0");
}

// Remote CNOT from A's |+> control to B's |0> target through a telegate, followed by the
// measurement of the parity of both qubits in the Pauli basis `basis` ("z", "x" or "y").
// The parity is computed at B, which receives A's outcome, and is returned as P(odd).
double telegate_odd_parity(const std::string& basis, const RunOptions& options)
{
    Circuit a("A", 1, 1, 3), b("B", 1, 1, 4);
    a.gate("h", 0);
    cat_entangler({&a, &b}, 0, {a.comm(), b.comm()}, {0, 0}, "telegate");
    b.gate("cx", b.comm(), 0);
    cat_disentangler({&a, &b}, 0, {b.comm()}, {1}, {1});

    auto rotate = [&basis](Circuit& c) {
        if (basis == "x")
            c.gate("h", 0);
        else if (basis == "y")
            c.gate("sdg", 0).gate("h", 0);
    };
    rotate(a);
    a.measure(0, 2, false).send({2}, "B");
    rotate(b);
    b.recv({2}, "A").cif({2}).gate("x", 0).endcif();
    b.measure(0, 3);
    return probability(run({a, b}, options)[1], "1");
}

} // End of anonymous namespace


// ---------------------------------------------------------------------------------------
// Without a communication model
// ---------------------------------------------------------------------------------------

TEST(QCEngineIdeal, TeledataWithoutModelIsIdealAndUntimed)
{
    for (const auto& state : PAULI_EIGENSTATES) {
        RunOptions options;
        options.shots = 300;
        EXPECT_DOUBLE_EQ(teledata_fidelity(state, options), 1.0) << "state " << state.name;
    }

    Circuit a("A", 1, 1, 2), b("B", 1, 1, 3);
    qsend(a, 0, a.comm(), 0, 1, "B", "t");
    qrecv(b, 0, b.comm(), 0, 1, "A", "t");
    b.measure(0, 2);
    const auto results = run({a, b});
    EXPECT_FALSE(results[1].contains("simulated_time"));
    EXPECT_FALSE(results[1].contains("comm_stats"));
}

TEST(QCEngineIdeal, SeededRunsAreReproducibleWithIndependentShots)
{
    Circuit a("A", 1, 1, 1), b("B", 1, 1, 1);
    a.gate("h", 0).measure(0, 0);
    b.gate("h", 0).measure(0, 0);

    RunOptions options;
    options.shots = 2000;
    options.seed = 5;
    const auto first = run({a, b}, options);
    const auto second = run({a, b}, options);
    EXPECT_EQ(first[0].at("counts"), second[0].at("counts"));
    // Every shot has its own seed, so a seeded run still samples both outcomes.
    EXPECT_NEAR(probability(first[0], "0"), 0.5, tolerance(0.5, options.shots));

    options.comm_model = JSON::object();
    const auto timed = run({a, b}, options);
    EXPECT_NEAR(probability(timed[1], "0"), 0.5, tolerance(0.5, options.shots));
    EXPECT_EQ(timed[1].at("counts"), run({a, b}, options)[1].at("counts"));
}


// ---------------------------------------------------------------------------------------
// Simulated time of the timing blocks, without the noise model
// ---------------------------------------------------------------------------------------

TEST(QCEngineTiming, EntanglementSynchronizesTheClocks)
{
    // examples/qu_comm/08-simulated_time: Bob's comm qubit is reset when Alice asks for the
    // pair, so the generation starts when both have asked for it (1 us) and lasts 2 us.
    const JSON timing = {
        {"one_qubit_gate", 5e-8},
        {"multi_qubit_gate", 3e-7},
        {"measure", 1e-6},
        {"reset", 1e-6},
        {"classical_latency", 5e-7},
        {"entanglement", 2e-6}
    };
    Circuit alice("Alice", 1, 1, 2), bob("Bob", 1, 1, 3);
    alice.gate("h", 0).gen_ent(alice.comm(), {"Bob", "Alice"}, "epr").measure(alice.comm(), 0);
    alice.send({0}, "Bob").measure(0, 1);
    bob.reset(bob.comm()).gen_ent(bob.comm(), {"Alice", "Bob"}, "epr").measure(bob.comm(), 0);
    bob.recv({1}, "Alice").cif({1}).gate("x", 0).endcif().measure(0, 2);

    RunOptions options;
    options.shots = 400;
    options.timing = timing;
    const auto results = run({alice, bob}, options);

    // Alice: pair [1, 3) us, comm measurement [3, 4) us; her comm qubit idles 1 us.
    const auto& a = results[0].at("simulated_time");
    EXPECT_EQ(a.at("unit"), "s");
    EXPECT_NEAR(a.at("min").get<double>(), 4e-6, 1e-15);
    EXPECT_NEAR(a.at("max").get<double>(), 4e-6, 1e-15);
    EXPECT_NEAR(a.at("idle_mean")[0].get<double>(), 0.0, 1e-15);
    EXPECT_NEAR(a.at("idle_mean")[1].get<double>(), 1e-6, 1e-15);

    // Bob: the bit arrives at 4.5 us; X [4.5, 4.55) us only when it is 1, but the data qubit
    // waits for it in every shot, so the measurement ends at 5.55 or 5.5 us.
    const auto& b = results[1].at("simulated_time");
    EXPECT_NEAR(b.at("min").get<double>(), 5.5e-6, 1e-15);
    EXPECT_NEAR(b.at("max").get<double>(), 5.55e-6, 1e-15);
    EXPECT_NEAR(b.at("idle_mean")[0].get<double>(), 4.5e-6, 1e-15);
    EXPECT_NEAR(b.at("idle_mean")[1].get<double>(), 0.0, 1e-15);

    // Bob's comm qubit holds Alice's pair and his data qubit copies her bit.
    EXPECT_NEAR(probability(results[1], "00") + probability(results[1], "11"), 1.0, 1e-12);
    EXPECT_FALSE(results[1].contains("comm_stats"));
}

TEST(QCEngineTiming, SkippedCorrectionsStillWaitForTheMessage)
{
    // Teledata with 1 us measurements, other operations instantaneous and 2 us of latency.
    // A measures in [0, 1) us and the corrections reach B at 3 us. Whatever they are, B
    // cannot swap the qubit into its data qubit before, so its measurement ends at 4 us
    // (at 1 us if skipped corrections did not wait).
    JSON timing = ZERO_DURATIONS;
    timing["measure"] = 1e-6;
    timing["classical_latency"] = 2e-6;

    Circuit a("A", 1, 1, 2), b("B", 1, 1, 3);
    a.gate("h", 0);
    qsend(a, 0, a.comm(), 0, 1, "B", "t");
    qrecv(b, 0, b.comm(), 0, 1, "A", "t");
    b.measure(0, 2);

    RunOptions options;
    options.shots = 200;
    options.timing = timing;
    const auto results = run({a, b}, options);
    EXPECT_NEAR(results[0].at("simulated_time").at("max").get<double>(), 1e-6, 1e-15);
    EXPECT_NEAR(results[1].at("simulated_time").at("min").get<double>(), 4e-6, 1e-15);
    EXPECT_NEAR(results[1].at("simulated_time").at("max").get<double>(), 4e-6, 1e-15);
}

TEST(QCEngineTiming, PartialTimingIsIgnored)
{
    Circuit a("A", 1, 1, 2), b("B", 1, 1, 3);
    qsend(a, 0, a.comm(), 0, 1, "B", "t");
    qrecv(b, 0, b.comm(), 0, 1, "A", "t");
    b.measure(0, 2);

    RunOptions options;
    options.shots = 50;
    options.backend_extra = {{{"timing", ZERO_DURATIONS}}, JSON::object()};
    const auto results = run({a, b}, options);
    EXPECT_FALSE(results[0].contains("simulated_time"));
    EXPECT_FALSE(results[1].contains("simulated_time"));
}

TEST(QCEngineTiming, CommModelUsesTheDefaultDurationsWithoutTimingBlocks)
{
    // Default durations: CX 0.3 us, 1-qubit gates 0.05 us, measurements 1 us. A measures its
    // comm qubit in [0.3, 1.3) and its data qubit in [0.35, 1.35) us; B applies the Z
    // correction in [1.35, 1.4) us or skips it, then SWAP and measurement take 1.3 us more.
    Circuit a("A", 1, 1, 2), b("B", 1, 1, 3);
    qsend(a, 0, a.comm(), 0, 1, "B", "t");
    qrecv(b, 0, b.comm(), 0, 1, "A", "t");
    b.measure(0, 2);

    RunOptions options;
    options.shots = 400;
    options.comm_model = JSON::object();
    const auto results = run({a, b}, options);
    EXPECT_NEAR(results[1].at("simulated_time").at("min").get<double>(), 2.65e-6, 1e-15);
    EXPECT_NEAR(results[1].at("simulated_time").at("max").get<double>(), 2.7e-6, 1e-15);
}

TEST(QCEngineTiming, GenerationTakesTheEntanglementDurationPlusTheAttempts)
{
    JSON timing = ZERO_DURATIONS;
    timing["entanglement"] = 2e-6;

    Circuit a("A", 1, 1, 2), b("B", 1, 1, 3);
    qsend(a, 0, a.comm(), 0, 1, "B", "t");
    qrecv(b, 0, b.comm(), 0, 1, "A", "t");
    b.measure(0, 2);

    RunOptions options;
    options.shots = 20;
    options.timing = timing;
    options.comm_model = {{"epr", {{"attempt_time", 1e-6}}}};
    const auto results = run({a, b}, options);
    EXPECT_NEAR(results[1].at("comm_stats").at("mean_generation_time").get<double>(), 3e-6, 1e-15);
    EXPECT_NEAR(results[1].at("simulated_time").at("max").get<double>(), 3e-6, 1e-15);
}


// ---------------------------------------------------------------------------------------
// Pair state delivered by the generator
// ---------------------------------------------------------------------------------------

TEST(QCEngineEPR, TeledataWithWernerPairHasAverageFidelityTwoFPlusOneOverThree)
{
    const double F = 0.8;
    RunOptions options;
    options.shots = 4000;
    options.comm_model = {{"epr", {{"fidelity", F}}}};

    double average = 0.0;
    for (const auto& state : PAULI_EIGENSTATES) {
        const double fidelity = teledata_fidelity(state, options);
        // A Werner pair depolarizes every input state alike.
        EXPECT_NEAR(fidelity, (2.0 * F + 1.0) / 3.0, tolerance(0.87, options.shots)) << "state " << state.name;
        average += fidelity / PAULI_EIGENSTATES.size();
    }
    // The six Pauli eigenstates form a 2-design: their mean is the average fidelity.
    EXPECT_NEAR(average, (2.0 * F + 1.0) / 3.0, tolerance(0.87, 6 * options.shots));
}

TEST(QCEngineEPR, TeledataWithBellDiagonalPairIsThePairPauliChannel)
{
    // {Phi+, Psi+, Psi-, Phi-} = {I, X, Y, Z} errors on the teleported qubit.
    const std::array<double, 4> bell{0.7, 0.2, 0.06, 0.04};
    RunOptions options;
    options.shots = 4000;
    options.comm_model = {{"epr", {{"bell_coefficients", bell}}}};

    const double z_basis = 1.0 - bell[1] - bell[2]; // flipped by X and Y
    const double x_basis = 1.0 - bell[2] - bell[3]; // flipped by Y and Z
    const double y_basis = 1.0 - bell[1] - bell[3]; // flipped by X and Z
    const std::vector<double> expected{z_basis, z_basis, x_basis, x_basis, y_basis, y_basis};

    for (std::size_t i = 0; i < PAULI_EIGENSTATES.size(); ++i)
        EXPECT_NEAR(teledata_fidelity(PAULI_EIGENSTATES[i], options), expected[i], tolerance(expected[i], options.shots))
            << "state " << PAULI_EIGENSTATES[i].name;
}

TEST(QCEngineEPR, TelegateWithBellDiagonalPairGivesCorrelatedPauliError)
{
    // A telegate CNOT with a Bell-diagonal pair suffers II, IX, ZI or ZX on (control, target)
    // with the coefficients of Phi+, Psi+, Phi- and Psi-. The ideal output is Phi+, so the
    // parities read: ZZ odd for IX and ZX, XX odd for ZI and ZX, YY odd for II and ZX.
    const std::array<double, 4> bell{0.6, 0.25, 0.1, 0.05};   // Phi+, Psi+, Psi-, Phi-
    RunOptions options;
    options.shots = 6000;
    options.comm_model = {{"epr", {{"bell_coefficients", bell}}}};

    const double zz = bell[1] + bell[2];
    const double xx = bell[3] + bell[2];
    const double yy = bell[0] + bell[2];
    EXPECT_NEAR(telegate_odd_parity("z", options), zz, tolerance(zz, options.shots));
    EXPECT_NEAR(telegate_odd_parity("x", options), xx, tolerance(xx, options.shots));
    EXPECT_NEAR(telegate_odd_parity("y", options), yy, tolerance(yy, options.shots));
}

TEST(QCEngineEPR, MultipartyEntanglementUsesTheNoiseOfEachLink)
{
    // Fan-out of A's |+> control to B and C: the GHZ state comes from the links A-B and A-C,
    // and the Psi errors of each link flip the target of that link only.
    const double f_ab = 0.85, f_ac = 0.7;
    RunOptions options;
    options.shots = 6000;
    options.comm_model = {
        {"epr", {{"fidelity", f_ab}}},
        {"links", {{{"qpus", {"A", "C"}}, {"epr", {{"fidelity", f_ac}}}}}}
    };

    Circuit a("A", 1, 1, 4), b("B", 1, 1, 4), c("C", 1, 1, 4);
    a.gate("h", 0);
    cat_entangler({&a, &b, &c}, 0, {a.comm(), b.comm(), c.comm()}, {0, 0, 0}, "fanout");
    b.gate("cx", b.comm(), 0);
    c.gate("cx", c.comm(), 0);
    cat_disentangler({&a, &b, &c}, 0, {b.comm(), c.comm()}, {1, 2}, {1, 1});

    a.measure(0, 3, false).send({3}, "B").send({3}, "C");
    for (auto* target : {&b, &c})
        target->recv({2}, "A").cif({2}).gate("x", 0).endcif().measure(0, 3);

    const auto results = run({a, b, c}, options);
    const double odd_b = 2.0 * (1.0 - f_ab) / 3.0;
    const double odd_c = 2.0 * (1.0 - f_ac) / 3.0;
    EXPECT_NEAR(probability(results[1], "1"), odd_b, tolerance(odd_b, options.shots));
    EXPECT_NEAR(probability(results[2], "1"), odd_c, tolerance(odd_c, options.shots));
    EXPECT_NEAR(results[0].at("comm_stats").at("entanglement_requests_per_shot").get<double>(), 1.0, 1e-12);
}

TEST(QCEngineEPR, TabulatedSourceReplaysItsSamples)
{
    RunOptions options;
    options.shots = 4000;
    options.comm_model = {{"epr", {
        {"source", "tabulated"},
        {"samples", {
            {{"delay", 1e-6}, {"fidelity", 1.0}, {"weight", 1.0}},
            {{"delay", 3e-6}, {"fidelity", 0.4}, {"weight", 1.0}}
        }}
    }}};

    const double expected = 0.5 * 1.0 + 0.5 * (2.0 * 0.4 + 1.0) / 3.0;
    Circuit a("A", 1, 1, 2), b("B", 1, 1, 3);
    a.gate("h", 0);
    qsend(a, 0, a.comm(), 0, 1, "B", "t");
    qrecv(b, 0, b.comm(), 0, 1, "A", "t");
    b.gate("h", 0).measure(0, 2);
    const auto results = run({a, b}, options);
    EXPECT_NEAR(probability(results[1], "0"), expected, tolerance(expected, options.shots));
    EXPECT_NEAR(results[1].at("comm_stats").at("mean_generation_time").get<double>(), 2e-6, 0.1e-6);
}


// ---------------------------------------------------------------------------------------
// Decoherence in simulated time
// ---------------------------------------------------------------------------------------

TEST(QCEngineTime, IdleRelaxationMatchesThermalRelaxation)
{
    // The data qubit waits a deterministic generation time tau before it is teleported.
    const double tau = 10e-6;
    struct Case { double t1, t2; };
    for (const auto& c : {Case{20e-6, 30e-6} /* amplitude damping (Kraus) */, Case{20e-6, 10e-6} /* reset mixture */}) {
        RunOptions options;
        options.shots = 5000;
        options.timing = ZERO_DURATIONS;
        options.comm_model = {
            {"data_qubits", {{"T1", c.t1}, {"T2", c.t2}}},
            {"comm_qubits", {{"T1", 1e3}, {"T2", 1e3}}},
            {"epr", {{"attempt_time", tau}}}
        };
        const double excited = std::exp(-tau / c.t1);
        const double plus = 0.5 * (1.0 + std::exp(-tau / c.t2));
        EXPECT_NEAR(1.0 - teledata_fidelity(PAULI_EIGENSTATES[1], options), 1.0 - excited, tolerance(excited, options.shots))
            << "T1 = " << c.t1 << ", T2 = " << c.t2;
        EXPECT_NEAR(teledata_fidelity(PAULI_EIGENSTATES[2], options), plus, tolerance(plus, options.shots))
            << "T1 = " << c.t1 << ", T2 = " << c.t2;
        EXPECT_NEAR(teledata_fidelity(PAULI_EIGENSTATES[0], options), 1.0, tolerance(1.0, options.shots));
    }
}

TEST(QCEngineTime, FidelityDecreasesWithInverseSuccessProbabilityAndAttemptTime)
{
    // |+> idles while its pair is generated: the expected coherence is the Laplace transform
    // of the generation time, E[exp(-T/T2)].
    const double t2 = 50e-6;
    struct Generator { double p, tau; };
    const std::vector<Generator> generators{{1.0, 1e-6}, {0.2, 1e-6}, {0.05, 1e-6}, {0.05, 4e-6}};

    double previous = 1.0;
    for (const auto& g : generators) {
        RunOptions options;
        options.shots = 6000;
        options.timing = ZERO_DURATIONS;
        options.comm_model = {
            {"data_qubits", {{"T2", t2}}},
            {"comm_qubits", {{"T1", 1e3}, {"T2", 1e3}}},
            {"epr", {{"success_probability", g.p}, {"attempt_time", g.tau}}}
        };
        const double expected = 0.5 * (1.0 + geometric_laplace(g.p, g.tau, 1.0 / t2));
        const double measured = teledata_fidelity(PAULI_EIGENSTATES[2], options);
        EXPECT_NEAR(measured, expected, tolerance(expected, options.shots)) << "p = " << g.p << ", tau = " << g.tau;
        EXPECT_LT(expected, previous);
        EXPECT_LT(measured, previous);
        previous = measured;
    }
}

TEST(QCEngineTime, ClassicalLatencyDegradesTeledata)
{
    // The receiver's half of the pair waits for the corrections for the latency of the
    // receiver. The corrections twirl its relaxation, so |+> keeps (1 + exp(-L/T2))/2.
    const double t2 = 20e-6;
    double previous = 1.01;
    for (double latency : {0.0, 10e-6, 40e-6}) {
        RunOptions options;
        options.shots = 5000;
        options.timing = ZERO_DURATIONS;
        options.timing["classical_latency"] = latency;
        options.comm_model = {{"comm_qubits", {{"T1", 1e3}, {"T2", t2}}}};
        const double expected = 0.5 * (1.0 + std::exp(-latency / t2));
        const double measured = teledata_fidelity(PAULI_EIGENSTATES[2], options);
        EXPECT_NEAR(measured, expected, tolerance(expected, options.shots)) << "latency = " << latency;
        EXPECT_LT(measured, previous);
        previous = measured;
    }
}

TEST(QCEngineTime, JitterIsSampledPerShot)
{
    // Exponential jitter of mean m on top of latency L: E[exp(-(L + J)/T2)] = e^(-L/T2) / (1 + m/T2).
    const double t2 = 20e-6, latency = 5e-6, mean_jitter = 10e-6;
    RunOptions options;
    options.shots = 6000;
    options.timing = ZERO_DURATIONS;
    options.timing["classical_latency"] = latency;
    options.timing["classical_jitter"] = {{"distribution", "exponential"}, {"mean", mean_jitter}};
    options.comm_model = {{"comm_qubits", {{"T1", 1e3}, {"T2", t2}}}};
    const double expected = 0.5 * (1.0 + std::exp(-latency / t2) / (1.0 + mean_jitter / t2));
    EXPECT_NEAR(teledata_fidelity(PAULI_EIGENSTATES[2], options), expected, tolerance(expected, options.shots));

    options.comm_model["strategy"] = "averaged";
    EXPECT_NEAR(teledata_fidelity(PAULI_EIGENSTATES[2], options), expected, tolerance(expected, options.shots));
}

TEST(QCEngineTiming, JitterSpreadsTheArrivalOfEachMessage)
{
    // Uniform jitter in [0, 1) us on 2 us of latency, without the noise model: the two
    // corrections travel in one message, so B finishes in [2, 3) us, at 2.5 us on average.
    Circuit a("A", 1, 1, 2), b("B", 1, 1, 3);
    a.gate("h", 0);
    qsend(a, 0, a.comm(), 0, 1, "B", "t");
    qrecv(b, 0, b.comm(), 0, 1, "A", "t");
    b.measure(0, 2);

    RunOptions options;
    options.shots = 2000;
    options.timing = ZERO_DURATIONS;
    options.timing["classical_latency"] = 2e-6;
    options.timing["classical_jitter"] = {{"distribution", "uniform"}, {"width", 1e-6}};
    const auto timing = run({a, b}, options)[1].at("simulated_time");
    EXPECT_GE(timing.at("min").get<double>(), 2e-6);
    EXPECT_LT(timing.at("max").get<double>(), 3e-6);
    EXPECT_GT(timing.at("max").get<double>() - timing.at("min").get<double>(), 0.9e-6);
    EXPECT_NEAR(timing.at("mean").get<double>(), 2.5e-6, 4.5 * 1e-6 / std::sqrt(12.0 * options.shots));
    // The data qubit waits for the message, a single draw per shot.
    EXPECT_NEAR(timing.at("idle_mean")[0].get<double>(), timing.at("mean").get<double>(), 1e-15);
}

TEST(QCEngineTime, SimulatedTimeFollowsTheTimeline)
{
    // Deterministic timeline: pair ready at 1 us, CX and SWAP 300 ns, measurements 1 us,
    // decisions 100 ns after the bits are known and 2 us of classical latency.
    RunOptions options;
    options.shots = 50;
    options.timing = {
        {"one_qubit_gate", 0.0},
        {"multi_qubit_gate", 300e-9},
        {"measure", 1e-6},
        {"reset", 0.0},
        {"classical_latency", 2e-6},
        {"feed_forward_latency", 100e-9}
    };
    options.comm_model = {{"epr", {{"attempt_time", 1e-6}}}};

    Circuit a("A", 1, 1, 2), b("B", 1, 1, 3);
    a.gate("h", 0);
    qsend(a, 0, a.comm(), 0, 1, "B", "t");
    qrecv(b, 0, b.comm(), 0, 1, "A", "t");
    b.measure(0, 2);
    const auto results = run({a, b}, options);

    // A: CX [1, 1.3], measurements [1.3, 2.3], resets of its qubits decided at 2.4.
    // B: corrections decided at 2.3 + 2 + 0.1 = 4.4, SWAP [4.4, 4.7], measurement [4.7, 5.7].
    EXPECT_NEAR(results[0].at("simulated_time").at("mean").get<double>(), 2.4e-6, 1e-12);
    EXPECT_NEAR(results[1].at("simulated_time").at("min").get<double>(), 5.7e-6, 1e-12);
    EXPECT_NEAR(results[1].at("simulated_time").at("max").get<double>(), 5.7e-6, 1e-12);
    EXPECT_NEAR(results[1].at("comm_stats").at("mean_generation_time").get<double>(), 1e-6, 1e-12);
}

TEST(QCEngineTime, PrefetchingHidesTheGenerationTime)
{
    // A needs 10 us of local work before teleporting. Requesting the pair on demand starts the
    // 5 us generation afterwards; requesting it first overlaps both.
    auto completion = [](bool prefetch) {
        Circuit a("A", 2, 1, 2), b("B", 1, 1, 3);
        if (prefetch) {
            a.gen_ent(a.comm(), {"B", "A"}, "t");
            b.gen_ent(b.comm(), {"A", "B"}, "t");
        }
        a.gate("cx", 0, 1);
        qsend(a, 0, a.comm(), 0, 1, "B", "t", prefetch);
        qrecv(b, 0, b.comm(), 0, 1, "A", "t", prefetch);
        b.measure(0, 2);

        RunOptions options;
        options.shots = 20;
        options.timing = ZERO_DURATIONS;
        options.timing["multi_qubit_gate"] = 10e-6;
        options.comm_model = {{"epr", {{"attempt_time", 5e-6}}}};
        const auto results = run({a, b}, options);
        return std::make_pair(results[0].at("simulated_time").at("mean").get<double>(),
                              results[1].at("simulated_time").at("mean").get<double>());
    };

    const auto on_demand = completion(false);
    const auto prefetched = completion(true);
    EXPECT_NEAR(on_demand.first, 25e-6, 1e-12);
    EXPECT_NEAR(on_demand.first - prefetched.first, 5e-6, 1e-12);
    EXPECT_NEAR(on_demand.second - prefetched.second, 5e-6, 1e-12);
}


// ---------------------------------------------------------------------------------------
// Per-shot sampling of the waiting time: exact and averaged strategies
// ---------------------------------------------------------------------------------------

TEST(QCEngineStrategy, ExactAndAveragedAgreeForASingleQubit)
{
    const double t2 = 50e-6, p = 0.05, tau = 1e-6;
    const double expected = 0.5 * (1.0 + geometric_laplace(p, tau, 1.0 / t2));
    for (const std::string strategy : {"exact", "averaged"}) {
        RunOptions options;
        options.shots = 6000;
        options.timing = ZERO_DURATIONS;
        options.comm_model = {
            {"strategy", strategy},
            {"data_qubits", {{"T2", t2}}},
            {"comm_qubits", {{"T1", 1e3}, {"T2", 1e3}}},
            {"epr", {{"success_probability", p}, {"attempt_time", tau}}}
        };
        EXPECT_NEAR(teledata_fidelity(PAULI_EIGENSTATES[2], options), expected, tolerance(expected, options.shots))
            << strategy;
    }
}

TEST(QCEngineStrategy, AveragedChannelMissesCorrelatedWaits)
{
    // B holds |Phi+> on two data qubits, both targets of remote CNOTs served by one pair
    // (cat-entangler fan-out) with the control in |0>. Both qubits dephase while the pair is
    // generated, during the same random time T. The XX parity of the pair keeps
    //   exact:     E[exp(-2T/T2)]       (the same T for both qubits)
    //   averaged:  E[exp(-T/T2)]^2      (each qubit averaged on its own)
    const double t2 = 20e-6, p = 0.02, tau = 0.2e-6;   // E[T] = T2 / 2
    const int shots = 40000;

    auto even_parity = [&](const std::string& strategy) {
        Circuit a("A", 1, 1, 2), b("B", 2, 1, 4);
        b.gate("h", 0).gate("cx", 0, 1);
        cat_entangler({&a, &b}, 0, {a.comm(), b.comm()}, {0, 0}, "fanout");
        b.gate("cx", b.comm(), 0).gate("cx", b.comm(), 1);
        cat_disentangler({&a, &b}, 0, {b.comm()}, {1}, {1});
        b.gate("h", 0).gate("h", 1).measure(0, 2).measure(1, 3);

        RunOptions options;
        options.shots = shots;
        options.timing = ZERO_DURATIONS;
        options.comm_model = {
            {"strategy", strategy},
            {"data_qubits", {{"T2", t2}}},
            {"comm_qubits", {{"T1", 1e3}, {"T2", 1e3}}},
            {"epr", {{"success_probability", p}, {"attempt_time", tau}}}
        };
        const auto result = run({a, b}, options)[1];
        return probability(result, "00") + probability(result, "11");
    };

    const double exact_expected = 0.5 * (1.0 + geometric_laplace(p, tau, 2.0 / t2));
    const double averaged_expected = 0.5 * (1.0 + std::pow(geometric_laplace(p, tau, 1.0 / t2), 2));
    const double exact = even_parity("exact");
    const double averaged = even_parity("averaged");

    EXPECT_NEAR(exact, exact_expected, tolerance(exact_expected, shots));
    EXPECT_NEAR(averaged, averaged_expected, tolerance(averaged_expected, shots));
    // The gap is resolved well beyond the statistical error of a difference of estimates.
    const double sigma_gap = std::sqrt(2.0 * 0.75 * 0.25 / shots);
    EXPECT_GT(exact - averaged, 6.0 * sigma_gap);

    std::cout << "[ gap      ] Bell XX parity with a shared random wait, E[T] = T2/2:\n"
              << "             exact    " << exact << " (expected " << exact_expected << ")\n"
              << "             averaged " << averaged << " (expected " << averaged_expected << ")\n"
              << "             gap      " << exact - averaged << " (expected " << exact_expected - averaged_expected << ")\n";
}

TEST(QCEngineStrategy, BinningApproachesTheExactStrategy)
{
    // Bins much narrower than T2 give the same statistics as unbinned delays.
    const double t2 = 50e-6, p = 0.05, tau = 1e-6;
    const double expected = 0.5 * (1.0 + geometric_laplace(p, tau, 1.0 / t2));
    RunOptions options;
    options.shots = 6000;
    options.timing = ZERO_DURATIONS;
    options.comm_model = {
        {"time_bin", 0.3e-6},
        {"data_qubits", {{"T2", t2}}},
        {"comm_qubits", {{"T1", 1e3}, {"T2", 1e3}}},
        {"epr", {{"success_probability", p}, {"attempt_time", tau}}}
    };
    EXPECT_NEAR(teledata_fidelity(PAULI_EIGENSTATES[2], options), expected, tolerance(expected, options.shots));
}


// ---------------------------------------------------------------------------------------
// Pauli-twirled variant on a stabilizer simulator
// ---------------------------------------------------------------------------------------

TEST(QCEnginePauli, TwirledRelaxationRunsOnStabilizer)
{
    // Amplitude damping cannot run on a stabilizer simulator: its twirl flips |0> and |1>
    // alike, with probability (1 - exp(-t/T1))/2.
    const double tau = 10e-6, t1 = 20e-6;
    RunOptions options;
    options.shots = 5000;
    options.method = "stabilizer";
    options.timing = ZERO_DURATIONS;
    options.comm_model = {
        {"data_qubits", {{"T1", t1}, {"T2", t1}}},
        {"comm_qubits", {{"T1", 1e3}, {"T2", 1e3}}},
        {"epr", {{"fidelity", 0.9}, {"attempt_time", tau}}}
    };
    const double depolarized = (2.0 * 0.9 + 1.0) / 3.0;
    const double flip = 0.5 * (1.0 - std::exp(-tau / t1));
    const double expected = depolarized * (1.0 - flip) + (1.0 - depolarized) * flip;
    for (const auto& state : {PAULI_EIGENSTATES[0], PAULI_EIGENSTATES[1]})
        EXPECT_NEAR(teledata_fidelity(state, options), expected, tolerance(expected, options.shots))
            << "state " << state.name;
}

TEST(QCEnginePauli, TelegateOnStabilizerGivesTheSameCorrelatedErrors)
{
    const std::array<double, 4> bell{0.6, 0.25, 0.1, 0.05};
    RunOptions options;
    options.shots = 6000;
    options.method = "stabilizer";
    options.comm_model = {{"noise_mode", "pauli"}, {"epr", {{"bell_coefficients", bell}}}};

    EXPECT_NEAR(telegate_odd_parity("z", options), bell[1] + bell[2], tolerance(0.35, options.shots));
    EXPECT_NEAR(telegate_odd_parity("x", options), bell[3] + bell[2], tolerance(0.15, options.shots));
    EXPECT_NEAR(telegate_odd_parity("y", options), bell[0] + bell[2], tolerance(0.7, options.shots));
}

TEST(QCEnginePauli, ExactModeIsRejectedOnStabilizer)
{
    RunOptions options;
    options.shots = 10;
    options.method = "stabilizer";
    options.comm_model = {{"noise_mode", "exact"}};
    EXPECT_THROW(teledata_fidelity(PAULI_EIGENSTATES[0], options), std::invalid_argument);
}


// ---------------------------------------------------------------------------------------
// Local operations keep the per-vQPU noise model
// ---------------------------------------------------------------------------------------

TEST(QCEngineDeviceNoise, GateAndReadoutErrorsFromNoiseProperties)
{
    const std::string path = ::testing::TempDir() + "qc_noise_properties.json";
    {
        std::ofstream file(path);
        file << R"json({
            "Qubits": {"q[0]": {"T1 (s)": 1e3, "T2 (s)": 1e3, "Readout fidelity (RB)": 0.9}},
            "Q1Gates": {"q[0]": {"X": {"Gate duration (s)": 0.0, "Fidelity(RB)": 0.95}}}
        })json";
    }

    Circuit a("A", 1, 1, 1);
    a.gate("x", 0).measure(0, 0);
    RunOptions options;
    options.shots = 8000;
    JSON backend_sections = JSON::object();
    backend_sections["noise_model"] = {
        {"noise_properties_path", path},
        {"thermal_relaxation", true},
        {"readout_error", true},
        {"gate_error", true}
    };
    options.backend_extra = {backend_sections};
    const auto results = run({a}, options);

    // A gate error flips |1> with probability e = 1 - F_RB, and the readout flips the bit
    // with probability 1 - F_readout.
    const double expected = 0.05 * 0.9 + 0.95 * 0.1;
    EXPECT_NEAR(probability(results[0], "0"), expected, tolerance(expected, options.shots));
    EXPECT_TRUE(results[0].contains("simulated_time"));
    std::remove(path.c_str());
}

TEST(QCEngineDeviceNoise, InvalidRunModelIsReported)
{
    RunOptions options;
    options.shots = 10;
    options.comm_model = {{"epr", {{"success_probability", 2.0}}}};
    EXPECT_THROW(teledata_fidelity(PAULI_EIGENSTATES[0], options), std::invalid_argument);
}


// ---------------------------------------------------------------------------------------
// Every simulator of the quantum-communications scheme
// ---------------------------------------------------------------------------------------

class QCEngineSimulators : public ::testing::TestWithParam<std::string> { };

TEST_P(QCEngineSimulators, IdealEntanglementWithoutModel)
{
    RunOptions options;
    options.simulator = GetParam();
    options.shots = 200;
    EXPECT_DOUBLE_EQ(teledata_fidelity(PAULI_EIGENSTATES[1], options), 1.0);
    EXPECT_DOUBLE_EQ(teledata_fidelity(PAULI_EIGENSTATES[2], options), 1.0);
}

TEST_P(QCEngineSimulators, PauliModelWithWernerPairAndIdleNoise)
{
    // Simulators without Kraus channels run the Pauli-twirled model: |+> keeps the Werner
    // depolarization and the twirled dephasing of its wait for the pair.
    const double F = 0.85, tau = 10e-6, t2 = 20e-6;
    RunOptions options;
    options.simulator = GetParam();
    options.shots = 3000;
    options.timing = ZERO_DURATIONS;
    options.comm_model = {
        {"noise_mode", "pauli"},
        {"data_qubits", {{"T2", t2}}},
        {"comm_qubits", {{"T1", 1e3}, {"T2", 1e3}}},
        {"epr", {{"fidelity", F}, {"attempt_time", tau}}}
    };
    const double depolarized = (2.0 * F + 1.0) / 3.0;
    const double flip = 0.5 * (1.0 - std::exp(-tau / t2));
    const double expected = depolarized * (1.0 - flip) + (1.0 - depolarized) * flip;
    EXPECT_NEAR(teledata_fidelity(PAULI_EIGENSTATES[2], options), expected, tolerance(expected, options.shots));
}

INSTANTIATE_TEST_SUITE_P(
    AllSimulators, QCEngineSimulators,
    ::testing::Values("Aer", "Munich", "Qulacs", "Qsim", "Quest", "Cunqa"),
    [](const ::testing::TestParamInfo<std::string>& info) { return info.param; });
