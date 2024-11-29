module load cesga/2022 gcc/system gcccore/system openmpi/4.1.4 scalapack/2.2.0 openblas/system
 
AER_INCL_DIR=/mnt/netapp1/Store_CESGA/home/cesga/jvazquez/works/api-simulator/installation/include/aer-cpp/src
PYBIND_INCL_DIR=/mnt/netapp1/Store_CESGA/home/cesga/jvazquez/conda/envs/qiskit-aer/include/pybind11
PYTHON_INCL_DIR=/mnt/netapp1/Store_CESGA/home/cesga/jvazquez/conda/envs/qiskit-aer/include/python3.9
PYTHON_LIB=/mnt/netapp1/Store_CESGA/home/cesga/jvazquez/conda/envs/qiskit-aer/lib
API_INCLUDES=/mnt/netapp1/Store_CESGA/home/cesga/jvazquez/works/api-simulator/installation/include
CUSTOM_JSON_LIB=/mnt/netapp1/Store_CESGA/home/cesga/jvazquez/works/api-simulator/installation/lib


export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PYTHON_LIB:$CUSTOM_JSON_LIB


g++ -std=c++17 pybind_test.cpp -o main.out -I$AER_INCL_DIR -I$PYBIND_INCL_DIR -I$PYTHON_INCL_DIR -I$API_INCLUDES -L$CUSTOM_JSON_LIB -L$PYTHON_LIB -fopenmp -llapack -lblas -fPIC -lpython3.9 -lmpi -lcustom-json