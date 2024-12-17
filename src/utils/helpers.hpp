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



struct OneGate
{ 
    uint16_t gate_name : 4; 
    uint16_t qubit : 6; 
};

struct OnePGate
{
    uint16_t gate_name : 4; 
    uint16_t qubit : 6; 
    int16_t parameter_int : 9;
    uint32_t parameter_dec : 23;

};

struct TwoGate
{
    uint16_t gate_name : 4; 
    uint16_t first_qubit : 6; 
    uint16_t second_qubit : 6; 
};

struct TwoPGate
{
    uint16_t gate_name : 4; 
    uint16_t first_qubit : 6; 
    uint16_t second_qubit : 6; 
    int16_t parameter_int : 9;
    uint32_t parameter_dec : 23;
};

struct Measure
{
    uint16_t measure : 4; 
    uint16_t qubit : 6; 
    uint16_t cbit : 6;
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
    onepgate.qubit = opg_json["qubits"][0];
    double p = opg_json["params"][0];
    int intPart = static_cast<int>(std::floor(p));
    double decPart = p - intPart; 
    int decPartInt = static_cast<int>(std::round(decPart * 10000000));
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
    int intPart = static_cast<int>(std::floor(p));
    double decPart = p - intPart; 
    int decPartInt = static_cast<int>(std::round(decPart * 10000000));
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

std::vector<bool> json_to_bin(json qc_json){

     //std::ofstream outFile("bin_circuit.txt");
     std::vector<bool> bool_vector;


    //int sum = 0;

    for (int j = 0; j < qc_json.size(); j++){
        if (std::find(one_gates_no_parameters.begin(), one_gates_no_parameters.end(), qc_json[j]["name"]) != one_gates_no_parameters.end()){
            OneGate res = onegate_json(qc_json[j]);
            uint16_t concatenated = (res.gate_name << 6) | res.qubit;
            std::bitset<10> z = concatenated;
            
            //std::cout << z << "\n";
            for (int i = 0; i < z.size(); i++) {
                bool_vector.push_back(z[z.size() -1 - i]); 
            }
        
            //sum = sum + 10;
            //outFile << z ;

            
        }
        else if(std::find(one_gates_parameters.begin(), one_gates_parameters.end(), qc_json[j]["name"]) != one_gates_parameters.end()){
            OnePGate res = onepgate_json(qc_json[j]);
            uint64_t concatenated = (static_cast<uint64_t>(res.gate_name) << 38) | (static_cast<uint64_t>(res.qubit) << 34) | (static_cast<uint64_t>(res.parameter_int) << 23) | static_cast<uint64_t>(res.parameter_dec);
            std::bitset<42> z = concatenated;
            
            for (int i = 0; i < z.size(); i++) {
                bool_vector.push_back(z[z.size() - 1 -i]); 
            }


            //sum = sum + 42;
            //outFile << z ;
            

        }
        else if(std::find(two_gates_no_parameters.begin(), two_gates_no_parameters.end(), qc_json[j]["name"]) != two_gates_no_parameters.end()){
            TwoGate res = twogate_json(qc_json[j]);
            uint16_t concatenated = (res.gate_name << 12) | (res.first_qubit << 6) | res.second_qubit;
            std::bitset<16> z = concatenated;

            for (int i = 0; i < z.size(); i++) {
                bool_vector.push_back(z[z.size() - 1 -i]); 
            }

            //sum = sum + 16;
            //outFile << z  ;

            
        }
        else if(std::find(two_gates_parameters.begin(), two_gates_parameters.end(), qc_json[j]["name"]) != two_gates_parameters.end()){
            TwoPGate res = twopgate_json(qc_json[j]);
            uint64_t concatenated = (static_cast<uint64_t>(res.gate_name) << 44) | (static_cast<uint64_t>(res.first_qubit) << 38) | (static_cast<uint64_t>(res.second_qubit) << 32) | (static_cast<uint64_t>(res.parameter_int) << 23) | static_cast<uint64_t>(res.parameter_dec);
            std::bitset<48> z = concatenated;
            for (int i = 0; i < z.size(); i++) {
            bool_vector.push_back(z[z.size() - 1 -i]); 
            }

            //sum = sum + 48;
            //outFile << z ;

            
        }
        else if(std::find(measures.begin(), measures.end(), qc_json[j]["name"]) != measures.end()){
            Measure res = measure_json(qc_json[j]);
            uint16_t concatenated = (res.measure << 12) | (res.qubit << 6) | res.cbit;
            std::bitset<16> z = concatenated;
            for (int i = 0; i < z.size(); i++) {
            bool_vector.push_back(z[z.size() - 1 -i]);
            }

            //sum = sum + 16;
            //outFile << z ;

            
        }
        else {
            std::cout << "No valid gate. \n";
        }


    }
    

    //std::cout << sum << "\n";

    return bool_vector;

}




std::vector<json> from_bin_to_json(std::vector<bool> bool_vector){
    std::vector<json> circ_json;
    json aux_json;
    int index0 = 0;
    int index1 = 0;
    double decimal = 0;
    int step = 0;


    for (int k = 0; k < bool_vector.size(); k++){

        std::vector<bool> aux_bool_vec(bool_vector.begin() + step, bool_vector.begin() + step + 4);


        for (size_t i = 0; i < aux_bool_vec.size(); i++) {
            index0 = index0 + aux_bool_vec[i]*std::pow(2,i);
        }  


        aux_json["name"] = basic_gates[index0 - 1];
        std::cout << aux_json["name"] << "\n";
        index0 = 0;
        step = step + 4;

        
        if (std::find(one_gates_no_parameters.begin(), one_gates_no_parameters.end(), aux_json["name"]) != one_gates_no_parameters.end()){
            //std::cout << "Dentro de og" << "\n";

            std::vector<bool> aux_bool_vec(bool_vector.begin() + step, bool_vector.begin() + step + 6);

            for (size_t i = 0; i < 6; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2,i);
            }

            aux_json["qubit"] = {index0};
            index0 = 0;

            circ_json.push_back(aux_json);
            aux_json = {};
            step = step + 6;

        }

        else if (std::find(one_gates_parameters.begin(), one_gates_parameters.end(), aux_json["name"]) != one_gates_parameters.end()){
            std::cout << "Index0: " << index0 << "\n";
            std::vector<bool> aux_bool_vec(bool_vector.begin() + step, bool_vector.begin() + step + 38);
            for (size_t i = 0; i < 6; i++) {
                index0 = index0 + aux_bool_vec[aux_bool_vec.size() - 1 - i]*std::pow(2,i);
                std::cout << aux_bool_vec[aux_bool_vec.size() - 1 - i] ;
                }
            aux_json["qubit"] = {index0};
            std::cout << "qubit: " << index0 << "\n";
            index0 = 0;
            
            for (size_t i = 6; i < 15; i++) {
                index0 = index0 + static_cast<int>(aux_bool_vec[aux_bool_vec.size() - 1 - i])*std::pow(2,i);
                }
            std::cout << index0 << "\n";

            for (size_t i = 15; i < 38; i++) {
                index1 = index1 + aux_bool_vec[aux_bool_vec.size() - 1 - i]*std::pow(2,i);
                }
            std::cout << index1 << "\n";

            double decimal = index1 / pow(10, std::to_string(index0).length());

            aux_json["params"] = {static_cast<double>(index0) + decimal};
            

            circ_json.push_back(aux_json);
            aux_json = {};
            
            index0 = 0;
            index1 = 0;
            decimal = 0;
            step = step + 38;

            
            }
        
        else if (std::find(two_gates_no_parameters.begin(), two_gates_no_parameters.end(), aux_json["name"]) != two_gates_no_parameters.end()){
            //std::cout << "Dentro de tg" << "\n";
            std::vector<bool> aux_bool_vec(bool_vector.begin() + step, bool_vector.begin() + step + 12);
            for (size_t i = 0; i <6; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2,i);
                }
            for (size_t i = 6; i < 12; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2,i);
                }

