#!/bin/bash
#SBATCH --job-name=qraise 
#SBATCH --ntasks=8
#SBATCH -c 2
#SBATCH -N 1
#SBATCH --mem=120G
#SBATCH --time=00:10:00
#SBATCH --output=qraise_%j

unset SLURM_MEM_PER_CPU SLURM_MEM_PER_NODE SLURM_CPU_BIND_LIST SLURM_CPU_BIND
EPILOG_PATH=/home/cesga/jvazquez/bin/epilog.sh
srun --task-epilog=$EPILOG_PATH setup_qpus co_located cc default Aer
