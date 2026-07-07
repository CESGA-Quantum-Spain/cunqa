#pragma once

#include <iosfwd>  
#include <string>   

#include "args_qraise.hpp"

void write_sbatch(std::ofstream& sbatchFile, const CunqaArgs& args);