#pragma once
#include <iostream>
#include <bit>
#include <cstdint>
#include <vector>
#include <list>
#include <bitset>
#include <cmath>
#include <fstream> 

#include <nlohmann/json.hpp>
using json = nlohmann::json;


// Secure cast of size
template<typename TO, typename FROM>
TO legacy_size_cast(FROM value)
{
    static_assert(std::is_unsigned_v<FROM> && std::is_unsigned_v<TO>,
                  "Only unsigned types can be cast here!");
    TO result = value;
    return result;
}


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

struct OneGate
{ 
    uint16_t gate_name : gate_bit_len; 
    uint16_t qubit : qubit_bit_len; 
};

struct OnePGate
{
    uint16_t gate_name : gate_bit_len; 
    uint16_t qubit : qubit_bit_len; 
    uint16_t parameter_sign: param_sign_bit_len;
    int16_t parameter_int : param_int_bit_len;
    uint32_t parameter_dec : param_dec_bit_len;

};

struct TwoGate
{
    uint16_t gate_name : gate_bit_len; 
    uint16_t first_qubit : qubit_bit_len; 
    uint16_t second_qubit : qubit_bit_len; 
};

struct TwoPGate
{
    uint16_t gate_name : gate_bit_len; 
    uint16_t first_qubit : qubit_bit_len; 
    uint16_t second_qubit : qubit_bit_len; 
    uint16_t parameter_sign: param_sign_bit_len;
    int16_t parameter_int : param_int_bit_len;
    uint32_t parameter_dec : param_dec_bit_len;
};

struct Measure
{
    uint16_t measure : gate_bit_len; 
    uint16_t qubit : qubit_bit_len; 
    uint16_t cbit : cbit_bit_len;
};


std::vector<std::string> basic_gates = {"measure", "id", "x", "y", "z", "rx", "ry", "rz", "h", "cx", "cy", "cz"};
std::list<std::string> one_gates_no_parameters = {"id", "x", "y", "z", "h"};
std::list<std::string> one_gates_parameters = {"rx", "ry", "rz"};
std::list<std::string> two_gates_no_parameters = {"cx", "cy", "cz"};
std::list<std::string> two_gates_parameters = {};
std::list<std::string> measures = {"measure"};


OneGate onegate_json(json og_json) {

    OneGate onegate;
    onegate.gate_name = std::find(basic_gates.begin(), basic_gates.end(), og_json["name"]) - basic_gates.begin() + 1;
    onegate.qubit = og_json["qubits"][0];

    return onegate;
}

OnePGate onepgate_json(json opg_json) {

    OnePGate onepgate;
    onepgate.gate_name = std::find(basic_gates.begin(), basic_gates.end(), opg_json["name"]) - basic_gates.begin() + 1;
    onepgate.qubit = static_cast<uint16_t>(opg_json["qubits"][0]);
    double p = opg_json["params"][0];
    uint16_t sign;
    if (p <= 0 ){
        sign = 1;
    }
    else {
        sign = 0;
    }
    p = std::abs(p);
    int16_t intPart = static_cast<int16_t>(std::floor(p));
    double decPart = p - intPart; 
    uint32_t decPartInt = static_cast<uint32_t>(decPart * precision);
    onepgate.parameter_sign = sign;
    onepgate.parameter_int = intPart;
    onepgate.parameter_dec = decPartInt;


    return onepgate;

}

TwoGate twogate_json(json tg_json) {

    TwoGate twogate;
    twogate.gate_name = std::find(basic_gates.begin(), basic_gates.end(), tg_json["name"]) - basic_gates.begin() + 1;
    twogate.first_qubit = tg_json["qubits"][0];
    twogate.second_qubit = tg_json["qubits"][1];


    return twogate;
}

