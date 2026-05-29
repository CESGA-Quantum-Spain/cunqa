#!/bin/bash
#SBATCH --job-name=qraise 
#SBATCH --ntasks=4
#SBATCH -c 2
#SBATCH -N 1
#SBATCH --mem=60G
#SBATCH --time=00:10:00
#SBATCH --output=qraise_%j

unset SLURM_MEM_PER_CPU SLURM_MEM_PER_NODE SLURM_CPU_BIND_LIST SLURM_CPU_BIND
EPILOG_PATH=/home/cesga/jvazquez/bin/epilog.sh
srun --task-epilog=$EPILOG_PATH setup_qpus co_located nc default Aer '{"noise_properties_path":"/mnt/netapp1/Store_CESGA/home/cesga/jvazquez/works/cunqa/examples/no_comm/noise_properties_example.json","thermal_relaxation":"1","readout_error":"1","gate_error":"1","fakeqmio":"0"}'

