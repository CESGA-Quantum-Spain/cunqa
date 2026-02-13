#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <regex>

#include "utils/json.hpp"

namespace
{
using namespace cunqa;
using AddersFn = void(*)(std::string_view, JSON&, JSON&);
constexpr double PI = 3.141592653589793;
const std::string PI_STR = "pi";

void found_and_replace_pi(std::string& s) {
    const std::string pi_val = std::to_string(PI);

    size_t pos = 0;
    while ((pos = s.find(PI_STR, pos)) != std::string::npos) {
        s.replace(pos, PI_STR.length(), pi_val);
        pos += pi_val.length();
    }
}

double eval_simple_expr(const std::string& expr) {
    std::stringstream ss(expr);
    double result;
    ss >> result;

    char op;
    double value;

    while (ss >> op >> value) {
        if (op == '*') result *= value;
        else if (op == '/') result /= value;
    }
    return result;
}

std::string_view get_inst_name(std::string_view line) 
{
    auto end = line.find_first_of(" (");
    return line.substr(0, end);
}


void add_qreg_instruction(std::string_view sv_inst, JSON& circuit_json, JSON& aux_json)
{
    auto p0 = sv_inst.find_first_not_of(" \t\r");
    auto p1 = sv_inst.find(' ', p0);
    auto p2 = sv_inst.find_first_of(" \t\r", p1);
    auto p3 = sv_inst.find_first_not_of(" \t\r", p2);
    auto p4 = sv_inst.find('[', p3);
    auto p5 = sv_inst.find(']', p4);

    int num_qubits = circuit_json["num_qubits"];
    int num_qregs = std::stoi(std::string(sv_inst.substr(p4+1, p5-p4-1)));

    std::vector<int> qreg_indexes(num_qregs);
    std::iota(qreg_indexes.begin(), qreg_indexes.end(), num_qubits);
    circuit_json["quantum_registers"][std::string(sv_inst.substr(p3, p4-p3))] = qreg_indexes;

    circuit_json["num_qubits"] = circuit_json.value("num_qubits", 0) + num_qregs;

}

void add_creg_instruction(std::string_view sv_inst, JSON& circuit_json, JSON& aux_json)
{
    auto p0 = sv_inst.find_first_not_of(" \t\r");
    auto p1 = sv_inst.find(' ', p0);
    auto p2 = sv_inst.find_first_of(" \t\r", p1);
    auto p3 = sv_inst.find_first_not_of(" \t\r", p2);
    auto p4 = sv_inst.find('[', p3);
    auto p5 = sv_inst.find(']', p4);

    int num_clbits = circuit_json["num_clbits"];
    int num_cregs = std::stoi(std::string(sv_inst.substr(p4+1, p5-p4-1)));

    std::vector<int> creg_indexes(num_cregs);
    std::iota(creg_indexes.begin(), creg_indexes.end(), num_clbits);
    circuit_json["classical_registers"][std::string(sv_inst.substr(p3, p4-p3))] = creg_indexes;

    circuit_json["num_clbits"] = circuit_json.value("num_clbits", 0) + num_cregs;
}

void add_meas_instruction(std::string_view sv_inst, JSON& circuit_json, JSON& aux_json)
{
    auto p0 = sv_inst.find_first_not_of(" \t\n\r");
    auto p1 = sv_inst.find(' ', p0);
    auto p2 = sv_inst.find_first_not_of(" \t\n\r", p1);
    auto p3 = sv_inst.find('[', p2);
    if (p3 == std::string_view::npos) {
        p3 = sv_inst.find_first_not_of(" \t\r", p2);
        auto p4 = sv_inst.find(' ', p3);
        auto p5 = sv_inst.find('>', p4);
        auto p6 = sv_inst.find_first_not_of(" \t\r", p5+1);
        auto p7 = sv_inst.find(';', p6);
        
        int counter = 0;
        for (auto& qindex : circuit_json["quantum_registers"][std::string(sv_inst.substr(p3, p4-p3))]) {
            aux_json["name"] = "measure";
            aux_json["qubits"] = {qindex};
            aux_json["clbits"] = {circuit_json["classical_registers"]
                                        [std::string(sv_inst.substr(p6, p7-p6))][counter]};

            circuit_json["instructions"].push_back(aux_json);
            counter++;
            
            aux_json.clear();
        }

    } else {
        auto p4 = sv_inst.find(']', p3);
        auto p5 = sv_inst.find('>', p4);
        auto p6 = sv_inst.find_first_not_of(" \t\r", p5+1);
        auto p7 = sv_inst.find('[', p6);
        auto p8 = sv_inst.find(']', p7);

        aux_json["qubits"] = {
            circuit_json["quantum_registers"][std::string(sv_inst.substr(p2, p3-p2))]
            [
                std::stoi(std::string(sv_inst.substr(p3+1, p4-p3-1)))
            ]
        };

        aux_json["clbits"] = {
            circuit_json["classical_registers"][std::string(sv_inst.substr(p6, p7-p6))]
            [
                std::stoi(std::string(sv_inst.substr(p7+1, p8-p7-1)))
            ]
        };

        circuit_json["instructions"].push_back(aux_json);
    }

    
    aux_json.clear();
}

void add_1q0p_instruction(std::string_view sv_inst, JSON& circuit_json, JSON& aux_json) 
{
    auto p0 = sv_inst.find_first_not_of(" \t\r");
    auto p1 = sv_inst.find(' ', p0);
    auto p2 = sv_inst.find_first_not_of(" \t\r", p1);
    auto p3 = sv_inst.find('[', p2);
    auto p4 = sv_inst.find(']', p3);

    aux_json["name"] = std::string(sv_inst.substr(p0, p1-p0));
    aux_json["qubits"] = 
        {circuit_json["quantum_registers"][std::string(sv_inst.substr(p2, p3-p2))]
        [
            std::stoi(std::string(sv_inst.substr(p3+1, p4-p3-1)))
        ]};

    circuit_json["instructions"].push_back(aux_json);

    aux_json.clear();
}

void add_1q1p_instruction(std::string_view sv_inst, JSON& circuit_json, JSON& aux_json) 
{
    auto p0 = sv_inst.find_first_not_of(" \t\r");
    auto p1 = sv_inst.find('(', p0);
    auto p2 = sv_inst.find(")", p1);
    auto p3 = sv_inst.find_first_not_of(" \t\r", p2+1);
    auto p4 = sv_inst.find('[', p3);
    auto p5 = sv_inst.find(']', p4);

    aux_json["name"] = std::string(sv_inst.substr(p0, p1-p0));
    aux_json["qubits"] = 
        {circuit_json["quantum_registers"][std::string(sv_inst.substr(p3, p4-p3))]
        [
            std::stoi(std::string(sv_inst.substr(p4+1, p5-p4-1)))
        ]};
    
    std::string param_str = std::string(sv_inst.substr(p1+1, p2-p1-1));
    found_and_replace_pi(param_str);
    double param = eval_simple_expr(param_str);

    aux_json["params"] = {param};
    circuit_json["instructions"].push_back(aux_json);

    aux_json.clear();
}

void add_1q2p_instruction(std::string_view sv_inst, JSON& circuit_json, JSON& aux_json) 
{
    auto p0 = sv_inst.find_first_not_of(" \t\r");
    auto p1 = sv_inst.find('(', p0);
    auto p2 = sv_inst.find(",", p1);
    auto p3 = sv_inst.find(")", p2);
    auto p4 = sv_inst.find_first_not_of(" \t\r", p3+1);
    auto p5 = sv_inst.find('[', p4);
    auto p6 = sv_inst.find(']', p5);

    aux_json["name"] = std::string(sv_inst.substr(p0, p1-p0));
    aux_json["qubits"] = 
        {circuit_json["quantum_registers"][std::string(sv_inst.substr(p4, p5-p4))]
        [
            std::stoi(std::string(sv_inst.substr(p5+1, p6-p5-1)))
        ]};

    std::vector<std::string> params_str = {std::string(sv_inst.substr(p1+1, p2-p1-1)), 
                                        std::string(sv_inst.substr(p2+1, p3-p2-1))};
    std::vector<double> params(2);
    int counter = 0;
    for (auto& param_str : params_str) {
        found_and_replace_pi(param_str);
        params[counter] = eval_simple_expr(param_str);
        counter++;
    }
    aux_json["params"] = params;

    circuit_json["instructions"].push_back(aux_json);

    aux_json.clear();
}

void add_1q3p_instruction(std::string_view sv_inst, JSON& circuit_json, JSON& aux_json) 
{
    auto p0 = sv_inst.find_first_not_of(" \t\r");
    auto p1 = sv_inst.find('(', p0);
    auto p2 = sv_inst.find(",", p1);
    auto p3 = sv_inst.find(",", p2);
    auto p4 = sv_inst.find(")", p3);
    auto p5 = sv_inst.find_first_not_of(" \t\r", p4+1);
    auto p6 = sv_inst.find('[', p5);
    auto p7 = sv_inst.find(']', p6);

    aux_json["name"] = std::string(sv_inst.substr(p0, p1-p0));
    aux_json["qubits"] = 
        {circuit_json["quantum_registers"][std::string(sv_inst.substr(p5, p6-p5))]
        [
            std::stoi(std::string(sv_inst.substr(p6+1, p7-p6-1)))
        ]};

    std::vector<std::string> params_str = {std::string(sv_inst.substr(p1+1, p2-p1-1)), 
                                        std::string(sv_inst.substr(p2+1, p3-p2-1)),
                                        std::string(sv_inst.substr(p3+1, p4-p3-1))};
    std::vector<double> params(3);
    int counter = 0;
    for (auto& param_str : params_str) {
        found_and_replace_pi(param_str);
        params[counter] = eval_simple_expr(param_str);
        counter++;
    }
    aux_json["params"] = params;

    circuit_json["instructions"].push_back(aux_json);

    aux_json.clear();

}

void add_2q0p_instruction(std::string_view sv_inst, JSON& circuit_json, JSON& aux_json) 
{
    auto p0 = sv_inst.find_first_not_of(" \t\r");
    auto p1 = sv_inst.find(' ', p0);
    auto p2 = sv_inst.find_first_not_of(" \t\r", p1);
    auto p3 = sv_inst.find('[', p2);
    auto p4 = sv_inst.find(']', p3);
    auto p5 = sv_inst.find(',', p4);
    auto p6 = sv_inst.find_first_not_of(" \t\r", p5+1);
    auto p7 = sv_inst.find('[', p6);
    auto p8 = sv_inst.find(']', p7);


    aux_json["name"] = std::string(sv_inst.substr(p0, p1-p0));
    aux_json["qubits"] = 
        {circuit_json["quantum_registers"][std::string(sv_inst.substr(p2, p3-p2))]
        [
            std::stoi(std::string(sv_inst.substr(p3+1, p4-p3-1)))
        ],
        circuit_json["quantum_registers"][std::string(sv_inst.substr(p6, p7-p6))]
        [
            std::stoi(std::string(sv_inst.substr(p7+1, p8-p7-1)))
        ]};

    circuit_json["instructions"].push_back(aux_json);

    aux_json.clear();
}

void add_2q1p_instruction(std::string_view sv_inst, JSON& circuit_json, JSON& aux_json) 
{
    auto p0 = sv_inst.find_first_not_of(" \t\r");
    auto p1 = sv_inst.find('(', p0);
    auto p2 = sv_inst.find(")", p1);
    auto p3 = sv_inst.find_first_not_of(" \t\r", p2+1);
    auto p4 = sv_inst.find('[', p3);
    auto p5 = sv_inst.find(']', p4);
    auto p6 = sv_inst.find(',', p5);
    auto p7 = sv_inst.find_first_not_of(" \t\r", p6+1);
    auto p8 = sv_inst.find('[', p7);
    auto p9 = sv_inst.find(']', p8);


    aux_json["name"] = std::string(sv_inst.substr(p0, p1-p0));
    aux_json["qubits"] = 
        {circuit_json["quantum_registers"][std::string(sv_inst.substr(p3, p4-p3))]
        [
            std::stoi(std::string(sv_inst.substr(p4+1, p5-p4-1)))
        ],
        circuit_json["quantum_registers"][std::string(sv_inst.substr(p7, p8-p7))]
        [
            std::stoi(std::string(sv_inst.substr(p8+1, p9-p8-1)))
        ]};

    std::string param_str = std::string(sv_inst.substr(p1+1, p2-p1-1));
    found_and_replace_pi(param_str);
    double param = eval_simple_expr(param_str);
    aux_json["params"] = {param};

    circuit_json["instructions"].push_back(aux_json);

    aux_json.clear();

}

void add_2q2p_instruction(std::string_view sv_inst, JSON& circuit_json, JSON& aux_json) 
{
    auto p0 = sv_inst.find_first_not_of(" \t\r");
    auto p1 = sv_inst.find('(', p0);
    auto p2 = sv_inst.find(",", p1);
    auto p3 = sv_inst.find(")", p2);
    auto p4 = sv_inst.find_first_not_of(" \t\r", p3+1);
    auto p5 = sv_inst.find('[', p4);
    auto p6 = sv_inst.find(']', p5);
    auto p7 = sv_inst.find(',', p6);
    auto p8 = sv_inst.find_first_not_of(" \t\r", p7+1);
    auto p9 = sv_inst.find('[', p8);
    auto p10 = sv_inst.find(']', p9);


    aux_json["name"] = std::string(sv_inst.substr(p0, p1-p0));
    aux_json["qubits"] = 
        {circuit_json["quantum_registers"][std::string(sv_inst.substr(p4, p5-p4))]
        [
            std::stoi(std::string(sv_inst.substr(p5+1, p6-p5-1)))
        ],
        circuit_json["quantum_registers"][std::string(sv_inst.substr(p8, p9-p8))]
        [
            std::stoi(std::string(sv_inst.substr(p9+1, p10-p9-1)))
        ]};
        
    std::vector<std::string> params_str = {std::string(sv_inst.substr(p1+1, p2-p1-1)), 
                                        std::string(sv_inst.substr(p2+1, p3-p2-1))};
    std::vector<double> params(2);
    int counter = 0;
    for (auto& param_str : params_str) {
        found_and_replace_pi(param_str);
        params[counter] = eval_simple_expr(param_str);
        counter++;
    }
    aux_json["params"] = params;

    circuit_json["instructions"].push_back(aux_json);

    aux_json.clear();
}

void add_2q3p_instruction(std::string_view sv_inst, JSON& circuit_json, JSON& aux_json) 
{
    auto p0 = sv_inst.find_first_not_of(" \t\r");
    auto p1 = sv_inst.find('(', p0);
    auto p2 = sv_inst.find(",", p1);
    auto p3 = sv_inst.find(",", p2);
    auto p4 = sv_inst.find(")", p3);
    auto p5 = sv_inst.find_first_not_of(" \t\r", p4+1);
    auto p6 = sv_inst.find('[', p5);
    auto p7 = sv_inst.find(']', p6);
    auto p8 = sv_inst.find(',', p7);
    auto p9 = sv_inst.find_first_not_of(" \t\r", p8+1);
    auto p10 = sv_inst.find('[', p9);
    auto p11 = sv_inst.find(']', p10);


    aux_json["name"] = std::string(sv_inst.substr(p0, p1-p0));
    aux_json["qubits"] = 
        {circuit_json["quantum_registers"][std::string(sv_inst.substr(p5, p6-p5))]
        [
            std::stoi(std::string(sv_inst.substr(p6+1, p7-p6-1)))
        ],
        circuit_json["quantum_registers"][std::string(sv_inst.substr(p9, p10-p9))]
        [
            std::stoi(std::string(sv_inst.substr(p10+1, p11-p10-1)))
        ]};
        
    std::vector<std::string> params_str = {std::string(sv_inst.substr(p1+1, p2-p1-1)), 
                                        std::string(sv_inst.substr(p2+1, p3-p2-1)),
                                        std::string(sv_inst.substr(p3+1, p4-p3-1))};
    std::vector<double> params(3);
    int counter = 0;
    for (auto& param_str : params_str) {
        found_and_replace_pi(param_str);
        params[counter] = eval_simple_expr(param_str);
        counter++;
    }
    aux_json["params"] = params;

    circuit_json["instructions"].push_back(aux_json);

    aux_json.clear();
}

void add_3q0p_instruction(std::string_view sv_inst, JSON& circuit_json, JSON& aux_json) 
{
    auto p0 = sv_inst.find_first_not_of(" \t\r");
    auto p1 = sv_inst.find(' ', p0);
    auto p2 = sv_inst.find_first_not_of(" \t\r", p1);
    auto p3 = sv_inst.find('[', p2);
    auto p4 = sv_inst.find(']', p3);
    auto p5 = sv_inst.find(',', p4);
    auto p6 = sv_inst.find_first_not_of(" \t\r", p5+1);
    auto p7 = sv_inst.find('[', p6);
    auto p8 = sv_inst.find(']', p7);
    auto p9 = sv_inst.find(',', p8);
    auto p10 = sv_inst.find_first_not_of(" \t\r", p9+1);
    auto p11 = sv_inst.find('[', p10);
    auto p12 = sv_inst.find(']', p11);


    aux_json["name"] = std::string(sv_inst.substr(p0, p1-p0));
    aux_json["qubits"] = 
        {circuit_json["quantum_registers"][std::string(sv_inst.substr(p2, p3-p2))]
        [
            std::stoi(std::string(sv_inst.substr(p3+1, p4-p3-1)))
        ],
        circuit_json["quantum_registers"][std::string(sv_inst.substr(p6, p7-p6))]
        [
            std::stoi(std::string(sv_inst.substr(p7+1, p8-p7-1)))
        ],
        circuit_json["quantum_registers"][std::string(sv_inst.substr(p10, p11-p10))]
        [
            std::stoi(std::string(sv_inst.substr(p11+1, p12-p11-1)))
        ]};

    circuit_json["instructions"].push_back(aux_json);

    aux_json.clear();
}



std::unordered_map<std::string_view, AddersFn> add_instruction {
    {"qreg", add_qreg_instruction},
    {"creg", add_creg_instruction},

    {"measure", add_meas_instruction},

    {"x", add_1q0p_instruction},
    {"y", add_1q0p_instruction},
    {"z", add_1q0p_instruction},
    {"h", add_1q0p_instruction},
    {"s", add_1q0p_instruction},
    {"sdg", add_1q0p_instruction},
    {"sx", add_1q0p_instruction},
    {"sxdg", add_1q0p_instruction},
    {"sy", add_1q0p_instruction},
    {"sydg", add_1q0p_instruction},
    {"sz", add_1q0p_instruction},
    {"szdg", add_1q0p_instruction},
    {"t", add_1q0p_instruction},
    {"tdg", add_1q0p_instruction},
    {"p0", add_1q0p_instruction},
    {"p1", add_1q0p_instruction},

    {"u1", add_1q1p_instruction},
    {"p", add_1q1p_instruction},
    {"rx", add_1q1p_instruction},
    {"ry", add_1q1p_instruction},
    {"rz", add_1q1p_instruction},

    {"u2", add_1q2p_instruction},
    {"r", add_1q2p_instruction},

    {"u3", add_1q3p_instruction},
    {"u", add_1q2p_instruction},

    {"ecr", add_2q0p_instruction},
    {"swap", add_2q0p_instruction},
    {"cx", add_2q0p_instruction},
    {"cy", add_2q0p_instruction},
    {"cz", add_2q0p_instruction},
    {"csx", add_2q0p_instruction},
    {"csy", add_2q0p_instruction},
    {"csz", add_2q0p_instruction},
    {"ct", add_2q0p_instruction},

    {"cp", add_2q1p_instruction},
    {"cu1", add_2q1p_instruction},
    {"crx", add_2q1p_instruction},
    {"cry", add_2q1p_instruction},
    {"crz", add_2q1p_instruction},
    {"rxx", add_2q1p_instruction},
    {"ryy", add_2q1p_instruction},
    {"rzz", add_2q1p_instruction},
    {"rzx", add_2q1p_instruction},

    {"cu2", add_2q2p_instruction},
    {"cr", add_2q2p_instruction},

    {"cu3", add_2q3p_instruction},
    {"cu", add_2q3p_instruction},

    {"cecr", add_3q0p_instruction},
    {"cswap", add_3q0p_instruction},
    {"ccx", add_3q0p_instruction},
    {"ccy", add_3q0p_instruction},
    {"ccyz", add_3q0p_instruction},

};

} // End namespace

JSON qasm2_to_json(const std::string& circuit_qasm) {

    JSON circuit_json = 
    {
        {"instructions", std::vector<JSON>()},
        {"num_qubits", 0},
        {"num_clbits", 0},
        {"quantum_registers", JSON()},
        {"classical_registers", JSON()}
    };
    JSON aux_json;

    size_t start = 0;
    size_t end;
    std::string inst;
    std::string_view sv_inst;
    std::string_view sv_inst_name;
    while ((end = circuit_qasm.find('\n', start)) != std::string::npos) 
    {
        inst = circuit_qasm.substr(start, end - start);
        sv_inst = inst;
        sv_inst_name = get_inst_name(sv_inst);
        auto it = add_instruction.find(sv_inst_name);
        if (it != add_instruction.end()) {
            it->second(sv_inst, circuit_json, aux_json);
        }
        start = end + 1;
    }

    return circuit_json;
}