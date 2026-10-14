#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "comm_model.hpp"

using namespace cunqa;
using namespace cunqa::sim;

namespace {

JSON backend(int data, int comm, JSON comm_model = nullptr, JSON timing = nullptr)
{
    JSON b = {{"name", "QCBackend"}, {"num_qubits", {data, comm}}};
    if (!comm_model.is_null())
        b["comm_model"] = comm_model;
    if (!timing.is_null())
        b["timing"] = timing;
    return b;
}

} // End of anonymous namespace

TEST(CommModel, DisabledWithoutModelSections)
{
    const auto model = CommModel::from_backends({{"A", backend(2, 1)}, {"B", backend(3, 2)}});
    EXPECT_FALSE(model.enabled);
    EXPECT_FALSE(model.any_timed());
    ASSERT_EQ(model.vqpus.size(), 2u);
    EXPECT_EQ(model.vqpus[0].first_qubit, 0u);
    EXPECT_EQ(model.vqpus[1].first_qubit, 3u);
    EXPECT_EQ(model.vqpus[1].num_comm_qubits, 2u);
    EXPECT_TRUE(model.vqpus[1].is_comm_qubit(3));
    EXPECT_FALSE(model.vqpus[1].is_comm_qubit(2));

    // Ideal links by default: certain, instantaneous and perfect pairs.
    const auto& epr = model.epr("A", "B");
    EXPECT_DOUBLE_EQ(epr.delay()->mean(), 0.0);
    EXPECT_DOUBLE_EQ(epr.mean_bell()[0], 1.0);
}

TEST(CommModel, TimingBlocksAreReadPerVQPU)
{
    const JSON timing = {{"multi_qubit_gate", 4e-7}, {"classical_latency", 1e-6}, {"entanglement", 2e-6}};
    const auto model = CommModel::from_backends({{"A", backend(1, 1, nullptr, timing)}, {"B", backend(1, 1)}});
    // A timing block alone gives simulated time, not the noise model.
    EXPECT_FALSE(model.enabled);
    EXPECT_TRUE(model.any_timed());
    EXPECT_FALSE(model.all_timed());

    ASSERT_TRUE(model.vqpus[0].timing.has_value());
    EXPECT_DOUBLE_EQ(model.vqpus[0].timing->duration(InstructionType::CX, 2), 4e-7);
    EXPECT_DOUBLE_EQ(model.vqpus[0].timing->classical_latency(), 1e-6);
    EXPECT_DOUBLE_EQ(model.vqpus[0].timing->entanglement_duration(), 2e-6);
    EXPECT_FALSE(model.vqpus[1].timing.has_value());

    EXPECT_THROW(CommModel::from_backends({{"A", backend(1, 1, nullptr, {{"mesure", 1e-6}})}}), std::invalid_argument);
}

TEST(CommModel, BackendSectionsEnableTheModel)
{
    const JSON comm = {
        {"comm_qubits", {{"T1", 1e-3}, {"T2", 5e-4}}},
        {"epr", {{"fidelity", 0.9}, {"success_probability", 0.1}, {"attempt_time", 1e-6}}}
    };
    const auto model = CommModel::from_backends({{"A", backend(1, 1, comm)}, {"B", backend(1, 1, comm)}});
    EXPECT_TRUE(model.enabled);
    EXPECT_EQ(model.strategy, WaitStrategy::EXACT);
    EXPECT_EQ(model.noise_mode, NoiseMode::AUTO);
    EXPECT_DOUBLE_EQ(model.time_bin, 0.0);

    ASSERT_NE(model.vqpus[0].coherence(1), nullptr);
    EXPECT_DOUBLE_EQ(model.vqpus[0].coherence(1)->T2, 5e-4);
    EXPECT_EQ(model.vqpus[0].coherence(0), nullptr);

    const auto& epr = model.epr("B", "A");
    EXPECT_NEAR(epr.delay()->mean(), 1e-5, 1e-18);
    EXPECT_NEAR(epr.mean_bell()[0], 0.9, 1e-12);
}

