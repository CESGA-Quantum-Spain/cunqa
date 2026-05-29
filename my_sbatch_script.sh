#!/bin/bash
#SBATCH --job-name=qraise 
#SBATCH --ntasks=3
#SBATCH -c 2
#SBATCH -N 1
#SBATCH --mem=30G
#SBATCH --time=00:10:00
#SBATCH --output=qraise_%j

unset SLURM_MEM_PER_CPU SLURM_MEM_PER_NODE SLURM_CPU_BIND_LIST SLURM_CPU_BIND
EPILOG_PATH=/home/cesga/jvazquez/bin/epilog.sh
srun --exclusive -n 2 -c 1 --mem-per-cpu=1G --task-epilog=$EPILOG_PATH setup_qpus co_located qc default Aer &
srun --exclusive -n 1 -c 2 --mem=28G setup_executor Aer 2
