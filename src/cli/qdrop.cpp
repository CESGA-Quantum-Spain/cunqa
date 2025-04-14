#include <string>
#include <fstream>
#include <slurm/slurm.h>
#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <stdlib.h>
#include <stdexcept>
#include <set>
#include <nlohmann/json.hpp>
#include "argparse.hpp"
#include "logger/logger.hpp"

using json = nlohmann::json;
using namespace std::literals;
struct MyArgs : public argparse::Args
{
    std::optional<std::vector<uint32_t>>& ids = arg("Slurm IDs of the QPUs to be dropped.").multi_argument();
    std::optional<std::string>& info_path     = kwarg("info_path", "PATH to the QPU information file.");
    bool &all                                 = flag("all", "All qraise jobs will be dropped.");
};
int isValidSlurmJobID(uint32_t jobID)
{
    job_info_msg_t *jobInfoMsg = nullptr;
    slurm_init(NULL);
    int ret = slurm_load_job_user(&jobInfoMsg, static_cast<uint32_t>(getuid()), SHOW_LOCAL);
    if (ret != SLURM_SUCCESS) {
        SPDLOG_LOGGER_ERROR(logger, "Could not retrieve the SLURM jobs. {}", slurm_strerror(ret));
        return false;
    }
    int found = 2;
    for (uint32_t i = 0; i < jobInfoMsg->record_count; ++i) {
        job_info_t *jobInfo = &jobInfoMsg->job_array[i];
        if (jobInfo->job_id == jobID && jobInfo->job_state == JOB_RUNNING) {
            std::string(jobInfo->name) != "qraise"s ? found = 1 : found = 0;
            break;
        }
    }
    slurm_free_job_info_msg(jobInfoMsg);
    slurm_fini();
    return found;
}

void removeAllJobs(std::string& id_str)
{
    job_info_msg_t *jobInfoMsg = nullptr;

    slurm_init(NULL);

    int ret = slurm_load_job_user(&jobInfoMsg, static_cast<uint32_t>(getuid()), SHOW_LOCAL);
    if (ret != SLURM_SUCCESS) {
        SPDLOG_LOGGER_ERROR(logger, "Could not retrieve the SLURM jobs. {}", slurm_strerror(ret));
        return;
    } else {
        for (uint32_t i = 0; i < jobInfoMsg->record_count; ++i) {
            job_info_t *jobInfo = &jobInfoMsg->job_array[i];
            if (std::string(jobInfo->name) == "qraise"s && jobInfo->job_state == JOB_RUNNING) {
                id_str += std::to_string(jobInfo->job_id) + " ";
            }
        }
    }

    slurm_free_job_info_msg(jobInfoMsg);
    slurm_fini();
}

int main(int argc, char* argv[]) 
{
    auto args = argparse::parse<MyArgs>(argc, argv);
    std::string install_path = getenv("INSTALL_PATH");
    setenv("SLURM_CONF", (install_path + "/slurm.conf").c_str(), 1); 
    std::string id_str;
    std::string cmd;

    if (args.all) {
        if (args.ids.has_value())
        std::cerr << "\033[1;33m" << "Warning: " << "\033[0m" << "You arr setting the --all flag and putting IDs (every qraise process will be eliminated).\n";
        removeAllJobs(id_str); 
        if (id_str.empty()) {
            std::cerr << "\033[1;31m" << "Error: " << "\033[0m" << "No qraise jobs are currently running.\n";
            return -1;
        }
    } else if (args.info_path.has_value()) {
        std::ifstream info_path(args.info_path.value());
        json json_file;
        std::set<std::string> unique_job_ids;

        info_path >> json_file;

        for (auto& k : json_file.items()) {
            std::string key = k.key();
            std::string job_id = key.substr(0, key.find('_'));
            unique_job_ids.insert(job_id);

        }

        for (const auto& element : unique_job_ids) {
            id_str += element + " ";
        }

    } else if (!args.ids.has_value()) {
        std::cerr << "\033[1;31m" << "Error: " << "\033[0m" << "You must specify the IDs of the jobs to be removed or use the --all flag.\n";
        return -1;
    } else {
        for(const auto& id : args.ids.value()){
            switch(isValidSlurmJobID(id)){
                case 0:
                    id_str += std::to_string(id) + " ";
                    SPDLOG_LOGGER_DEBUG(logger, "Removed QPUs from qraise job {}.", id);
                    break;
                case 1:
                    std::cerr << "\033[1;33m" << "Warning: " << "\033[0m" << "You are removing a job different than qraise, use scancel if wou want to stop it.\n";
                    return -1;
                    break;
                default:
                    std::cerr << "\033[1;31m" << "Error: " << "\033[0m" << "No job with ID "
                          << id << " is currently running.\n";
                    return -1;
            }
        }
    }
    
    std::system(("scancel "s + id_str).c_str());
    std::cout << "Removed job(s) with ID(s): \033[1;32m"
                    << id_str
                    << "\033[0m" << "\n";
    return 0;
}