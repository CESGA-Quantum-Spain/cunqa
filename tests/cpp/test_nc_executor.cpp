#include <memory>

#include <gtest/gtest.h>

#include "AER/AER_simulator_adapter.hpp"
#include "nc_executor.hpp"

using namespace cunqa;
using namespace cunqa::sim;

TEST(NCExecutorTiming, SkippedCorrectionWaitsForItsCondition)
{
    auto simulator = std::make_unique<AERSimulatorAdapter>();
    simulator->num_qubits = 2;
    NCExecutor executor;
    executor.set_simulator(std::move(simulator));
    executor.set_timing(TimingModel(JSON::object()));  // gates 0.05 us, measurements 1 us

    // q0 is measured into c0, which controls an X on q1. No seed: both outcomes must appear.
    const JSON task = {
        {"config", {
            {"qpu_id", "A"}, {"shots", 400}, {"method", "statevector"},
            {"avoid_parallelization", false}, {"is_dynamic", true}, {"num_clbits", 2},
            {"device", {{"device_name", "CPU"}}}
        }},
        {"instructions", JSON::array({
            {{"name", "h"}, {"qubits", 0}},
            {{"name", "measure"}, {"qubits", 0}, {"clbits", 0}, {"save", true}},
            {{"name", "cif"}, {"clbits", {0}}, {"condition", 1}, {"operation", "and"}},
            {{"name", "x"}, {"qubits", 1}},
            {{"name", "endcif"}, {"clbits", {0}}},
            {{"name", "measure"}, {"qubits", 1}, {"clbits", 1}, {"save", true}}
        })}
    };
    const auto result = executor.execute(task);
    const auto& time = result.at("simulated_time");

    // c0 is known at 1.05 us. If it is 1, X runs in [1.05, 1.1) and q1 is measured until
    // 2.1 us. If it is 0, X is skipped but q1 still waits for c0: measured in [1.05, 2.05).
    EXPECT_NEAR(time.at("min").get<double>(), 2.05e-6, 1e-12);
    EXPECT_NEAR(time.at("max").get<double>(), 2.1e-6, 1e-12);
    EXPECT_NEAR(time.at("idle_mean")[1].get<double>(), 1.05e-6, 1e-12);
}
