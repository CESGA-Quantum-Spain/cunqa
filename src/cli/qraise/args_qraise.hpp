#pragma once

#include <optional>

#include "argparse.hpp"
#include "logger.hpp"

using namespace std::literals;

struct CunqaArgs : public argparse::Args 
{
    //int& node = kwarg("node", "Specific node to raise the qpus."); //Álvaro 
    int& n_qpus                          = kwarg("n,num_qpus", "Number of QPUs to be raised.").set_default(1);
    std::string& time                    = kwarg("t,time", "Time for the QPUs to be raised.");
    int& cores_per_qpu                   = kwarg("c,cores", "Number of cores per QPU.").set_default(2);
    int& mem_per_qpu                     = kwarg("mem,mem-per-qpu", "Memory given to each QPU in GB.").set_default(8);
    std::optional<std::size_t>& number_of_nodes  = kwarg("N,n_nodes", "Number of nodes.").set_default(1);
    std::optional<std::vector<std::string>>& node_list = kwarg("node_list", "List of nodes where the QPUs will be deployed.").multi_argument(); 
    std::optional<int>& qpus_per_node    = kwarg("qpuN,qpus_per_node", "Number of qpus in each node.");
    std::optional<std::string>& backend  = kwarg("b,backend", "Path to the backend config file.");
    std::optional<std::string>& noise_properties  = kwarg("noise-prop,noise-properties", "Path to the noise properties json file, only supported for simulator Aer.");
    std::string& simulator               = kwarg("sim,simulator", "Simulator reponsible of running the simulations.").set_default("Aer");
    
    // fakeqmio kwarg and flags
    std::optional<std::string>& fakeqmio = kwarg("fq,fakeqmio", "Raise FakeQmio backend from calibration file.", /*implicit*/"last_calibrations");
    bool& no_thermal_relaxation          = flag("no-thermal-relaxation", "Deactivate thermal relaxation on FakeQmio.").set_default("false");
    bool& no_readout_error               = flag("no-readout-error", "Deactivate readout error on FakeQmio.").set_default("false");
    bool& no_gate_error                  = flag("no-gate-error", "Deactivate gate error on FakeQmio.").set_default("false");

    std::string& family_name             = kwarg("fam,family_name", "Name that identifies which QPUs were raised together.").set_default("default");
    //bool& hpc                          = flag("hpc", "Default HPC mode. The user can connect with the local node QPUs.");
    bool& cloud                          = flag("cloud", "CLOUD mode. The user can connect with any deployed QPU.");
    bool& cc                             = flag("classical_comm", "Enable classical communications.");
    bool& qc                             = flag("quantum_comm", "Enable quantum communications.");
    bool& qmio                           = flag("qmio", "Deploy QMIO.");

    void welcome() {
        std::cout << "Welcome to qraise command, a command responsible for turning on the required QPUs.\n" << std::endl;
    }
};