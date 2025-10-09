#!/bin/bash
#SBATCH -c 64
#SBATCH -t 10:00:00
#SBATCH --mem 16000

mode=${SLURM_JOB_NAME}

ml load qmio/hpc gcc/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 pybind11/2.12.0-python-3.9.9 nlohmann_json/3.11.3 ninja/1.9.0 qiskit/1.2.4-python-3.9.9
ml load qmio-tools/0.2.0-python-3.9.9

OMP_NUM_THREADS=64

python -u run-vqe-${mode}.py