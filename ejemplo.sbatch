#!/bin/bash

#SBATCH --job-name=qraise 
#SBATCH --ntasks=3
#SBATCH -c 2
#SBATCH -N 1
#SBATCH --partition=a64
#SBATCH --mem-per-cpu=4G
#SBATCH --time=00:00:15
#SBATCH --output=qraise_%j



echo "SLURM_CPUS_PER_TASK: $SLURM_CPUS_PER_TASK"
echo "SLURM_TRES_PER_TASK: $SLURM_TRES_PER_TASK"
hostname
sleep 10
echo "Fin."
