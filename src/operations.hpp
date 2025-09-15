#pragma once

#include <string>
#include <vector>

#include "comm/client.hpp"

namespace {
const int gate_bit_len(4);
const int qubit_bit_len(6);
const int cbit_bit_len(6);
const int param_sign_bit_len(1);
const int param_int_bit_len(8);
const int param_dec_bit_len(23);
const int precision(100000); // Number_of_digits_of(2^param_dec_bit_len) - 1
const int og_bit_len(gate_bit_len + qubit_bit_len);
const int opg_bit_len(gate_bit_len + qubit_bit_len + param_sign_bit_len + param_int_bit_len + param_dec_bit_len);
const int tg_bit_len(gate_bit_len + 2*qubit_bit_len);
const int tpg_bit_len(gate_bit_len + 2*qubit_bit_len + param_sign_bit_len + param_int_bit_len + param_dec_bit_len);
const int meas_bit_len(gate_bit_len + qubit_bit_len + cbit_bit_len);

struct OneGate // 16 bits (2 bytes)
{ 
    uint8_t gate_name; 
    uint8_t qubit; 

};

struct OnePGate // 64 bits (8 bytes)
{
    uint8_t gate_name; 
    uint8_t qubit;
    uint8_t parameter_sign;
    uint8_t parameter_int;
    uint32_t parameter_dec;

};

struct TwoGate // 24 bits (3 bytes)
{
    uint8_t gate_name;
    uint8_t first_qubit;
    uint8_t second_qubit; 
};

struct TwoPGate // 72 bits! (9 bytes)
{
    uint8_t gate_name; 
    uint8_t first_qubit; 
    uint8_t second_qubit; 
    uint8_t parameter_sign;
    int8_t parameter_int;
    uint32_t parameter_dec;
};


struct Measure // 24 bits (3 bytes)
{
    uint8_t gate_name; 
    uint8_t qubit; 
    uint8_t clbit;
};

} //End of anonymous namespace

namespace cunqa {

class Operations {
public:
    Operations();
    ~Operations() = default;

    void apply_gate(const std::string& gate_name, const std::vector<int>& qubit);
    void apply_parametric_gate(const std::string& gate_name, const std::vector<int>& qubit, const double& param);
    void apply_measure(const std::vector<int>& qubit, const std::vector<int>& clbit);
    void flush();

private:
    void _serialize_onegate_instruction(OneGate& instruction);
    void _serialize_onepgate_instruction(OnePGate& instruction);
    void _serialize_twogate_instruction(TwoGate& instruction);
    void _serialize_twopgate_instruction(TwoPGate& instruction);
    void _serialize_measure_instruction(Measure& instruction);

    std::vector<uint8_t> serialized_ops;
    std::unique_ptr<comm::Client> qclient;

};


Operations::Operations()
{
    
}


void Operations::apply_gate(const std::string& gate_name, const std::vector<int>& qubit)
{

}


void Operations::apply_parametric_gate(const std::string& gate_name, const std::vector<int>& qubit, const double& param)
{

}


void Operations::apply_measure(const std::vector<int>& qubit, const std::vector<int>& clbit)
{


}


void Operations::flush()
{


}


void Operations::_serialize_onegate_instruction(OneGate& instruction)
{
    serialized_ops.push_back(instruction.gate_name);
    serialized_ops.push_back(instruction.qubit);
}


void Operations::_serialize_onepgate_instruction(OnePGate& instruction)
{
    serialized_ops.push_back(instruction.gate_name);
    serialized_ops.push_back(instruction.qubit);
    serialized_ops.push_back(instruction.parameter_sign);
    serialized_ops.push_back(instruction.parameter_int);

    // Append the 4 bytes of the converted value
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&instruction.parameter_dec);
    for (size_t i = 0; i < sizeof(instruction.parameter_dec); i++) {
        serialized_ops.push_back(ptr[i]);
    }
}


void Operations::_serialize_twogate_instruction(TwoGate& instruction)
{
    serialized_ops.push_back(instruction.gate_name);
    serialized_ops.push_back(instruction.first_qubit);
    serialized_ops.push_back(instruction.second_qubit);
}


void Operations::_serialize_twopgate_instruction(TwoPGate& instruction)
{
    serialized_ops.push_back(instruction.gate_name);
    serialized_ops.push_back(instruction.first_qubit);
    serialized_ops.push_back(instruction.second_qubit);
    serialized_ops.push_back(instruction.parameter_sign);
    serialized_ops.push_back(instruction.parameter_int);

    // Append the 4 bytes of the converted value
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&instruction.parameter_dec);
    for (size_t i = 0; i < sizeof(instruction.parameter_dec); i++) {
        serialized_ops.push_back(ptr[i]);
    }
}


void Operations::_serialize_measure_instruction(Measure& instruction)
{
    serialized_ops.push_back(instruction.gate_name);
    serialized_ops.push_back(instruction.qubit);
    serialized_ops.push_back(instruction.clbit);
}


} // End of cunqa namespace