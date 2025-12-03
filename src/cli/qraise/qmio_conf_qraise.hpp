#pragma once

#include <string>
#include <fstream>

#include "argparse/argparse.hpp"
#include "args_qraise.hpp"


#include "logger.hpp"

void write_qmio_sbatch(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    std::string home = std::getenv("HOME");
    std::string cunqa_path = home + "/cunqa/";

    sbatchFile << "#!/bin/bash\n";
    sbatchFile << "#SBATCH --job-name=qraise \n";
    sbatchFile << "#SBATCH --partition qpu \n";
    sbatchFile << "# SBATCH --nodelist=c7-23 \n";
    sbatchFile << "#SBATCH --ntasks=" << 1 << "\n";
    sbatchFile << "#SBATCH --mem-per-cpu=4G \n";
    sbatchFile << "#SBATCH --time=" << args.time << "\n";

    sbatchFile << "#SBATCH --output=qraise_%j\n";

    sbatchFile << "\n\n";

    sbatchFile << "srun --task-epilog=$EPILOG_PATH setup_qmio";

}