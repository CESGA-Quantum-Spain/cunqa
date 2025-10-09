#!/bin/bash
ml load qmio/hpc gcc/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 pybind11/2.12.0-python-3.9.9 nlohmann_json/3.11.3 ninja/1.9.0 qiskit/1.2.4-python-3.9.9
ml load qmio-tools/0.2.0-python-3.9.9
mode=$1
cores=2
repetitions=128

n=8

if [ "$mode" == "cunqa" ]; then

    for qpu in 1 2 4 8 16 32 64; do

        qraise -n $qpu -t 91:00:00 -c $cores --cloud --fakeqmio=/opt/cesga/qmio/hpc/calibrations/2025_04_02__12_00_02.json --family_name=cunqa-$qpu

        num_cores_total=$(($qpu * $cores))

        sleep 30

        sbatch --job-name="$mode$qpu" \
            --output="./logs/$mode-$qpu-%j.out" \
            --error="./logs/$mode-$qpu-%j.err" \
            --time=90:00:00 \
            -c $qpu \
            --mem-per-cpu=8G \
            --wrap="OMP_NUM_THREADS=1 python -u run-vqe-$mode.py --num_qubits \"$n\" --cores \"$cores\" --qpus \"$qpu\""
        
        sleep 15
    done

else

    for ((n=2; n<=repetitions; n++)); do
        num_params=$((n * 4))

        echo $num_params

        num_cores_total=$(($num_params * $cores))

        sbatch --job-name="$mode$n" \
               --output="./logs/$mode-$n-%j.out" \
               --error="./logs/$mode-$n-%j.err" \
               --time=05:00:00 \
               -c $num_cores_total \
               --mem-per-cpu=8G \
               --wrap="python -u run-vqe-$mode.py --num_qubits \"$n\" --cores \"$cores\""
        
        sleep 15
    done

fi