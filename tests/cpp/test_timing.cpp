#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "sim/timing.hpp"

using namespace cunqa;
using namespace cunqa::sim;

namespace {

constexpr double EPS = 1e-15;

TimingModel model(JSON timing)
{
    return TimingModel(timing);
}

} // End of anonymous namespace

TEST(TimingModel, DefaultsAreInSeconds)
{
    const TimingModel timing(JSON::object());
    EXPECT_DOUBLE_EQ(timing.duration(InstructionType::H, 1), 5e-8);
    EXPECT_DOUBLE_EQ(timing.duration(InstructionType::CX, 2), 3e-7);
    EXPECT_DOUBLE_EQ(timing.duration(InstructionType::MEASURE, 1), 1e-6);
    EXPECT_DOUBLE_EQ(timing.duration(InstructionType::RESET, 1), 1e-6);
    EXPECT_DOUBLE_EQ(timing.duration(InstructionType::DEPOLARIZINGNOISE, 1), 0.0);
    EXPECT_DOUBLE_EQ(timing.classical_latency(), 0.0);
    EXPECT_EQ(timing.classical_jitter(), nullptr);
    EXPECT_DOUBLE_EQ(timing.feed_forward_latency(), 0.0);
    EXPECT_DOUBLE_EQ(timing.entanglement_duration(), 0.0);
}

TEST(TimingModel, PerGateDurationsAndValidation)
{
    const auto timing = model({{"multi_qubit_gate", 4e-7}, {"gate_durations", {{"cx", 2.5e-7}, {"rz", 0.0}}}});
    EXPECT_DOUBLE_EQ(timing.duration(InstructionType::CX, 2), 2.5e-7);
    EXPECT_DOUBLE_EQ(timing.duration(InstructionType::CZ, 2), 4e-7);
    EXPECT_DOUBLE_EQ(timing.duration(InstructionType::RZ, 1), 0.0);

    EXPECT_THROW(model({{"one_qubit_gte", 1e-8}}), std::invalid_argument);
    EXPECT_THROW(model({{"measure", -1e-6}}), std::invalid_argument);
}

TEST(TimingModel, ClassicalJitterAndFeedForwardLatency)
{
    const auto uniform = model({{"classical_jitter", {{"distribution", "uniform"}, {"width", 4e-7}}},
                                {"feed_forward_latency", 1e-7}});
    ASSERT_NE(uniform.classical_jitter(), nullptr);
    EXPECT_DOUBLE_EQ(uniform.classical_jitter()->mean(), 2e-7);
    EXPECT_DOUBLE_EQ(uniform.classical_jitter()->min(), 0.0);
    EXPECT_DOUBLE_EQ(uniform.feed_forward_latency(), 1e-7);

    const auto exponential = model({{"classical_jitter", {{"distribution", "exponential"}, {"mean", 3e-7}}}});
    ASSERT_NE(exponential.classical_jitter(), nullptr);
    EXPECT_DOUBLE_EQ(exponential.classical_jitter()->mean(), 3e-7);

    // A zero jitter is no jitter.
    EXPECT_EQ(model({{"classical_jitter", {{"distribution", "uniform"}, {"width", 0.0}}}}).classical_jitter(), nullptr);

    EXPECT_THROW(model({{"classical_jitter", {{"distribution", "gaussian"}, {"mean", 1e-7}}}}), std::invalid_argument);
    EXPECT_THROW(model({{"classical_jitter", {{"distribution", "uniform"}, {"mean", 1e-7}}}}), std::invalid_argument);
    EXPECT_THROW(model({{"classical_jitter", {{"distribution", "exponential"}}}}), std::invalid_argument);
    EXPECT_THROW(model({{"classical_jitter", {{"distribution", "exponential"}, {"mean", -1e-7}}}}), std::invalid_argument);
    EXPECT_THROW(model({{"classical_jitter", 1e-7}}), std::invalid_argument);
    EXPECT_THROW(model({{"feed_forward_latency", -1e-7}}), std::invalid_argument);
}

TEST(SimClock, SchedulesOperationsAsSoonAsPossible)
{
    SimClock clock(model({{"one_qubit_gate", 1e-7}, {"multi_qubit_gate", 3e-7}}), 3);
    clock.apply(InstructionType::H, OneQubitNoParam{0});           // q0: [0, 0.1) us
    clock.apply(InstructionType::H, OneQubitNoParam{2});           // q2: [0, 0.1) us, in parallel
    clock.apply(InstructionType::CX, TwoQubitNoParam{{0, 1}});     // [0.1, 0.4) us, q1 idles 0.1 us

    EXPECT_NEAR(clock.qubit_time(0), 4e-7, EPS);
    EXPECT_NEAR(clock.qubit_time(1), 4e-7, EPS);
    EXPECT_NEAR(clock.qubit_time(2), 1e-7, EPS);
    EXPECT_NEAR(clock.now(), 4e-7, EPS);
    EXPECT_NEAR(clock.idle_times()[1], 1e-7, EPS);
    EXPECT_NEAR(clock.idle_times()[2], 0.0, EPS);
}

TEST(SimClock, IdleHookReportsEveryWait)
{
    SimClock clock(model({{"one_qubit_gate", 1e-7}, {"multi_qubit_gate", 3e-7}}), 2, 10);
    std::vector<std::tuple<std::size_t, double, double>> idles;
    clock.idle_hook = [&](std::size_t qubit, double start, double duration) {
        idles.emplace_back(qubit, start, duration);
    };

    clock.apply(InstructionType::H, OneQubitNoParam{10});
    clock.apply(InstructionType::H, OneQubitNoParam{10});
    clock.apply(InstructionType::CX, TwoQubitNoParam{{10, 11}});

    // Only qubit 11 waits, from 0 until the CX starts at 0.2 us. Qubits keep their global index.
    ASSERT_EQ(idles.size(), 1u);
    EXPECT_EQ(std::get<0>(idles[0]), 11u);
    EXPECT_NEAR(std::get<1>(idles[0]), 0.0, EPS);
    EXPECT_NEAR(std::get<2>(idles[0]), 2e-7, EPS);
}