TwoPGate twopgate_json(json tgp_json) {

    TwoPGate twopgate;
    twopgate.gate_name = std::find(basic_gates.begin(), basic_gates.end(), tgp_json["name"]) - basic_gates.begin() + 1;
    twopgate.first_qubit = tgp_json["qubits"][0];
    twopgate.second_qubit = tgp_json["qubits"][1];
    double p = tgp_json["params"][0];
    uint16_t sign;
    if (p <= 0 ){
        sign = 1;
    }
    else {
        sign = 0;
    }
    p = std::abs(p);
    int16_t intPart = static_cast<int16_t>(std::floor(p));
    double decPart = p - intPart; 
    uint32_t decPartInt = static_cast<uint32_t>(decPart * precision);
    twopgate.parameter_sign = sign;
    twopgate.parameter_int = intPart;
    twopgate.parameter_dec = decPartInt;

    return twopgate;
}

Measure measure_json(json meas_json) {

    Measure measure;
    measure.measure = 1;
    measure.qubit = meas_json["qubits"][0];
    measure.cbit = meas_json["memory"][0];


    return measure;
}


// The input is a list of jsons in string format, each json corresponding to a gate/measure with all its properties.
std::vector<bool> from_json_to_bin(std::string qc_str){

    json qc_json = json::parse(qc_str);

     //std::ofstream outFile("bin_circuit.txt");
     std::vector<bool> bool_vector;


    //int sum = 0;

    for (int j = 0; j < qc_json.size(); j++){
        if (std::find(one_gates_no_parameters.begin(), one_gates_no_parameters.end(), qc_json[j]["name"]) != one_gates_no_parameters.end()){
            OneGate res = onegate_json(qc_json[j]);

            uint16_t concatenated = (res.gate_name << qubit_bit_len) | res.qubit;
            std::bitset<10> z = concatenated;
            
            for (int i = 0; i < z.size(); i++) {
                bool_vector.push_back(z[z.size() -1 - i]); 
            }
        
            //sum = sum + og_bit_len;
            //outFile << z ;


            
        }
        else if(std::find(one_gates_parameters.begin(), one_gates_parameters.end(), qc_json[j]["name"]) != one_gates_parameters.end()){
            OnePGate res = onepgate_json(qc_json[j]);

            uint64_t concatenated = (static_cast<uint64_t>(res.gate_name) << (opg_bit_len - gate_bit_len)) | (static_cast<uint64_t>(res.qubit) << (opg_bit_len - gate_bit_len - qubit_bit_len)) | (static_cast<uint64_t>(res.parameter_sign) << (opg_bit_len - gate_bit_len - qubit_bit_len - param_sign_bit_len)) | (static_cast<uint64_t>(res.parameter_int) << (opg_bit_len - gate_bit_len - qubit_bit_len - param_sign_bit_len - param_int_bit_len)) | static_cast<uint64_t>(res.parameter_dec);
            std::bitset<opg_bit_len> z = concatenated;
            
            
            for (int i = 0; i < z.size(); i++) {
                bool_vector.push_back(z[z.size() - 1 -i]); 
            }


            //sum = sum + opg_bit_len;
            //outFile << z ;


        }
        else if(std::find(two_gates_no_parameters.begin(), two_gates_no_parameters.end(), qc_json[j]["name"]) != two_gates_no_parameters.end()){
            TwoGate res = twogate_json(qc_json[j]);

            uint16_t concatenated = (res.gate_name << (tg_bit_len - gate_bit_len)) | (res.first_qubit << (tg_bit_len - gate_bit_len - qubit_bit_len)) | res.second_qubit;
            std::bitset<tg_bit_len> z = concatenated;

            for (int i = 0; i < z.size(); i++) {
                bool_vector.push_back(z[z.size() - 1 -i]); 
            }

            //sum = sum + tg_bit_len;
            //outFile << z  ;
            

            
        }
        else if(std::find(two_gates_parameters.begin(), two_gates_parameters.end(), qc_json[j]["name"]) != two_gates_parameters.end()){
            TwoPGate res = twopgate_json(qc_json[j]);

            uint64_t concatenated = (static_cast<uint64_t>(res.gate_name) << (tpg_bit_len - gate_bit_len)) | (static_cast<uint64_t>(res.first_qubit) << (tpg_bit_len - gate_bit_len - qubit_bit_len)) | (static_cast<uint64_t>(res.second_qubit) << (tpg_bit_len - gate_bit_len - 2*qubit_bit_len)) | (static_cast<uint64_t>(res.parameter_int) << (tpg_bit_len - gate_bit_len - 2*qubit_bit_len - param_int_bit_len)) | static_cast<uint64_t>(res.parameter_dec);
            std::bitset<tpg_bit_len> z = concatenated;

            for (int i = 0; i < z.size(); i++) {
            bool_vector.push_back(z[z.size() - 1 -i]); 
            }

            //sum = sum + tpg_bit_len;
            //outFile << z ;

            
        }
        else if(std::find(measures.begin(), measures.end(), qc_json[j]["name"]) != measures.end()){
            Measure res = measure_json(qc_json[j]);

            uint16_t concatenated = (res.measure << (meas_bit_len - gate_bit_len)) | (res.qubit << (meas_bit_len - gate_bit_len - qubit_bit_len)) | res.cbit;
            std::bitset<meas_bit_len> z = concatenated;

            for (int i = 0; i < z.size(); i++) {
            bool_vector.push_back(z[z.size() - 1 -i]);
            }

            //sum = sum + meas_bit_len;
            //outFile << z ;

            
        }
        else {
            break;
        }

    }

    return bool_vector;

}




