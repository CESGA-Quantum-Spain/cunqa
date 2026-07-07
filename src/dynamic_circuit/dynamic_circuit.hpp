#pragma once

#include <vector>

#include "circuit.hpp"

#include "parameters.hpp"
#include "instruction.hpp"

#include "utils/json.hpp"

namespace cunqa {

struct DynamicCircuit : public Circuit{
    std::vector<Instruction> instructions;
    Parameters params;

    explicit DynamicCircuit(const JSON& instructions_json);

    void update_params(const std::vector<double>& new_params) override
    {
        params.update_params(new_params);
    }

    JSON to_json() const {
        JSON instructions_json = JSON::array();
        
        for (const auto& instruction : instructions) {
            JSON instr_json;
            instr_json["type"] = static_cast<int>(instruction.type);
            
            // Use a visitor to serialize the payload based on its type
            std::visit([&instr_json](const auto& payload) {
                using T = std::decay_t<decltype(payload)>;
                
                if constexpr (std::is_same_v<T, std::monostate>) {
                    instr_json["payload"] = nullptr;
                }
                else if constexpr (std::is_same_v<T, OneQubitNoParam>) {
                    instr_json["payload"]["qubit"] = payload.qubit;
                }
                else if constexpr (std::is_same_v<T, OneQubitOneParam>) {
                    instr_json["payload"]["qubit"] = payload.qubit;
                    instr_json["payload"]["param"] = *payload.param;
                }
                else if constexpr (std::is_same_v<T, OneQubitTwoParam>) {
                    instr_json["payload"]["qubit"] = payload.qubit;
                    instr_json["payload"]["params"] = {*payload.params[0], *payload.params[1]};
                }
                else if constexpr (std::is_same_v<T, OneQubitThreeParam>) {
                    instr_json["payload"]["qubit"] = payload.qubit;
                    instr_json["payload"]["params"] = {*payload.params[0], *payload.params[1], *payload.params[2]};
                }
                else if constexpr (std::is_same_v<T, OneQubitFourParam>) {
                    instr_json["payload"]["qubit"] = payload.qubit;
                    instr_json["payload"]["params"] = {*payload.params[0], *payload.params[1], *payload.params[2], *payload.params[3]};
                }
                else if constexpr (std::is_same_v<T, TwoQubitNoParam>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                }
                else if constexpr (std::is_same_v<T, TwoQubitOneParam>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    instr_json["payload"]["param"] = *payload.param;
                }
                else if constexpr (std::is_same_v<T, TwoQubitTwoParam>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    instr_json["payload"]["params"] = {*payload.params[0], *payload.params[1]};
                }
                else if constexpr (std::is_same_v<T, TwoQubitThreeParam>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    instr_json["payload"]["params"] = {*payload.params[0], *payload.params[1], *payload.params[2]};
                }
                else if constexpr (std::is_same_v<T, TwoQubitFourParam>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    instr_json["payload"]["params"] = {*payload.params[0], *payload.params[1], *payload.params[2], *payload.params[3]};
                }
                else if constexpr (std::is_same_v<T, ThreeQubitNoParam>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                }
                else if constexpr (std::is_same_v<T, MultiNoParam>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                }
                else if constexpr (std::is_same_v<T, MultiParam>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    JSON params_array = JSON::array();
                    for (const auto& p : payload.params) {
                        params_array.push_back(*p);
                    }
                    instr_json["payload"]["params"] = params_array;
                }
                else if constexpr (std::is_same_v<T, PauliNoParam>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    instr_json["payload"]["paulistr"] = payload.paulistr;
                }
                else if constexpr (std::is_same_v<T, PauliParam>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    instr_json["payload"]["param"] = *payload.param;
                    instr_json["payload"]["paulistr"] = payload.paulistr;
                }
                else if constexpr (std::is_same_v<T, MultiPauli>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    instr_json["payload"]["param"] = *payload.param;
                    instr_json["payload"]["pauli_id_list"] = payload.pauli_id_list;
                }
                else if constexpr (std::is_same_v<T, NumControlsNoParam>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    instr_json["payload"]["num_controls"] = payload.num_controls;
                }
                else if constexpr (std::is_same_v<T, NumControlsParam>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    instr_json["payload"]["param"] = *payload.param;
                    instr_json["payload"]["num_controls"] = payload.num_controls;
                }
                else if constexpr (std::is_same_v<T, FusedSwap>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    instr_json["payload"]["block_size"] = payload.block_size;
                }
                else if constexpr (std::is_same_v<T, MatrixGate>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    JSON matrix_json = JSON::array();
                    for (const auto& row : payload.matrix) {
                        JSON row_json = JSON::array();
                        for (const auto& elem : row) {
                            row_json.push_back({{"real", elem[0]}, {"imag", elem[1]}});
                        }
                        matrix_json.push_back(row_json);
                    }
                    instr_json["payload"]["matrix"] = matrix_json;
                }
                else if constexpr (std::is_same_v<T, DiagonalMatrixGate>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    JSON matrix_json = JSON::array();
                    for (const auto& elem : payload.matrix) {
                        matrix_json.push_back({{"real", elem[0]}, {"imag", elem[1]}});
                    }
                    instr_json["payload"]["matrix"] = matrix_json;
                }
                else if constexpr (std::is_same_v<T, OneQubitNoise>) {
                    instr_json["payload"]["qubit"] = payload.qubit;
                    instr_json["payload"]["params"] = *payload.params;
                    instr_json["payload"]["seed"] = payload.seed;
                }
                else if constexpr (std::is_same_v<T, TwoQubitNoise>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    instr_json["payload"]["params"] = *payload.params;
                    instr_json["payload"]["seed"] = payload.seed;
                }
                else if constexpr (std::is_same_v<T, RandomUnitary>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                    instr_json["payload"]["seed"] = payload.seed;
                }
                else if constexpr (std::is_same_v<T, Measure>) {
                    instr_json["payload"]["qubit"] = payload.qubit;
                    instr_json["payload"]["clbit"] = payload.clbit;
                    instr_json["payload"]["save"] = payload.save;
                }
                else if constexpr (std::is_same_v<T, Reset>) {
                    instr_json["payload"]["qubits"] = payload.qubits;
                }
                else if constexpr (std::is_same_v<T, Copy>) {
                    instr_json["payload"]["l_clbits"] = payload.l_clbits;
                    instr_json["payload"]["r_clbits"] = payload.r_clbits;
                }
                else if constexpr (std::is_same_v<T, ClassicalComm>) {
                    instr_json["payload"]["clbits"] = payload.clbits;
                    instr_json["payload"]["qpus"] = payload.qpus;
                }
                else if constexpr (std::is_same_v<T, GenEnt>) {
                    instr_json["payload"]["qubit"] = payload.qubit;
                    instr_json["payload"]["qpus"] = payload.qpus;
                    instr_json["payload"]["tag"] = payload.tag;
                }
                else if constexpr (std::is_same_v<T, ClassicalIf>) {
                    instr_json["payload"]["clbits"] = payload.clbits;
                }
            }, instruction.payload);
            
            instructions_json.push_back(instr_json);
        }
        
        return instructions_json;
    }

};

} // End of cunqa namespace
