#!/bin/bash


#SBATCH -J qraise_from_slurm            # Job name
#SBATCH -o qraise_from_slurm_%j.o       # Name of stdout output file(%j expands to jobId)
#SBATCH -e qraise_from_slurm_%j.e       # Name of stderr output file(%j expands to jobId)
#SBATCH -n 10                           # Number of total QPUS
#SBATCH -N 1
#SBATCH -c 4                            # Cores per QPU (the maximum between the different QPU types)
#SBATCH --time 00:30:00                 # Run time (hh:mm:ss) 
#SBATCH --mem-per-cpu=14G               # Memory per core (the maximum between the different QPU types) 

# Loading modules
ml load qmio/hpc gcc/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 pybind11/2.12.0-python-3.9.9 nlohmann_json/3.11.3 ninja/1.9.0 qiskit/1.2.4-python-3.9.9

# Import auxiliar functions
source functions.sh

# Installation PATHs
export INSTALL_PATH=/mnt/netapp1/Store_CESGA/home/cesga/acarballido/repos/api-simulator/installation
export PATH=$PATH:$INSTALL_PATH/bin

# Where QPUs information will be saved 
export INFO_PATH=$STORE/.cunqa/qpus.json


##################### First QPUs group ##########################
n_qpus_0=2
cores_per_qpu_0=2
mem_per_qpu_0=20
number_of_nodes_0=1
simulator_0="Aer"
# path_to_backend =
mem_per_core_0=$((${mem_per_qpu_0}/${cores_per_qpu_0})) 
# Check resources:
check_resources ${n_qpus_0} ${cores_per_qpu_0} ${mem_per_core_0}


##################### Second QPUs group ##########################
n_qpus_1=4
cores_per_qpu_1=4
mem_per_qpu_1=12 
number_of_nodes_1=1
simulator_1="Munich"
# path_to_backend =
mem_per_core_1=$((${mem_per_qpu_1}/${cores_per_qpu_1}))
# Check resources:
check_resources ${n_qpus_1} ${cores_per_qpu_1} ${mem_per_core_1}


##################### Third QPUs group ##########################
n_qpus_2=3
cores_per_qpu_2=3
mem_per_qpu_2=30
number_of_nodes_2=1
fakeqmio="fakeqmio"
mem_per_core_2=$((${mem_per_qpu_2}/${cores_per_qpu_2}))
# Check resources:
check_resources ${n_qpus_2} ${cores_per_qpu_2} ${mem_per_core_2}



################### Check total resources ########################
total_qpus=$((${n_qpus_0} + ${n_qpus_1} + ${n_qpus_2})) 
total_cores=$((${n_qpus_0}*${cores_per_qpu_0} + ${n_qpus_1}*${cores_per_qpu_1} + ${n_qpus_2}*${cores_per_qpu_2}))
total_memory=$((${mem_per_core_0}*${n_qpus_0}*${cores_per_qpu_0} + ${mem_per_core_1}*${n_qpus_1}*${cores_per_qpu_1} + ${mem_per_core_2}*${n_qpus_2}*${cores_per_qpu_2}))

check_total_resources ${total_qpus} ${total_cores} ${total_memory}

###################################################################



#srun -n ${n_qpus_0} -N ${number_of_nodes_0} -c ${cores_per_qpu_0} --mem-per-cpu="${mem_per_core_0}G" --task-epilog=$INSTALL_PATH/bin/epilog.sh setup_qpus ${INFO_PATH} ${simulator_0} &

#echo "First srun passed"

#srun -n ${n_qpus_1} -N ${number_of_nodes_1} -c ${cores_per_qpu_1} --mem-per-cpu="${mem_per_core_1}G" --task-epilog=$INSTALL_PATH/bin/epilog.sh setup_qpus ${INFO_PATH} ${simulator_1} &

#echo "Second srun passed"

#srun -n ${n_qpus_2} -c ${cores_per_qpu_2} --mem-per-cpu="${mem_per_core_2}G" -N ${number_of_nodes_2} --task-epilog=$INSTALL_PATH/bin/epilog.sh setup_qpus ${INFO_PATH} ${simulator_0} &

#echo "Third srun passed"

check_info_json ${total_qpus} # Uses jq



wait


            