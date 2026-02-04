#!/bin/bash

if [ $LMOD_SYSTEM_NAME == "QMIO" ]; then
    # Execution for QMIO
    ml load qmio/hpc gcc/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 gcccore/12.3.0 eigen/5.0.0 nlohmann_json/3.11.3 ninja/1.9.0 pybind11/2.13.6-python-3.11.9 qiskit/1.2.4-python-3.11.9
    conda deactivate
elif [ $LMOD_SYSTEM_NAME == "FT3" ]; then
    # Execution for FT3 
    #-----------------------------------------------------------------
    #--------------------MODULES FOR BASIC COMPILATION--------------------
    #-----------------------------------------------------------------
    ml load cesga/2022 gcc/system gcccore/system cmake boost openmpi/5.0.5 cython/3.0.11 pybind11/2.12.0 qiskit/1.2.4

    #-----------------------------------------------------------------
    #--------------------MODULES FOR GPU COMPILATION--------------------
    #-----------------------------------------------------------------
    #ml load cesga/2022 gcc/system gcccore/system cmake boost openmpi/5.0.5 cython/3.0.11 pybind11/2.12.0 qiskit/1.2.4 cuda/12.8.0

    conda deactivate
else
    echo "You need to specify the modules for your cluster"
    # PUT YOUR MODULES HERE
fi

if [ -n "$1" ]; then
    cmake -B build/ -DCMAKE_INSTALL_PREFIX=$1 #-DAER_GPU=TRUE
else
    cmake -B build/ #-DAER_GPU=TRUE
fi
cmake --build build/ --parallel $(nproc)
cmake --install build/
