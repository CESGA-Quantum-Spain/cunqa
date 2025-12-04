#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <stdlib.h>
#include <stdexcept>
#include <set>
#include <unordered_set>
#include <ranges>

#include "argparse/argparse.hpp"
//#include "logger.hpp"
#include "utils/constants.hpp"
#include "utils/json.hpp"

using namespace std::literals;

struct CunqaArgs : public argparse::Args
{
    std::optional<std::vector<std::string>>& ids    = arg("Slurm IDs of the QPUs to be dropped.").multi_argument();
    std::optional<std::vector<std::string>>& family = kwarg("fam,family_name", "Family name of the QPUs to be dropped.");
    bool &all                                       = flag("all", "All qraise jobs will be dropped.");
};

cunqa::JSON read_qpus_json() 
{
    std::ifstream in(cunqa::constants::QPUS_FILEPATH);
    cunqa::JSON j;
    in >> j;
    return j;
}

std::vector<std::string> get_qpus_ids(const cunqa::JSON& jobs)
{
    std::vector<std::string> ids;
    std::string last_id;
    for (auto it = jobs.begin(); it != jobs.end(); ++it) {
        const auto& key = it.key();
        std::size_t pos = key.find('_');
        auto id = (pos == std::string::npos) ? key : key.substr(0, pos);

        if(id == last_id) continue;
        ids.push_back(id);
        last_id = id;
    }

    return ids;
}

std::vector<std::string> find_family_id(const cunqa::JSON& qpus, std::vector<std::string> target_families) {
    std::vector<std::string> ids;

    for (const auto& target_family: target_families) {
        for (const auto& [key, entry] : qpus.items()) {
            if (!entry.is_object()) continue;

            auto itFam = entry.find("family");
            auto itJob = entry.find("slurm_job_id");
            if (itFam == entry.end() || itJob == entry.end()) continue;

            std::string fam = itFam->get<std::string>();
            if (fam == target_family) {
                ids.push_back(itJob->get<std::string>());
                break;
            }
        }
    }

    return ids;
}

void removeJobs(const std::vector<std::string>& job_ids, const bool& all = false)
{
    std::string scancel = "scancel ";
    std::string job_ids_str;
    for (const auto& job_id: job_ids) {
        job_ids_str += job_id + " ";
    }
    scancel += job_ids_str;
    std::cout << "cmd: " << scancel.c_str() << "\n";
    std::system(scancel.c_str());
    std::cout << "Removed job(s) with ID(s): \033[1;32m"
              << job_ids_str
              << "\033[0m" << "\n";

    // In case the qpus.json is not correctly removed
    if (all) {
        auto left_jobs = read_qpus_json();
        if(size(left_jobs)) {
            std::ofstream ofs(cunqa::constants::QPUS_FILEPATH, std::ios::trunc);
            ofs << "{}";
        }
    }
}

int main(int argc, char* argv[]) 
{
    auto args = argparse::parse<CunqaArgs>(argc, argv);

    if (args.all) {
        auto ids = get_qpus_ids(read_qpus_json());

        if (size(ids)) removeJobs(ids, true);
        else return EXIT_FAILURE;
    } else if (args.ids.has_value() && !args.family.has_value()) {
        auto ids = get_qpus_ids(read_qpus_json());

        std::unordered_set<std::string> keep(args.ids.value().begin(), args.ids.value().end());
        auto ids_rng = ids | std::views::filter([&](const std::string& id){ return keep.count(id); });
        auto filtered_ids = std::vector<std::string>(ids_rng.begin(), ids_rng.end());

        if (size(filtered_ids)) 
            removeJobs(filtered_ids);
        else {
            std::cerr << "\033[1;33m" << "Warning: " << "\033[0m" 
                      << "No qraise jobs are currently running with the specified id.\n";
            return EXIT_FAILURE;
        } 
    } else if (!args.ids.has_value() && args.family.has_value()) {
        auto ids = find_family_id(read_qpus_json(), args.family.value());

        if (size(ids)) 
            removeJobs(ids);
        else {
            std::cerr << "\033[1;33m" << "Warning: " << "\033[0m" 
                      << "No qraise jobs are currently running with the specified family names.\n";
            return EXIT_FAILURE;
        }
    } else {
        std::cerr << "\033[1;31m" << "Error: " << "\033[0m" 
                  << "You must specify either the IDs or the family name (with --fam) "
                  << "of the jobs to be removed, or use the --all flag.\n";
        return -1;
    }
    
    return EXIT_SUCCESS;
}