#include <string>
#include <fstream>
#include <regex>
#include <any>
#include <filesystem> //debug

#include <iostream>
#include <cstdlib>

#include "argparse/argparse.hpp"

#include "utils/constants.hpp"
#include "qraise/utils_qraise.hpp"
#include "qraise/args_qraise.hpp"
#include "qraise/noise_model_conf_qraise.hpp"
#include "qraise/simple_conf_qraise.hpp"
#include "qraise/cc_conf_qraise.hpp"
#include "qraise/qc_conf_qraise.hpp"
#include "qraise/qmio_conf_qraise.hpp"
#include "qraise/infrastructure_conf_qraise.hpp"

#include "logger.hpp"

using namespace std::literals;
using namespace cunqa;

namespace fs = std::filesystem;

int main(int argc, char* argv[]) 
{
    auto args = argparse::parse<CunqaArgs>(argc, argv, true); //true ensures an error is raised if we feed qraise an unrecognized flag

    std::ofstream sbatchFile("qraise_sbatch_tmp.sbatch");
    try {
        if (args.infrastructure.has_value()) {
            write_infrastructure_sbatch(sbatchFile, args);
        } else if (args.qmio) {
            write_qmio_sbatch(sbatchFile, args);
        } else if (args.noise_properties.has_value() || args.fakeqmio.has_value()) {
            write_noise_model_sbatch(sbatchFile, args);
        } else if (args.cc) {
            write_cc_sbatch(sbatchFile, args);
        } else if (args.qc) {
            write_qc_sbatch(sbatchFile, args);
        } else {
            write_simple_sbatch(sbatchFile, args);
        }
    } catch (const std::exception& e) {
        sbatchFile.close();
        LOGGER_ERROR("Error writing the sbatch file. Aborting. {}", e.what());
        std::system("rm qraise_sbatch_tmp.sbatch");
        return 1;
    }
    sbatchFile.close();

    // Executing and deleting the file
    std::system("sbatch qraise_sbatch_tmp.sbatch");
    remove_tmp_files();
    
    
    return 0;
}