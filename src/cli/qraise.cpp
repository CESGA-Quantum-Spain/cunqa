#include "argparse.hpp"
#include "simulators/simulator.hpp"
#include <string>
#include <regex>

using namespace std::literals;

struct MyArgs : public argparse::Args {
    int& n_qpus                         = kwarg("n,num_qpus", "Number of QPUs to be raised.");
    std::string& time                   = kwarg("t,time", "Time for the QPUs to be raised.");
    std::optional<std::string>& backend = kwarg("b,backend_path", "Path to the backend config file.");
    std::string& simulator              = kwarg("sim,simulator", "Simulator reponsible of running the simulations.").set_default("Aer"); 
    std::string& mem_per_qpu            = kwarg("mem-per-qpu", "Memory given to each QPU.").set_default("1G");


    void welcome() {
        std::cout << "Welcome to qraise command, a command responsible for turn on the required QPUs.\n" << std::endl;
    }
};

bool check_time_format(const std::string& time)
{
    std::regex format("^(\\d{2}):(\\d{2}):(\\d{2})$");
    return std::regex_match(time, format);   
}

bool check_mem_format(const std::string& mem) {
    std::regex format("^(\\d{1,2})G$");
    return std::regex_match(mem, format);
}

int main(int argc, char* argv[]) {
    auto args = argparse::parse<MyArgs>(argc, argv);

    std::ofstream sbatchFile("qraise_sbatch_tmp.sbatch");

    // Escribir el contenido del script SBATCH
    sbatchFile << "#!/bin/bash\n";
    sbatchFile << "#SBATCH --job-name=qraise \n";
    sbatchFile << "#SBATCH -c 2 \n";
    sbatchFile << "#SBATCH --ntasks=" << args.n_qpus << "\n";
    sbatchFile << "#SBATCH -N 1 \n";

    // TODO: Can the user decide the number of cores?
    if (check_mem_format(args.mem_per_qpu)) 
    {
        int mem_per_qpu = args.mem_per_qpu[0] - '0';
        sbatchFile << "#SBATCH --mem-per-cpu=" << mem_per_qpu*2 << "G\n";
    } else
        std::cerr << "ERROR: Memory format is incorrect, must be: xG (where x is the number of Gigabytes)\n";

    if (check_time_format(args.time))
        sbatchFile << "#SBATCH --time=" << args.time << "\n";
    else
        std::cerr << "ERROR: Time format is incorrect, must be: xx:xx:xx\n";

    sbatchFile << "#SBATCH --output=qraise_%j\n";

    sbatchFile << "\n";
    sbatchFile << "if [ ! -d \"$STORE/.api_simulator\" ]; then\n";
    sbatchFile << "mkdir $STORE/.api_simulator\n";
    sbatchFile << "fi\n";

    const char* var_name = "INSTALL_PATH"; // Replace with your variable name
    const char* var_install_path = std::getenv(var_name);

    if (var_install_path) {
        sbatchFile << "BINARIES_DIR=" << var_install_path << "/bin\n";
    } else {
        std::cerr << "Environment variable INSTALL_PATH is not set: aborting.\n"; 
    }

    sbatchFile << "export INFO_PATH=" << std::getenv("STORE") << "/.api_simulator/qpu.json\n";
    if(args.backend.has_value())
        sbatchFile << "srun --task-epilog=$BINARIES_DIR/epilog.sh setup_qpus $INFO_PATH " << args.simulator.c_str() << " " << args.backend.value().c_str() << "\n";
    else
        sbatchFile << "srun --task-epilog=$BINARIES_DIR/epilog.sh setup_qpus $INFO_PATH " << args.simulator.c_str() << "\n";
    //sbatchFile << "rm -rf $STORE/.api_simulator\n";
    sbatchFile.close();

    std::system("sbatch qraise_sbatch_tmp.sbatch");
    std::system("rm qraise_sbatch_tmp.sbatch");

    return 0;
}