TEST(CommModel, CoherencePrecedence)
{
    const JSON comm = {
        {"data_qubits", {{"T1", 1e-4}, {"T2", 1e-4}}},
        {"comm_qubits", {{"T1", 1e-3}, {"T2", 2e-3}}}
    };
    const auto model = CommModel::from_backends({{"A", backend(2, 1, comm)}});
    const auto& vqpu = model.vqpus[0];
    ASSERT_NE(vqpu.coherence(0), nullptr);
    EXPECT_DOUBLE_EQ(vqpu.coherence(0)->T1, 1e-4);
    ASSERT_NE(vqpu.coherence(2), nullptr);
    EXPECT_DOUBLE_EQ(vqpu.coherence(2)->T2, 2e-3);

    // Communication qubits fall back to the data-qubit times.
    const auto data_only = CommModel::from_backends({{"A", backend(2, 1, {{"data_qubits", {{"T2", 3e-5}}}})}});
    ASSERT_NE(data_only.vqpus[0].coherence(2), nullptr);
    EXPECT_DOUBLE_EQ(data_only.vqpus[0].coherence(2)->T2, 3e-5);
    EXPECT_TRUE(std::isinf(data_only.vqpus[0].coherence(2)->T1));
}

TEST(CommModel, RunOverridesMergeIntoTheDeployment)
{
    const JSON comm = {{"epr", {{"fidelity", 0.9}, {"success_probability", 0.5}, {"attempt_time", 1e-6}}}};
    const auto deployed = CommModel::from_backends({{"A", backend(1, 1, comm)}, {"B", backend(1, 1, comm)}});

    const JSON run = {
        {"strategy", "averaged"},
        {"noise_mode", "pauli"},
        {"time_bin", 1e-8},
        {"epr", {{"success_probability", 0.1}}},
        {"comm_qubits", {{"T2", 1e-4}}}
    };
    const auto model = deployed.with_run_overrides(run);
    EXPECT_EQ(model.strategy, WaitStrategy::AVERAGED);
    EXPECT_EQ(model.noise_mode, NoiseMode::PAULI);
    EXPECT_DOUBLE_EQ(model.time_bin, 1e-8);
    // Only the success probability changes: fidelity and attempt time are kept.
    EXPECT_NEAR(model.epr("A", "B").delay()->mean(), 1e-5, 1e-18);
    EXPECT_NEAR(model.epr("A", "B").mean_bell()[0], 0.9, 1e-12);
    for (const auto& vqpu : model.vqpus) {
        ASSERT_NE(vqpu.coherence(1), nullptr);
        EXPECT_DOUBLE_EQ(vqpu.coherence(1)->T2, 1e-4);
    }

    // The deployment model is left untouched.
    EXPECT_EQ(deployed.strategy, WaitStrategy::EXACT);
    EXPECT_NEAR(deployed.epr("A", "B").delay()->mean(), 2e-6, 1e-18);

    // An empty run model enables the noise model with the deployment parameters.
    EXPECT_TRUE(CommModel::from_backends({{"A", backend(1, 1)}}).with_run_overrides(JSON::object()).enabled);
    EXPECT_FALSE(CommModel::from_backends({{"A", backend(1, 1)}}).with_run_overrides(JSON()).enabled);
}

TEST(CommModel, PerLinkParameters)
{
    const auto deployed = CommModel::from_backends({{"A", backend(1, 1)}, {"B", backend(1, 1)}, {"C", backend(1, 1)}});
    const JSON run = {
        {"epr", {{"fidelity", 0.95}, {"attempt_time", 1e-6}}},
        {"links", {
            {{"qpus", {"C", "A"}}, {"epr", {{"success_probability", 0.25}}}}
        }}
    };
    const auto model = deployed.with_run_overrides(run);
    EXPECT_NEAR(model.epr("A", "C").delay()->mean(), 4e-6, 1e-18);
    EXPECT_NEAR(model.epr("A", "C").mean_bell()[0], 0.95, 1e-12);
    EXPECT_NEAR(model.epr("A", "B").delay()->mean(), 1e-6, 1e-18);
}