TEST(SimClock, MeasurementsMakeTheirBitAvailable)
{
    SimClock clock(model({{"one_qubit_gate", 1e-7}, {"measure", 1e-6}}), 1);
    clock.apply(InstructionType::H, OneQubitNoParam{0});
    clock.apply(InstructionType::MEASURE, Measure{0, 3, true});
    EXPECT_NEAR(clock.clbit_time(3), 1.1e-6, EPS);
    EXPECT_NEAR(clock.classical_time({3}), 1.1e-6, EPS);
}

TEST(SimClock, ExecutedConditionalBlockWaitsForItsBits)
{
    SimClock clock(model({{"one_qubit_gate", 1e-7}, {"measure", 1e-6}}), 2);
    clock.apply(InstructionType::MEASURE, Measure{0, 0, true});    // bit known at 1 us
    clock.begin_conditional({0});
    clock.apply(InstructionType::X, OneQubitNoParam{1});           // [1, 1.1) us
    clock.end_conditional();

    EXPECT_NEAR(clock.qubit_time(1), 1.1e-6, EPS);
    EXPECT_NEAR(clock.idle_times()[1], 1e-6, EPS);
}

TEST(SimClock, SkippedConditionalBlockStillWaitsForItsBits)
{
    SimClock clock(model({{"one_qubit_gate", 1e-7}, {"measure", 1e-6}}), 3);
    clock.apply(InstructionType::MEASURE, Measure{0, 0, true});    // bit known at 1 us
    clock.skip_conditional({0}, {1, 1});                            // two gates on q1, not applied

    // q1 cannot start anything before the decision, and that wait is idle time, counted once.
    EXPECT_NEAR(clock.qubit_time(1), 1e-6, EPS);
    EXPECT_NEAR(clock.idle_times()[1], 1e-6, EPS);
    // Qubits outside the block are not affected.
    EXPECT_NEAR(clock.qubit_time(2), 0.0, EPS);

    clock.apply(InstructionType::MEASURE, Measure{1, 1, true});
    EXPECT_NEAR(clock.clbit_time(1), 2e-6, EPS);
}

TEST(SimClock, FeedForwardLatencyDelaysEveryDecision)
{
    SimClock clock(model({{"one_qubit_gate", 1e-7}, {"measure", 1e-6}, {"feed_forward_latency", 2e-7}}), 3);
    clock.apply(InstructionType::MEASURE, Measure{0, 0, true});    // bit known at 1 us
    EXPECT_NEAR(clock.decision_time({0}), 1.2e-6, EPS);

    clock.begin_conditional({0});
    clock.apply(InstructionType::X, OneQubitNoParam{1});           // [1.2, 1.3) us
    clock.end_conditional();
    EXPECT_NEAR(clock.qubit_time(1), 1.3e-6, EPS);

    clock.skip_conditional({0}, {2});
    EXPECT_NEAR(clock.qubit_time(2), 1.2e-6, EPS);
    // Sending a bit does not wait for any decision.
    EXPECT_NEAR(clock.classical_time({0}), 1e-6, EPS);
}

TEST(SimClock, ReceivedBitsArriveAfterTheLatency)
{
    SimClock clock(model({{"classical_latency", 5e-7}}), 1);
    clock.receive_clbit(0, 2e-6);
    EXPECT_NEAR(clock.clbit_time(0), 2.5e-6, EPS);
    EXPECT_NEAR(clock.now(), 2.5e-6, EPS);

    // The jitter drawn for a message adds to the latency.
    clock.receive_clbit(1, 2e-6, 3e-7);
    EXPECT_NEAR(clock.clbit_time(1), 2.8e-6, EPS);
}

TEST(SimClock, OccupySpansSeveralVQPUs)
{
    SimClock clock(model(JSON::object()), 2, 4);
    EXPECT_TRUE(clock.owns_qubit(5));
    EXPECT_FALSE(clock.owns_qubit(3));
    clock.occupy(5, 1e-6, 3e-6);
    EXPECT_NEAR(clock.qubit_time(5), 3e-6, EPS);
    EXPECT_NEAR(clock.idle_times()[1], 1e-6, EPS);
    EXPECT_THROW(clock.occupy(3, 0.0, 1e-6), std::out_of_range);
}

TEST(TimingStats, ReportsSecondsPerShot)
{
    SimClock clock(model({{"one_qubit_gate", 1e-7}}), 2);
    TimingStats stats;

    clock.apply(InstructionType::H, OneQubitNoParam{0});
    stats.add_shot(clock);
    clock.reset();
    clock.apply(InstructionType::H, OneQubitNoParam{0});
    clock.apply(InstructionType::H, OneQubitNoParam{0});
    stats.add_shot(clock);

    const auto json = stats.to_json();
    EXPECT_EQ(json.at("unit"), "s");
    EXPECT_NEAR(json.at("mean").get<double>(), 1.5e-7, EPS);
    EXPECT_NEAR(json.at("min").get<double>(), 1e-7, EPS);
    EXPECT_NEAR(json.at("max").get<double>(), 2e-7, EPS);
    EXPECT_EQ(json.at("idle_mean").size(), 2u);
}