std::vector<json> from_bin_to_json(std::vector<bool> bool_vector){
    std::vector<json> circ_json;
    json aux_json;
    int index0 = 0;
    int index1 = 0;
    int sign = 0;
    double decimal = 0;
    int step = 0;


    while (step < bool_vector.size()){

        std::vector<bool> aux_bool_vec(bool_vector.begin() + step, bool_vector.begin() + step + gate_bit_len);


        for (size_t i = 0; i < aux_bool_vec.size(); i++) {
            index0 = index0 + aux_bool_vec[i]*std::pow(2, aux_bool_vec.size() - 1 - i);
        }  


        aux_json["name"] = basic_gates[index0 - 1];
        index0 = 0;
        aux_bool_vec.clear();

        step = step + gate_bit_len;


        
        if (std::find(one_gates_no_parameters.begin(), one_gates_no_parameters.end(), aux_json["name"]) != one_gates_no_parameters.end()){
            
            std::vector<bool> aux_bool_vec(bool_vector.begin() + step, bool_vector.begin() + step + (og_bit_len - gate_bit_len));

            for (size_t i = 0; i < qubit_bit_len; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2,(qubit_bit_len-1) - i);
            }

            aux_json["qubits"] = {index0};
            circ_json.push_back(aux_json);

            aux_json = {};
            aux_bool_vec.clear();
            index0 = 0;

            step = step + qubit_bit_len;
            

        }

        else if (std::find(one_gates_parameters.begin(), one_gates_parameters.end(), aux_json["name"]) != one_gates_parameters.end()){
            std::vector<bool> aux_bool_vec(bool_vector.begin() + step, bool_vector.begin() + step + (opg_bit_len - gate_bit_len));

            for (size_t i = 0; i < qubit_bit_len; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2,(qubit_bit_len - 1) - i);
                }

            aux_json["qubits"] = {index0};
            index0 = 0;

            if (aux_bool_vec[qubit_bit_len] == 1){
                sign = 1;
            }
            

            for (size_t i = (qubit_bit_len + param_sign_bit_len); i < (qubit_bit_len + param_sign_bit_len + param_int_bit_len); i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2,(qubit_bit_len + param_sign_bit_len + param_int_bit_len  - 1) - i);
                }
            

            for (size_t i = (qubit_bit_len + param_sign_bit_len + param_int_bit_len); i < (qubit_bit_len + param_sign_bit_len + param_int_bit_len + param_dec_bit_len); i++) {
                index1 = index1 + static_cast<double>(aux_bool_vec[i])*std::pow(2,(qubit_bit_len + param_sign_bit_len + param_int_bit_len + param_dec_bit_len - 1) - i);
                }
            
            
            

            double decimal = index1 / pow(10, std::to_string(index1).length());

            

            if (sign == 1){
                index0 = -index0;
                aux_json["params"] = {static_cast<double>(index0) - decimal};
            }
            else {
                aux_json["params"] = {static_cast<double>(index0) + decimal};
            }

            circ_json.push_back(aux_json);
            

            aux_json = {};
            aux_bool_vec.clear();          
            index0 = 0;
            index1 = 0;
            sign = 0;
            decimal = 0;

            step = step + (opg_bit_len - gate_bit_len);
            
       
        }
        
        else if (std::find(two_gates_no_parameters.begin(), two_gates_no_parameters.end(), aux_json["name"]) != two_gates_no_parameters.end()){
            std::vector<bool> aux_bool_vec(bool_vector.begin() + step, bool_vector.begin() + step + (tg_bit_len - gate_bit_len));

            for (size_t i = 0; i < qubit_bit_len; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2, (qubit_bit_len - 1) - i);
                }

            for (size_t i = qubit_bit_len; i < 2*qubit_bit_len; i++) {
                index1 = index1 + aux_bool_vec[i]*std::pow(2, (2*qubit_bit_len - 1) - i);
                }

            aux_json["qubits"] = {index0, index1};
            circ_json.push_back(aux_json);

            aux_json = {};
            aux_bool_vec.clear();
            index0 = 0;
            index1 = 0;

            step = step + (tg_bit_len - gate_bit_len);
            
        }
        
        else if (std::find(two_gates_parameters.begin(), two_gates_parameters.end(), aux_json["name"]) != two_gates_parameters.end()){
            std::vector<bool> aux_bool_vec(bool_vector.begin() + step, bool_vector.begin() + step + (tpg_bit_len - gate_bit_len));

            for (size_t i = 0; i < qubit_bit_len; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2, (qubit_bit_len - 1) - i);
                }

            for (size_t i = qubit_bit_len; i < 2*qubit_bit_len; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2, (2*qubit_bit_len - 1) - i);
                }

            aux_json["qubits"] = {index0, index1};
            index0 = 0;
            index1 = 0;

            if (aux_bool_vec[2*qubit_bit_len] == 1){
                sign = 1;
            }

            for (size_t i = (2*qubit_bit_len + param_sign_bit_len); i < (2*qubit_bit_len + param_sign_bit_len + param_int_bit_len); i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2, (2*qubit_bit_len + param_sign_bit_len + param_int_bit_len - 1) - i);
                }


            for (size_t i = (2*qubit_bit_len + param_sign_bit_len + param_int_bit_len); i < (2*qubit_bit_len + param_sign_bit_len + param_int_bit_len + param_dec_bit_len); i++) {
                index1 = index1 + aux_bool_vec[i]*std::pow(2, (2*qubit_bit_len + param_sign_bit_len + param_int_bit_len + param_dec_bit_len - 1) - i);
                }

            double decimal = index1 / pow(10, std::to_string(index1).length());

            if (sign == 1){
                index0 = -index0;
                aux_json["params"] = {static_cast<double>(index0) - decimal};
            }
            else {
                aux_json["params"] = {static_cast<double>(index0) + decimal};
            }

            circ_json.push_back(aux_json);

            aux_json = {};
            aux_bool_vec.clear();
            index0 = 0;
            index1 = 0;
            sign = 0;

            step = step + (tpg_bit_len - gate_bit_len);
            
        }
        
        else if (std::find(measures.begin(), measures.end(), aux_json["name"]) != measures.end()){

            std::vector<bool> aux_bool_vec(bool_vector.begin() + step, bool_vector.begin() + step + (meas_bit_len - gate_bit_len));
            for (size_t i = 0; i < qubit_bit_len; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2, (qubit_bit_len - 1) - i);
                }
            
            aux_json["qubits"] = {index0};
            index0 = 0;

            for (size_t i = qubit_bit_len; i < (qubit_bit_len + cbit_bit_len); i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2, (qubit_bit_len + cbit_bit_len - 1) - i);
            }

            aux_json["memory"] = {index0};
            circ_json.push_back(aux_json);
            
            aux_json = {};
            aux_bool_vec.clear();
            index0 = 0;
            index1 = 0;

            step = step + (meas_bit_len - gate_bit_len);
            
        }

        else {
            std::cout << "No valid gate name \n"; 
            break;
        }
    
    } // End while

    return circ_json;

}