TEST(CommModel, TabulatedSource)
{
    const JSON run = {{"epr", {
        {"source", "tabulated"},
        {"samples", {
            {{"delay", 1e-6}, {"fidelity", 1.0}, {"weight", 3.0}},
            {{"delay", 5e-6}, {"bell_coefficients", {0.5, 0.5, 0.0, 0.0}}, {"weight", 1.0}}
        }}
    }}};
    const auto model = CommModel::from_backends({{"A", backend(1, 1)}, {"B", backend(1, 1)}}).with_run_overrides(run);
    const auto& epr = model.epr("A", "B");
    EXPECT_NEAR(epr.delay()->mean(), 2e-6, 1e-18);
    EXPECT_NEAR(epr.mean_bell()[0], 0.875, 1e-12);
    EXPECT_NEAR(epr.mean_bell()[1], 0.125, 1e-12);

    // Each draw keeps the delay and the state of the same sample.
    Rng rng(3);
    int slow = 0;
    for (int i = 0; i < 4000; ++i) {
        const auto draw = epr.sample(rng);
        if (draw.delay > 2e-6) {
            ++slow;
            EXPECT_DOUBLE_EQ(draw.bell[0], 0.5);
        } else {
            EXPECT_DOUBLE_EQ(draw.bell[0], 1.0);
        }
    }
    EXPECT_NEAR(slow / 4000.0, 0.25, 0.03);
}

TEST(CommModel, TabulatedSourceFromFile)
{
    const std::string path = ::testing::TempDir() + "epr_table.json";
    {
        std::ofstream file(path);
        file << R"json({"samples": [{"delay": 2e-6, "fidelity": 0.8}]})json";
    }
    const JSON run = {{"epr", {{"source", "tabulated"}, {"path", path}}}};
    const auto model = CommModel::from_backends({{"A", backend(1, 1)}, {"B", backend(1, 1)}}).with_run_overrides(run);
    EXPECT_NEAR(model.epr("A", "B").delay()->mean(), 2e-6, 1e-18);
    EXPECT_NEAR(model.epr("A", "B").mean_bell()[0], 0.8, 1e-12);
    std::remove(path.c_str());
}

TEST(CommModel, InvalidModelsAreRejected)
{
    auto build = [](const JSON& run) {
        return CommModel::from_backends({{"A", backend(1, 1)}, {"B", backend(1, 1)}}).with_run_overrides(run);
    };
    EXPECT_THROW(build({{"epr", {{"fidelty", 0.9}}}}), std::invalid_argument);
    EXPECT_THROW(build({{"unknown", 1}}), std::invalid_argument);
    EXPECT_THROW(build({{"strategy", "fast"}}), std::invalid_argument);
    EXPECT_THROW(build({{"noise_mode", "kraus"}}), std::invalid_argument);
    EXPECT_THROW(build({{"epr", {{"success_probability", 0.0}}}}), std::invalid_argument);
    EXPECT_THROW(build({{"epr", {{"fidelity", 0.9}, {"bell_coefficients", {1, 0, 0, 0}}}}}), std::invalid_argument);
    EXPECT_THROW(build({{"links", {{{"circuits", {"c1", "c2"}}}}}}), std::invalid_argument);
    EXPECT_THROW(build({{"links", {{{"qpus", {"A"}}}}}}), std::invalid_argument);
    EXPECT_THROW(build({{"time_bin", -1e-9}}), std::invalid_argument);
    // Durations and latencies belong to the timing block of each vQPU.
    EXPECT_THROW(build({{"durations", {{"gate_1q", 5e-8}}}}), std::invalid_argument);
    EXPECT_THROW(build({{"classical_link", {{"latency", 1e-6}}}}), std::invalid_argument);
}

TEST(CommModel, BackendsMustAgreeOnTheNetwork)
{
    const JSON a = {{"epr", {{"fidelity", 0.9}}}};
    const JSON b = {{"epr", {{"fidelity", 0.8}}}};
    EXPECT_THROW(CommModel::from_backends({{"A", backend(1, 1, a)}, {"B", backend(1, 1, b)}}), std::invalid_argument);
    // A vQPU that does not set the network does not disagree.
    EXPECT_NO_THROW(CommModel::from_backends({{"A", backend(1, 1, a)}, {"B", backend(1, 1)}}));
}
