#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "AER/AER_simulator_adapter.hpp"
#include "CUNQA/cunqa_simulator_adapter.hpp"
#include "Munich/munich_simulator_adapter.hpp"
#include "Qsim/qsim_simulator_adapter.hpp"
#include "QuEST/quest_simulator_adapter.hpp"
#include "Qulacs/qulacs_simulator_adapter.hpp"
#include "comm_model.hpp"
#include "qc_engine.hpp"
#include "utils/json.hpp"

namespace test {

using cunqa::JSON;

// Circuit of one vQPU, written in the JSON instructions that cunqa.qpu.run sends once the
// circuit ids of the communication directives have been replaced by vQPU ids.
struct Circuit {
    std::string qpu;
    std::size_t num_data;
    std::size_t num_comm;
    std::size_t num_clbits;
    JSON instructions = JSON::array();

    Circuit(std::string qpu_id, std::size_t data, std::size_t comm, std::size_t clbits)
        : qpu{std::move(qpu_id)}, num_data{data}, num_comm{comm}, num_clbits{clbits}
    { }

    std::size_t comm(std::size_t k = 0) const { return num_data + k; }

    Circuit& gate(const std::string& name, std::size_t q)
    {
        instructions.push_back({{"name", name}, {"qubits", q}});
        return *this;
    }

    Circuit& gate(const std::string& name, std::size_t a, std::size_t b)
    {
        instructions.push_back({{"name", name}, {"qubits", {a, b}}});
        return *this;
    }

    Circuit& reset(std::size_t q)
    {
        instructions.push_back({{"name", "reset"}, {"qubits", {q}}});
        return *this;
    }

    Circuit& measure(std::size_t q, std::size_t c, bool save = true)
    {
        instructions.push_back({{"name", "measure"}, {"qubits", q}, {"clbits", c}, {"save", save}});
        return *this;
    }

    Circuit& cif(std::vector<std::size_t> clbits, const std::string& operation = "and")
    {
        instructions.push_back({{"name", "cif"}, {"clbits", clbits}, {"condition", 1}, {"operation", operation}});
        return *this;
    }

    Circuit& endcif()
    {
        instructions.push_back({{"name", "endcif"}, {"clbits", {0}}});
        return *this;
    }

    Circuit& send(std::vector<std::size_t> clbits, const std::string& to)
    {
        instructions.push_back({{"name", "send"}, {"clbits", clbits}, {"qpus", {to}}});
        return *this;
    }

    Circuit& recv(std::vector<std::size_t> clbits, const std::string& from)
    {
        instructions.push_back({{"name", "recv"}, {"clbits", clbits}, {"qpus", {from}}});
        return *this;
    }

