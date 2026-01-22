#!/bin/bash
ml load qmio/hpc gcc/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 gcccore/12.3.0 eigen/5.0.0 nlohmann_json/3.11.3 ninja/1.9.0 pybind11/2.13.6-python-3.11.9 qiskit/1.2.4-python-3.11.9
export INSTALL_PATH=/mnt/netapp1/Store_CESGA/home/cesga/dexposito/repos/CUNQA/installation
export PATH=$PATH:$INSTALL_PATH/bin