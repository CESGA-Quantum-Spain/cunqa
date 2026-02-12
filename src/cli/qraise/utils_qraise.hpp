#pragma once

#include <string>
#include <regex>
#include <fstream>
#include <cmath>
#include <cstdio> // For popen, pclose
#include <algorithm>
#include <filesystem>

#include "args_qraise.hpp"
#include "utils/json.hpp"
#include "logger.hpp"

namespace fs = std::filesystem;


constexpr int DEFAULT_MEM_PER_CORE = 15;

bool check_time_format(const std::string& time)
{
    std::regex format_hours_minutes_seconds("^(\\d+):(\\d{2}):(\\d{2})$");
    std::regex format_days_hours("^(\\d+)-(\\d{1,2})$");
    std::regex format_days_hours_minutes_seconds("^(\\d+)-(\\d{1,2}):(\\d{2}):(\\d{2})$");
    
    return std::regex_match(time, format_hours_minutes_seconds) || std::regex_match(time, format_days_hours) || std::regex_match(time, format_days_hours_minutes_seconds);   
}

bool check_mem_format(const int& mem) 
{
    std::string mem_str = std::to_string(mem) + "G";
    std::regex format("^(\\d{1,4})G$");
    
    return std::regex_match(mem_str, format);
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

int compute_needed_n_tasks(const int& cores_per_qpu){

}

void remove_tmp_files(const std::string directory = "./")
{
    std::system("rm qraise_sbatch_tmp.sbatch");
}