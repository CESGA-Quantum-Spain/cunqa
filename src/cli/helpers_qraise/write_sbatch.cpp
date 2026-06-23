#include <regex>
#include <fstream>
#include <cmath>
#include <cctype>
#include <cstdio> // For popen, pclose
#include <algorithm>
#include <filesystem>
#include <stdexcept>

#include "write_sbatch.hpp"
#include "utils_qraise.hpp"
#include "args_qraise.hpp"

#include "utils/json.hpp"
#include "utils/constants.hpp"

#include "logger.hpp"

namespace {

void write_sbatch_header(std::ofstream& sbatchFile, const CunqaArgs& args, Communications scheme)
{
    sbatchFile << "#!/bin/bash\n";
    sbatchFile << "#SBATCH --job-name=qraise \n";

    const int ntasks = (scheme == Communications::QC) ? args.n_qpus + 1 : args.n_qpus;
    sbatchFile << "#SBATCH --ntasks=" << ntasks << "\n";

    sbatchFile << "#SBATCH -c " << args.cores_per_qpu << "\n";

    if (args.partition.has_value())
            sbatchFile << "#SBATCH --partition=" << args.partition.value() << "\n";

    if (args.n_nodes.has_value())
        sbatchFile << "#SBATCH -N " << args.n_nodes.value() << "\n";

    if (args.qpus_per_node.has_value())
        sbatchFile << "#SBATCH --ntasks-per-node=" << args.qpus_per_node.value() << "\n";

    if (args.node_list.has_value()) {
        sbatchFile << "#SBATCH --nodelist=";

        const auto& nodes = args.node_list.value();
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (i > 0)
                sbatchFile << ",";
            sbatchFile << nodes[i];
        }
        sbatchFile << "\n";
    }
    
    if (args.mem_per_qpu.has_value()) {
        if (args.n_nodes.has_value()) {
            const int mem_per_cpu = static_cast<int>(args.mem_per_qpu.value() / args.cores_per_qpu);
            sbatchFile << "#SBATCH --mem-per-cpu=" << mem_per_cpu << "G\n";
        } else {
            const int total_mem = args.mem_per_qpu.value() * args.n_qpus;
            sbatchFile << "#SBATCH --mem=" << total_mem << "G\n";
        }
    }

    sbatchFile << "#SBATCH --time=" << args.time << "\n";
    sbatchFile << "#SBATCH --output=qraise_%j\n\n";
    
    if (args.gpu || args.gpu_name != "default") {
        if (args.simulator != "Aer")
            throw std::invalid_argument("At this moment, only Aer supports GPU simulation");
        const int number_of_gpus = scheme == Communications::QC ? 1 : args.n_qpus;
        
        if (args.gpu_name != "default")
            sbatchFile << "#SBATCH --gres=gpu:" << args.gpu_name << ":" << number_of_gpus << "\n";
        else
            sbatchFile << "#SBATCH --gres=gpu:" << number_of_gpus << "\n";
    }

    sbatchFile << "unset SLURM_MEM_PER_CPU SLURM_MEM_PER_NODE SLURM_CPU_BIND_LIST SLURM_CPU_BIND\n";
    sbatchFile << "EPILOG_PATH=" << cunqa::INSTALL_PATH << "/bin/epilog.sh\n";
}

