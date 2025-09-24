#pragma once

#include <string>
#include <regex>
#include <fstream>
#include <cmath>

#include "utils/json.hpp"
#include "logger.hpp"

bool check_time_format(const std::string& time)
{
    std::regex format("^(\\d{2}):(\\d{2}):(\\d{2})$");
    return std::regex_match(time, format);   
}

bool check_mem_format(const int& mem) 
{
    std::string mem_str = std::to_string(mem) + "G";
    std::regex format("^(\\d{1,2})G$");
    return std::regex_match(mem_str, format);
}

int check_memory_specs(const int& mem_per_qpu, const int& cores_per_qpu)
{
    int mem_per_cpu = mem_per_qpu/cores_per_qpu;
    auto system_var = std::getenv("LMOD_SYSTEM_NAME");
    if (system_var == nullptr) system_var = "QMIO";
    if ((std::string(system_var) == "QMIO" && mem_per_cpu > 15)) {
        return 1;
    } else if ((std::string(system_var) == "FT3" && mem_per_cpu > 4)){
        return 2;
    }
    return 0;
}

bool exists_family_name(const std::string& family, const std::string& info_path)
{
    std::ifstream file(info_path);
    if (!file) {
        return false;
    } else {
        cunqa::JSON qpus_json;
        try {
            file >> qpus_json;
            for (auto& [key, value] : qpus_json.items()) {
                if (value["family"] == family) {
                    return true;
                } 
            }
            return false;
        } catch (const std::exception& e) {
            LOGGER_DEBUG("The qpus.json file was completely empty. An empty json will be written on it.");
            file.close();
            std::ofstream out(info_path);
            if (!out) {
                LOGGER_DEBUG("Impossible to open the empty qpus.json file to write on it. It will be deleted and created again");
                std::remove(info_path.c_str());
                out.open(info_path);
                return false;  
            }
            out << "{ }";  
            out.close();

            return false;
        }
    }
}

bool check_simulator_name(const std::string& sim_name)
{
    if (sim_name == "Cunqa" || sim_name == "Munich" || sim_name == "Aer" || sim_name == "Maestro") {  // Add new valid simulators to the check here
        return true;
    } else {
        return false;
    }
}

int number_of_nodes(const int& number_of_qpus, const int& cores_per_qpu, const int& number_of_nodes, const int& cores_per_node)
{
    if (number_of_qpus * cores_per_qpu < number_of_nodes * cores_per_node) {
        return number_of_nodes;
    } else {
        float aprox_number_of_nodes = ((float)number_of_qpus * (float)cores_per_qpu)/(float)cores_per_node;
        return static_cast<int>(std::ceil(aprox_number_of_nodes));

    }
}