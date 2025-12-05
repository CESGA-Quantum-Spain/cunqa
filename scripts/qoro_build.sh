#!/bin/bash

# Needed modules
ml load qmio/hpc gcc/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 gcccore/12.3.0 nlohmann_json/3.11.3 eigen/5.0.0 ninja/1.9.0 pybind11/2.13.6-python-3.11.9 qiskit/1.2.4-python-3.11.9

# Update the submodules. Including maestro and QCSim
git submodule update --init --recursive

cmake -B build/ 
cmake --build build/ --parallel $(nproc)
cmake --install build/

echo "Success. Try: 
1) Open a new shell and load the modules above 
2) Run: python examples/python/simple_examples/example_qraise.py"