void write_run_command(std::ofstream& sbatchFile,
                       const CunqaArgs& args,
                       Communications scheme)
{
    auto subcommand =
        std::string(args.co_located ? "co_located" : "hpc") + " " +
        std::string(get_communications_name(scheme)) + " " +
        args.family + " " + args.simulator;

    if (args.infrastructure.has_value()) { 
        subcommand += " " + args.infrastructure.value();
    } else if (args.backend.has_value()) {
        subcommand += " " + args.backend.value();
    }

    if (scheme == Communications::NC) {
        sbatchFile << "srun --task-epilog=$EPILOG_PATH setup_qpus " << subcommand << "\n";
    } else if (scheme == Communications::CC) {
        sbatchFile << "srun";
    #ifdef USE_MPI_BTW_QPU
        sbatchFile << " --mpi=pmix";
    #endif
        sbatchFile << " --task-epilog=$EPILOG_PATH setup_qpus " << subcommand << '\n';
    } else if (scheme == Communications::QC) {

        std::vector<std::string> gpu_info{"", ""}; 
        if (args.gpu || args.gpu_name != "default") {
            gpu_info[0] = "--gres=gpu:0 ";
            if (args.gpu_name != "default")
                gpu_info[1] = "--gres=gpu:1 ";
            else
                gpu_info[1] = "--gres=gpu:" + args.gpu_name + ":1 ";
        }

        sbatchFile 
            << "srun --exclusive -n " << args.n_qpus << " -c 1 --mem-per-cpu=1G "
            << gpu_info[0] << "--task-epilog=$EPILOG_PATH setup_qpus " << subcommand << " &\n";

        sbatchFile << "srun --exclusive -n 1 -c ";

        const int simulator_n_cores = args.n_qpus * (args.cores_per_qpu - 1);
        if (simulator_n_cores <= 0)
            throw std::invalid_argument("cores_per_qpu must be equal or greater than 2.");

        sbatchFile << std::to_string(simulator_n_cores) << " ";

        if (args.mem_per_qpu.has_value()) {
            const int simulator_memory = args.n_qpus * (args.mem_per_qpu.value() - 1);
            sbatchFile << "--mem=" << simulator_memory << "G ";
        }

        sbatchFile 
            << gpu_info[1] << "setup_executor " << args.simulator << " " << args.n_qpus << "\n";
    }
}


void write_qmio(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    sbatchFile << "#!/bin/bash\n";
    sbatchFile << "#SBATCH --job-name=qraise \n";
    sbatchFile << "#SBATCH --partition qpu \n";
    sbatchFile << "#SBATCH --ntasks=1 \n";
    sbatchFile << "#SBATCH -c 2 \n";
    sbatchFile << "#SBATCH --mem-per-cpu=15G \n";
    sbatchFile << "#SBATCH --time=" << args.time << "\n";

    sbatchFile << "#SBATCH --output=qraise_%j\n";

    sbatchFile << "\n\n";

    sbatchFile << "unset SLURM_MEM_PER_CPU SLURM_MEM_PER_NODE SLURM_CPU_BIND_LIST SLURM_CPU_BIND\n";
    sbatchFile << "EPILOG_PATH=" << cunqa::INSTALL_PATH << "/bin/epilog.sh\n";

    sbatchFile << "\n\n";
    sbatchFile << "srun --task-epilog=$EPILOG_PATH setup_qmio " + args.family;
}

} // End of anonymous namespace

void write_sbatch(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    if (args.cc && args.qc)
        LOGGER_WARN("cc and qc both selected: using qc.");

    const std::string_view scheme_name = 
        args.cc ? "cc" :
        args.qc ? "qc" : "nc";
    auto scheme = get_communications_class(scheme_name);

    if (args.n_qpus == 0 || args.time == "")
        throw std::invalid_argument("qraise needs two mandatory arguments:\n \t " 
            "-n: number of vQPUs to be raised\n\t "
            "-t: maximum time vQPUs will be raised (hh:mm:ss)\n");

    if (!communication_supported_by_simulator(args.simulator, scheme))
        throw std::invalid_argument("Simulator " + args.simulator + 
            " is not available for " + get_communications_name(scheme) + 
            " communications simulation. Aborting.");

    if (exists_family_name(args.family, cunqa::QPUS_FILEPATH))
        throw std::runtime_error("There are QPUs with the same family name as the provided: " +
            args.family + ".");

    if (!args.qmio) {
        write_sbatch_header(sbatchFile, args, scheme);
        write_run_command(sbatchFile, args, scheme);
    } else
        write_qmio(sbatchFile, args);
}