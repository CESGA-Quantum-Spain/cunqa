#!/bin/bash

if [ "$LMOD_SYSTEM_NAME" == "QMIO" ]; then
    ml load qmio/hpc gcc/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 gcccore/12.3.0 nlohmann_json/3.11.3 ninja/1.9.0 
pybind11/2.13.6-python-3.11.9 qiskit/1.2.4-python-3.11.9
    conda deactivate

elif [ $LMOD_SYSTEM_NAME == "FT3" ]
    # Execution for FT3 
    ml load cesga/2022 gcc/system flexiblas/3.3.0 openmpi/5.0.5 boost pybind11 cmake qiskit/1.2.4
    conda deactivate

else   
    #LUSITANIA
    module purge

    module load gcc/gcc-11.2.0 cmake/cmake-3.23 openblas/openblas-0.3.24 openmpi/openmpi-4.1.2-gcc11.2.0 python/python-3.10
    
    export STORE=$HOME
    export CC=mpicc
    export CXX=mpicxx
    export CMAKE_PREFIX_PATH="/lusitania_apps/openblas-0.3.24:$CMAKE_PREFIX_PATH"
    export BLA_VENDOR=OpenBLAS
    PYBIND_PATH=$(python3 -c "import pybind11; print(pybind11.get_cmake_dir())")
    
    if [ -z "$PYBIND_PATH" ]; then
        echo "Error: No se pudo encontrar pybind11. Asegúrate de ejecutar: pip install --user pybind11"
        exit 1
    fi
    
    export pybind11_DIR=$PYBIND_PATH
    echo "pybind11 encontrado en: $pybind11_DIR"
fi

rm -rf build/
cmake -S . -B build/ 
cmake --build build/ --parallel $(nproc)
