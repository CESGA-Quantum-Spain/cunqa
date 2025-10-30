#!/bin/bash
#SBATCH --job-name=qraise 
#SBATCH -c 64
#SBATCH --ntasks=1
#SBATCH -N 1
#SBATCH --mem-per-cpu=15G
#SBATCH --time=00:30:00
#SBATCH --output=qraise_%j

ml load qmio/hpc gcc/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 gcccore/12.3.0 nlohmann_json/3.11.3 ninja/1.9.0 pybind11/2.13.6-python-3.11.9 qiskit/1.2.4-python-3.11.9
srun python tests.py