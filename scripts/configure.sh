#!/bin/bash

if [ ! -z "$1" ]; then
    cd ../

    # Execution for FT3 
    #ml load cesga/2022 gcc/system gcccore/system openmpi/4.1.4 flexiblas/3.3.0 boost python/3.10.8 pybind11/2.12.0 cmake/3.27.6
    
    # Execution for QMIO
    ml load qmio/hpc gcc/system gcccore/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 pybind11/2.13.6-python-3.11.9 nlohmann_json/3.11.3

    conda deactivate
    
    export INSTALL_PATH=$1
    export PATH=$PATH:$INSTALL_PATH/bin

    # Execution for FT3
    #cmake -B build/ -DPYBIND_PATH=/opt/cesga/2022/software/Compiler/gcccore/system/pybind11/2.12.0/lib/python3.9/site-packages/pybind11/share/cmake/pybind11

    # Execution for QMIO
    cmake -B build/
    
    cmake --build build/
    cmake --install build/
else
    echo "No install path provided. Please provide an argument." >&2
fi