    Circuit& gen_ent(std::size_t comm_qubit, std::vector<std::string> qpus, const std::string& tag)
    {
        instructions.push_back({{"name", "gen_ent"}, {"comm_qubit", comm_qubit}, {"qpus", qpus}, {"tag", tag}});
        return *this;
    }
};

// The protocols of cunqa.qc_protocols, instruction by instruction.
inline void qsend(Circuit& c, std::size_t data, std::size_t comm, std::size_t c0, std::size_t c1,
                  const std::string& to, const std::string& tag, bool prefetched = false)
{
    if (!prefetched)
        c.gen_ent(comm, {to, c.qpu}, tag);
    c.gate("cx", data, comm).gate("h", data);
    c.measure(data, c0, false).measure(comm, c1, false);
    c.cif({c0}).gate("x", data).endcif();
    c.cif({c1}).gate("x", comm).endcif();
    c.send({c0, c1}, to);
}

inline void qrecv(Circuit& c, std::size_t data, std::size_t comm, std::size_t c0, std::size_t c1,
                  const std::string& from, const std::string& tag, bool prefetched = false)
{
    if (!prefetched)
        c.gen_ent(comm, {from, c.qpu}, tag);
    c.recv({c0, c1}, from);
    c.cif({c1}).gate("x", comm).endcif();
    c.cif({c0}).gate("z", comm).endcif();
    c.gate("swap", comm, data);
}

inline void cat_entangler(std::vector<Circuit*> circuits, std::size_t data, std::vector<std::size_t> comms,
                          std::vector<std::size_t> clbits, const std::string& tag)
{
    std::vector<std::string> ids;
    for (const auto* c : circuits)
        ids.push_back(c->qpu);
    for (std::size_t k = 0; k < circuits.size(); ++k)
        circuits[k]->gen_ent(comms[k], ids, tag);

    auto& owner = *circuits[0];
    owner.gate("cx", data, comms[0]).measure(comms[0], clbits[0], false);
    owner.cif({clbits[0]}).gate("x", comms[0]).endcif();
    for (std::size_t k = 1; k < circuits.size(); ++k)
        owner.send({clbits[0]}, circuits[k]->qpu);
    for (std::size_t k = 1; k < circuits.size(); ++k) {
        circuits[k]->recv({clbits[k]}, owner.qpu);
        circuits[k]->cif({clbits[k]}).gate("x", comms[k]).endcif();
    }
}

inline void cat_disentangler(std::vector<Circuit*> circuits, std::size_t data, std::vector<std::size_t> comms,
                             std::vector<std::size_t> recv_clbits, std::vector<std::size_t> send_clbits)
{
    auto& owner = *circuits[0];
    for (std::size_t k = 1; k < circuits.size(); ++k)
        owner.recv({recv_clbits[k - 1]}, circuits[k]->qpu);
    owner.cif(recv_clbits, "xor").gate("z", data).endcif();
    for (std::size_t k = 1; k < circuits.size(); ++k) {
        auto& c = *circuits[k];
        c.gate("h", comms[k - 1]).measure(comms[k - 1], send_clbits[k - 1], false);
        c.cif({send_clbits[k - 1]}).gate("x", comms[k - 1]).endcif();
        c.send({send_clbits[k - 1]}, owner.qpu);
    }
}

// Like cunqa::sim::make_simulator, without Maestro: outside an installed CUNQA its library
// cannot load its plugins and double-frees in its global destructors at exit, which makes
// the test processes abort. system_tests/check_comm_noise.py --simulator Maestro covers it.
inline std::shared_ptr<cunqa::sim::Simulator> make_test_simulator(const std::string& name)
{
    using namespace cunqa::sim;
    if (name == "Aer")
        return std::make_shared<AERSimulatorAdapter>();
    if (name == "Cunqa")
        return std::make_shared<CunqaSimulatorAdapter>();
    if (name == "Munich")
        return std::make_shared<MunichSimulatorAdapter>();
    if (name == "Qsim")
        return std::make_shared<QsimSimulatorAdapter>();
    if (name == "Quest")
        return std::make_shared<QuestSimulatorAdapter>();
    if (name == "Qulacs")
        return std::make_shared<QulacsSimulatorAdapter>();
    throw std::invalid_argument("Simulator " + name + " is not available in the tests.");
}

// Timing block in which every operation is instantaneous, so that only the waits for the
// entangled pairs and the classical messages take simulated time.
inline const JSON ZERO_DURATIONS = {
    {"one_qubit_gate", 0.0},
    {"multi_qubit_gate", 0.0},
    {"measure", 0.0},
    {"reset", 0.0}
};

struct RunOptions {
    std::string simulator = "Aer";
    int shots = 2000;
    int seed = 11;
    std::string method = "statevector";
    JSON comm_model;                  // "comm_model" run parameter, null for none
    JSON timing;                      // "timing" block of every vQPU, null for none
    std::vector<JSON> backend_extra;  // extra backend sections of each vQPU, e.g. noise_model
};

// Runs the circuits as the quantum-communications executor does and returns the result of
// each one.
inline std::vector<JSON> run(const std::vector<Circuit>& circuits, const RunOptions& options = {})
{
    using namespace cunqa;
    using namespace cunqa::sim;

    std::vector<std::pair<std::string, JSON>> backends;
    std::size_t num_qubits = 0;
    for (std::size_t i = 0; i < circuits.size(); ++i) {
        JSON backend = {{"num_qubits", {circuits[i].num_data, circuits[i].num_comm}}};
        if (!options.timing.is_null())
            backend["timing"] = options.timing;
        if (i < options.backend_extra.size() && options.backend_extra[i].is_object())
            backend.update(options.backend_extra[i]);
        backends.emplace_back(circuits[i].qpu, backend);
        num_qubits += circuits[i].num_data + circuits[i].num_comm;
    }

    auto simulator = make_test_simulator(options.simulator);
    simulator->num_qubits = num_qubits;

    std::vector<QuantumTask> tasks;
    std::vector<RunConfig> configs;
    for (const auto& circuit : circuits) {
        JSON config = {
            {"qpu_id", circuit.qpu},
            {"shots", options.shots},
            {"method", options.method},
            {"avoid_parallelization", false},
            {"is_dynamic", true},
            {"num_clbits", circuit.num_clbits},
            {"seed", options.seed},
            {"device", {{"device_name", "CPU"}}}
        };
        if (!options.comm_model.is_null())
            config["comm_model"] = options.comm_model;
        configs.emplace_back(config);
        tasks.emplace_back("task_" + circuit.qpu, RunConfig(config),
                           std::make_unique<DynamicCircuit>(circuit.instructions));
    }
    simulator->config = RunConfig::combine_configs(configs);

    QCEngine engine(simulator, CommModel::from_backends(backends));
    return engine.run(tasks);
}

inline double probability(const JSON& result, const std::string& bitstring)
{
    const auto& counts = result.at("counts");
    std::size_t total = 0;
    for (const auto& [key, value] : counts.items())
        total += value.get<std::size_t>();
    if (total == 0)
        return 0.0;
    return counts.contains(bitstring) ? counts.at(bitstring).get<double>() / total : 0.0;
}

// Tolerance for an estimated probability: `sigmas` standard deviations of a binomial
// proportion over the given number of shots.
inline double tolerance(double p, int shots, double sigmas = 4.5)
{
    return sigmas * std::sqrt(std::max(p * (1.0 - p), 1e-4) / shots);
}

} // End of test namespace
