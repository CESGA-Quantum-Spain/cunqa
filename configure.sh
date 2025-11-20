#!/bin/bash

if [ $LMOD_SYSTEM_NAME == "QMIO" ]; then
    # Execution for QMIO
    ml load qmio/hpc gcc/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 gcccore/12.3.0 nlohmann_json/3.11.3 ninja/1.9.0 pybind11/2.13.6-python-3.11.9 qiskit/1.2.4-python-3.11.9
    conda deactivate
elif [ $LMOD_SYSTEM_NAME == "FT3" ]; then
    # Execution for FT3 
    ml load cesga/2022 gcc/system flexiblas/3.3.0 openmpi/5.0.5 boost pybind11 cmake qiskit/1.2.4
    conda deactivate
else
    # PUT YOUR MODULES HERE
fi

cmake -B build/ -DCMAKE_INSTALL_PREFIX=$1
cmake --build build/ --parallel $(nproc)
cmake --install build/