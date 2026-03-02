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

void remove_tmp_files(const std::string filepath = "")
{
    if (!filepath.empty()) {
        std::string rmv_cmd = "rm " + filepath;
        std::system(rmv_cmd.c_str());
    } else {
        std::system("rm qraise_sbatch_tmp.sbatch");
    }
}