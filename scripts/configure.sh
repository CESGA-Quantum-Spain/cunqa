#!/bin/bash

if [ ! -z "$1" ]; then
    cd ../

    ml load cesga/2022 gcc/system gcccore/system openmpi/4.1.4 flexiblas/3.3.0 boost python/3.10.8 pybind11/2.12.0 cmake/3.27.6
    conda deactivate
    
    export INSTALL_PATH=$1
    export PATH=$PATH:$INSTALL_PATH/bin

    cmake -B build/
    cmake --build build/
    cmake --install build/
else
    echo "No install path provided. Please provide an argument." >&2
fi