#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <numeric>

#include "quantum_task/instruction_type.hpp"
#include "sim/simulator.hpp"

namespace cunqa {

using EntWait = std::unordered_map<std::string, std::vector<std::size_t>>;

struct QuantumCommManager {
    std::unordered_map<std::size_t, bool> is_pending;
    EntWait ent_wait;

    QuantumCommManager(const std::vector<std::vector<std::size_t>>& communication_qubits)
    {
        is_pending.reserve(communication_qubits.size());
        for (const auto& group: communication_qubits) {
            for (const auto& comm_qubit: group)
                is_pending[comm_qubit] = false;
        }
    }

    void get_ghz(std::shared_ptr<cunqa::sim::Simulator> simulator, const cunqa::GenEnt& payload)
    {
        auto& wait = ent_wait[payload.tag];
        wait.push_back(payload.qubit);
        if (wait.size() == payload.qpus.size()) {
            simulator->apply_gate(
                cunqa::InstructionType::H,
                cunqa::OneQubitNoParam{wait[0]}
            );

            for (size_t i = 1; i < wait.size(); ++i) {
                simulator->apply_gate(
                    cunqa::InstructionType::CX,
                    cunqa::TwoQubitNoParam{{wait[i - 1], wait[i]}}
                );
            }
            for (const auto& qubit : wait)
                is_pending[qubit] = false;
        } else
            is_pending[payload.qubit] = true;
    }
};    

}