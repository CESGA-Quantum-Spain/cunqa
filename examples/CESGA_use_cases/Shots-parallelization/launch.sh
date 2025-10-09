#!/bin/bash
ml load qmio/hpc gcc/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 pybind11/2.12.0-python-3.9.9 nlohmann_json/3.11.3 ninja/1.9.0 qiskit/1.2.4-python-3.9.9
ml load qmio-tools/0.2.0-python-3.9.9

if [ -z "$1" ]; then
    echo "Error: Debes especificar el modo como primer argumento."
    echo "Uso: $0 <modo>"
    exit 1
fi
mode=$1
repetitions=128
cores_per_qpu=4
mem=$(($cores_per_qpu * 15))

for ((n=1; n<=repetitions; n++)); do

    qraise -n $n -t 20:00:00 -c $cores_per_qpu --mem-per-qpu=${mem}G --cloud  --family_name=$mode-$n

    sleep 15

    sbatch --job-name="$mode-$n" \
        --output="./logs/$mode-$n-%j.out" \
        --error="./logs/$mode-$n-%j.err" \
        -t 20:00:00 \
        -c 2\
        --mem-per-cpu 4000 \
        --wrap="python -u $mode.py --num_qpus \"$n\""
    
    sleep 15
done