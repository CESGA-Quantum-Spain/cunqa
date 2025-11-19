#!/bin/bash

if [ "$#" -gt 2 ]; then
    echo "Usage: source configure.sh [--ninja] [install_path]"
    echo "Arguments:"
    echo "  --ninja       (Optional) Use Ninja build system generator for faster compilation."
    echo "  install_path  (Optional) Destination directory for the installation."
    exit 1
fi

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
    echo "Error: It seems that your system is neither QMIO nor FT3; please consider checking configure.sh to specify the necessary modules for your system."
    exit 1 #REMOVE THIS IF YOU ADDED THE NECESSARY MODULES
fi

USE_NINJA=false
INSTALL_PREFIX=""

for arg in "$@"; do
    if [ "$arg" == "--ninja" ]; then
        USE_NINJA=true
        echo "Ninja Mode: ENABLED"
    else
        INSTALL_PREFIX="$arg" # If argument is not the flag, assume it is the installation path
    fi
done

if [ "$USE_NINJA" = true ]; then
  cmake -G Ninja -B build/ -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
  ninja -C build -j $(nproc)
  cmake --install build/
else
  cmake -B build/ -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
  cmake --build build/ --parallel $(nproc)
  cmake --install build/
fi

