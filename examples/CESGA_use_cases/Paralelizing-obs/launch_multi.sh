#!/bin/bash
ml load qmio/hpc gcc/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 pybind11/2.12.0-python-3.9.9 nlohmann_json/3.11.3 ninja/1.9.0 qiskit/1.2.4-python-3.9.9
ml load qmio-tools/0.2.0-python-3.9.9
mode=$1
cores_per_qpu=$2
repetitions=16

if [ "$mode" == "cunqa" ]; then

    for ((n=12; n<=repetitions; n++)); do

        echo "Raising 3 qpus with $cores_per_qpu cores each"

        total=$((3 * $cores_per_qpu))

        echo $total

        qraise -n 3 -t 90:00:00 -c $cores_per_qpu --cloud --fakeqmio=/opt/cesga/qmio/hpc/calibrations/2025_04_02__12_00_02.json --family_name=cunqa-$n-$cores_per_qpu

        sleep 30

        sbatch --job-name="$mode-$n" \
            --output="./logs/$mode-$n-%j.out" \
            --error="./logs/$mode-$n-%j.err" \
            --time=90:00:00 \
            -c  $total\
            --mem-per-cpu=4G \
            --wrap="OMP_NUM_THREADS=1 python -u run-vqe-$mode.py --num_qubits \"$n\" --cores \"$cores_per_qpu\""
        
        sleep 15
    done

else

    for ((n=2; n<=repetitions; n++)); do

        total=$((3 * $cores_per_qpu))

        sbatch --job-name="$mode$n" \
               --output="./logs/$mode-$n-%j.out" \
               --error="./logs/$mode-$n-%j.err" \
               --time=05:00:00 \
               -c $total \
               --mem-per-cpu=8G \
               --wrap="OMP_NUM_THREADS=$total python -u run-vqe-$mode.py --num_qubits \"$n\" --cores \"$cores_per_qpu\""
        
        sleep 15
    done

fi