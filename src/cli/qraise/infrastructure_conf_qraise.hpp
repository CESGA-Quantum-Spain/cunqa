#pragma once

#include <string>
#include <fstream>
#include <vector>

#include "argparse/argparse.hpp"
#include "args_qraise.hpp"

#include "utils/json.hpp"
#include "utils_qraise.hpp"
#include "logger.hpp"

using namespace cunqa;

namespace {
    
}

void write_sbatch_file_from_infrastructure(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    //------------------ Variables block ----------------------------
    JSON infrastructure;
    std::ifstream f(args.infrastructure.value());
    infrastructure = JSON::parse(f);

    auto qpus = infrastructure.at("qpus").get<JSON>();
    auto classical_connectivity = infrastructure.at("classical_connectivity").get<std::vector<std::string>>();
    auto quantum_connectivity = infrastructure.at("quantum_connectivity").get<std::vector<std::vector<std::string>>>();
    auto classical_resources = infrastructure.at("classical_resources").get<JSON>();
    int n_cc_qpus = 0;
    int n_qc_qpus = 0;

    std::vector<std::string> written_qpus;
    std::vector<std::string> qpus_class_resources;
    bool qpu_already_written;
    bool classical_resources_read;

    std::string simulator;
    std::string backend_path;
    std::string path;
    std::string qpus_path;
    std::string family_name;

    //---------------------------------------------------------------


    //----------------- Sbatch header block --------------------------
    int total_number_of_cores = 0;
    int total_memory_in_gb = 0;
    for (const auto& qc_group : quantum_connectivity) {
        for (const auto& qc_qpu : qc_group) {
            qpus_class_resources.push_back(qc_qpu);
            total_number_of_cores += classical_resources.at("qpus").at(qc_qpu).at("cores_per_qpu").get<int>();
            total_memory_in_gb += classical_resources.at("qpus").at(qc_qpu).at("memory").get<int>();
        }
        total_number_of_cores += qc_group.size();
        total_memory_in_gb += qc_group.size();
    }

    for (const auto& qpu : qpus.items()) {
        classical_resources_read = std::find(qpus_class_resources.begin(), qpus_class_resources.end(), qpu.key()) != qpus_class_resources.end();
        if (classical_resources_read) {continue;}
        total_number_of_cores += classical_resources.at("qpus").at(qpu.key()).at("cores_per_qpu").get<int>();
        total_memory_in_gb += classical_resources.at("qpus").at(qpu.key()).at("memory").get<int>();
    }
    
    int n_nodes = std::ceil(total_number_of_cores/64.0);
    if (!check_time_format(args.time)) {
        LOGGER_ERROR("Time format is incorrect, must be: xx:xx:xx.");
        return;
    }

    LOGGER_DEBUG("Just before writing");
    sbatchFile << "#!/bin/bash\n\n";
    sbatchFile << "#SBATCH --job-name=qraise \n";
    sbatchFile << "#SBATCH --ntasks=" << std::to_string(total_number_of_cores) << "\n";
    sbatchFile << "#SBATCH --mem=" << std::to_string(total_memory_in_gb) << "G\n";
    sbatchFile << "#SBATCH -N " << std::to_string(n_nodes) << "\n";
    sbatchFile << "#SBATCH --time=" << args.time << "\n";
    sbatchFile << "#SBATCH --output=qraise_%j\n";
    //sbatchFile << "#SBATCH --ntasks-per-node=" << 1 << "\n";
    //sbatchFile << "#SBATCH -c " << std::to_string(cores_per_task) << "\n"; 
    //sbatchFile << "#SBATCH --mem-per-cpu=" << std::to_string(mem_per_qpu) << "G\n";
    //sbatchFile << "#SBATCH --nodelist=c7-3 \n";
    //--------------------------------------------------------


    // ------ Directory and enviroment variables block -------
    sbatchFile << "EPILOG_PATH=" << std::string(constants::CUNQA_PATH) << "/epilog.sh\n";
    //--------------------------------------------------------


    //--------- Quantum communications block -----------------
    bool first_qc_group = true;
    for (auto& qc_group : quantum_connectivity) {
        if (!first_qc_group) {
            sbatchFile << " &\n\n";
        }
        first_qc_group = false;

        int group_cores = 0;
        int group_memory = 0;
        bool first_qpu = true;
        qpus_path = R"({"backend_from_infrastructure":{)";
        for (auto& qc_qpu : qc_group) {
            if (!first_qpu) {
                qpus_path += ", ";
            }
            first_qpu = false;

            group_cores += classical_resources.at("qpus").at(qc_qpu).at("cores_per_qpu").get<int>();
            group_memory += classical_resources.at("qpus").at(qc_qpu).at("memory").get<int>();
            simulator = qpus.at(qc_qpu).at("simulator").get<std::string>();
            path = qpus.at(qc_qpu).at("backend").get<std::string>();
            qpus_path += "\"" + qc_qpu + "\":\"" + path + "\"";

            written_qpus.push_back(qc_qpu);
            n_qc_qpus++;
        }
        qpus_path += R"(}})";

        sbatchFile << "srun -n " + std::to_string(qc_group.size()) + " -c 1 --mem-per-cpu=1G --exclusive --task-epilog=$EPILOG_PATH setup_qpus co_located qc " + qc_group[0] + " " + simulator + " \'" + qpus_path + "\' &\n";

        sbatchFile << "sleep 1\n";

        sbatchFile << "srun -n 1 -c " + std::to_string(group_cores) + " --mem=" + std::to_string(group_memory) + "G --exclusive setup_executor " + simulator + " " + qc_group[0];
    }
    //------------------------------------------------------


    //----------- Classical communications block -----------
    if ((classical_connectivity.size() > 0) && (quantum_connectivity.size() > 0)) {
        sbatchFile << " &\n\n";
    }

    bool first_cc_qpu = true;
    for (auto& cc_qpu : classical_connectivity) {
        qpu_already_written = std::find(written_qpus.begin(), written_qpus.end(), cc_qpu) != written_qpus.end();
        if (qpu_already_written) { continue; }
        if (!first_cc_qpu) {
            sbatchFile << " &\n\n";
        }
        first_cc_qpu = false;

        int qpu_cores = classical_resources.at("qpus").at(cc_qpu).at("cores_per_qpu").get<int>();
        int qpu_memory = classical_resources.at("qpus").at(cc_qpu).at("memory").get<int>();
        simulator = qpus.at(cc_qpu).at("simulator").get<std::string>();
        backend_path = qpus.at(cc_qpu).at("backend").get<std::string>();
        qpus_path = R"({"backend_from_infrastructure":{")" + cc_qpu + "\":\"" + backend_path + R"("}})";

        sbatchFile << "srun -n 1 -c " + std::to_string(qpu_cores) + " --mem=" + std::to_string(qpu_memory) + "G --exclusive --task-epilog=$EPILOG_PATH setup_qpus co_located cc " + cc_qpu + " " + simulator + " \'" + qpus_path + "\'";

        written_qpus.push_back(cc_qpu);
        n_cc_qpus++;    
    }

    //-----------------------------------------------------


    //--------------- Simple QPUs block -------------------
    if (((classical_connectivity.size() > 0) || (quantum_connectivity.size() > 0)) && (qpus.size() > n_cc_qpus + n_qc_qpus)) {
        sbatchFile << " &\n\n";
    }

    bool first_simple_qpu = true;
    for (auto& [name, properties] : qpus.items()) {
        qpu_already_written = std::find(written_qpus.begin(), written_qpus.end(), name) != written_qpus.end();
        if (qpu_already_written) { continue; }
        if (!first_simple_qpu) {
            sbatchFile << " &\n\n"; 
        }
        first_simple_qpu = false;

        int qpu_cores = classical_resources.at("qpus").at(name).at("cores_per_qpu").get<int>();
        int qpu_memory = classical_resources.at("qpus").at(name).at("memory").get<int>();
        simulator = properties.at("simulator").get<std::string>();
        backend_path = properties.at("backend").get<std::string>();
        qpus_path = R"({"backend_from_infrastructure":{")" + name + "\":\"" + backend_path +  R"("}})" ;

        sbatchFile << "srun -n 1 -c " + std::to_string(qpu_cores) + " --mem=" + std::to_string(qpu_memory) + "G --exclusive --task-epilog=$EPILOG_PATH setup_qpus co_located no_comm "  + name + " " + simulator + " \'" + qpus_path + "\'"; 
        
    }
    //--------------------------------------------------
}