            aux_json["qubits"] = {index0, index1};
            circ_json.push_back(aux_json);
            aux_json = {};

            index0 = 0;
            index1 = 0;
            step = step + 12;
            
            }
        
        else if (std::find(two_gates_parameters.begin(), two_gates_parameters.end(), aux_json["name"]) != two_gates_parameters.end()){
            //std::cout << "Dentro de tpg" << "\n";
            std::vector<bool> aux_bool_vec(bool_vector.begin() + step, bool_vector.begin() + step + 44);
            for (size_t i = 0; i <6; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2,i);
                }
            for (size_t i = 6; i < 12; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2,i);
                }

            aux_json["qubits"] = {index0, index1};
            index0 = 0;
            index1 = 0;

            for (size_t i = 12; i < 21; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2,i);
                }


            for (size_t i = 21; i < 44; i++) {
                index1 = index1 + aux_bool_vec[i]*std::pow(2,i);
                }

            double decimal = index1 / pow(10, std::to_string(index1).length());

            aux_json["params"] = {static_cast<double>(index0) + decimal};

            circ_json.push_back(aux_json);
            aux_json = {};

            index0 = 0;
            index1 = 0;
            step = step + 44;
            
            }
        
        else if (std::find(measures.begin(), measures.end(), aux_json["name"]) != measures.end()){
            //std::cout << "Dentro de meas" << "\n";
            std::vector<bool> aux_bool_vec(bool_vector.begin() + step, bool_vector.begin() + step + 12);
            for (size_t i = 0; i < 6; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2,i);
                }
            
            aux_json["qubits"] = {index0};
            index0 = 0;

            for (size_t i = 6; i < 12; i++) {
                index0 = index0 + aux_bool_vec[i]*std::pow(2,i);
            }

            aux_json["memory"] = {index0};

            circ_json.push_back(aux_json);
            aux_json = {};

            index0 = 0;
            index1 = 0;
            step = step + 12;
            
            }
        else {
            std::cout << "Error" << "\n";
        }
    

    }




    return circ_json;
}














