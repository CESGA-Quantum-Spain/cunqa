#pragma once

#include <optional>

#include "argparse/argparse.hpp"
#include "logger.hpp"

using namespace std::literals;

struct CunqaArgs : public argparse::Args 
{
    int& n_qpus                                         = kwarg("n,num_qpus", "Number of QPUs to be raised.").set_default(0);
    std::string& time                                   = kwarg("t,time", "Time for the QPUs to be raised.").set_default("");
    int& cores_per_qpu                                  = kwarg("c,cores", "Number of cores per QPU.").set_default(2);
    std::optional<std::string>& partition               = kwarg("p,partition", "Partition requested for the QPUs.");
    std::optional<int>& mem_per_qpu                     = kwarg("mem,mem-per-qpu", "Memory given to each QPU in GB.");
    std::optional<std::size_t>& number_of_nodes         = kwarg("N,n_nodes", "Number of nodes.").set_default(1);
    std::optional<std::vector<std::string>>& node_list  = kwarg("node_list", "List of nodes where the QPUs will be deployed.").multi_argument(); 
    std::optional<int>& qpus_per_node                   = kwarg("qpuN,qpus_per_node", "Number of qpus in each node.");
    std::optional<std::string>& backend                 = kwarg("b,backend", "Path to the backend config file.");
    std::string& simulator                              = kwarg("sim,simulator", "Simulator reponsible of running the simulations.").set_default("Aer");

    std::string& family_name                            = kwarg("fam,family_name", "Name that identifies which QPUs were raised together.").set_default("default");
    bool& co_located                                    = flag("co-located", "co-located mode. The user can connect with any deployed QPU.");
    bool& cc                                            = flag("classical_comm", "Enable classical communications.");
    bool& qc                                            = flag("quantum_comm", "Enable quantum communications.");
    std::optional<std::string>& infrastructure          = kwarg("infrastructure", "Path to a infrastructure of QPUs.");

    void welcome() {
        std::cout << "Welcome to qraise command, a command responsible for turning on the required QPUs.\n" << std::endl;
    }
};