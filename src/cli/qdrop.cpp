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
    std::optional<std::vector<uint32_t>>& ids       = arg("Slurm IDs of the QPUs to be dropped.").multi_argument();
    std::optional<std::vector<std::string>>& family = kwarg("f,fam", "Family name of the QPUs to be dropped.");
    bool &all                                       = flag("all", "All qraise jobs will be dropped.");
};

struct Job {
    int id;
    std::string state;
    std::string name;
};


cunqa::JSON read_qpus_json() {
    std::ifstream in(cunqa::constants::QPUS_FILEPATH);
    cunqa::JSON j;
    in >> j;
    return j;
}

std::vector<Job> read_qpus() {
    // Run the command and get the output
    std::string cmd = "squeue -h -o \"%i %t %j\" | awk -v name=\"qraise\" 'index($3, name) > 0 {print $1, $2, $3}'";
    std::array<char, 4096> buf{};
    std::string out;

    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed");
    while (fgets(buf.data(), buf.size(), pipe.get())) {
        out += buf.data();
    }

    // Obtain the jobs of the output
    std::istringstream in(out);
    std::vector<Job> jobs; 
    Job j; 
    while (in >> j.id >> j.state >> j.name)
        jobs.push_back(j);
    if(!size(jobs))
        std::cerr << "\033[1;31m" << "Error: " << "\033[0m" << "No qraise jobs are currently running.\n";
    
    return jobs;
}

void removeJobs(const std::vector<Job>& jobs)
{
    std::string scancel = "scancel ";
    std::string job_ids_str;
    for (const auto& job: jobs) {
        job_ids_str += std::to_string(job.id) + " ";
    }
    scancel += job_ids_str; 
    std::system(scancel.c_str());
    std::cout << "Removed job(s) with ID(s): \033[1;32m"
              << job_ids_str
              << "\033[0m" << "\n";
}

void removeJobs(const std::vector<std::string>& job_ids)
{
    std::string scancel = "scancel ";
    std::string job_ids_str;
    for (const auto& job_id: job_ids) {
        job_ids_str += job_id + " ";
    }
    scancel += job_ids_str;
    std::system(scancel.c_str());
    std::cout << "Removed job(s) with ID(s): \033[1;32m"
              << job_ids_str
              << "\033[0m" << "\n";
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
    if(!size(ids))
        std::cerr << "\033[1;31m" << "Error: " << "\033[0m" << "No qraise jobs are currently running with the specified family names.\n";

    return ids;
}

int main(int argc, char* argv[]) 
{
    auto args = argparse::parse<CunqaArgs>(argc, argv);

    if (args.all) {
        auto jobs = read_qpus();

        if (size(jobs)) removeJobs(jobs);
        else return EXIT_FAILURE;
    } else if (args.ids.has_value() && !args.family.has_value()) {
        auto jobs = read_qpus();

        std::unordered_set<int> keep(args.ids.value().begin(), args.ids.value().end());
        auto jobs_rng = jobs | std::views::filter([&](const Job& j){ return keep.count(j.id); });
        auto filtered_jobs = std::vector<Job>(jobs_rng.begin(), jobs_rng.end());

        if (size(filtered_jobs)) removeJobs(filtered_jobs);
        else return EXIT_FAILURE;
    } else if (!args.ids.has_value() && args.family.has_value()) {
        auto ids = find_family_id(read_qpus_json(), args.family.value());

        if (size(ids)) removeJobs(ids);
        else return EXIT_FAILURE;
    } else {
        std::cerr << "\033[1;31m" << "Error: " << "\033[0m" << "You must specify either the IDs or the family name (with -f) of the jobs to be removed or use the --all flag.\n";
        return -1;
    }
    
    return EXIT_SUCCESS;
}