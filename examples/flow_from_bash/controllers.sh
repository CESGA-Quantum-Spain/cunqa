#!/bin/bash

# check_resources ${n_qpus} ${cores_per_qpu} ${mem_per_core}
check_resources() {
    if [ $1 -gt $SLURM_NTASKS ]; then
        echo "Number of srun tasks greater than allocated tasks"
        exit 1
    elif [ $2 -gt $SLURM_CPUS_PER_TASK ]; then
        echo "More cores per QPU than allocated per task"
        exit 1
    elif [ $3 -gt $SLURM_MEM_PER_CPU ]; then
        echo "More memory per core than allocated"
        exit 1
    fi
    
    echo "Resources checked."
}

#check_total_resources ${total_qpus} ${total_cores} ${total_memory}
check_total_resources() {
    if [ $1 -gt $SLURM_NTASKS ]; then
        echo "More QPUs than allocated tasks"
        exit 1
    elif [ $2 -gt $((${SLURM_CPUS_PER_TASK}*${SLURM_NTASKS})) ]; then
        echo "More cores than allocated per task"
        exit 1
    elif [ $3 -gt $((${SLURM_MEM_PER_CPU}*${SLURM_CPUS_PER_TASK}*${SLURM_NTASKS})) ]; then
        echo "More memory than allocated"
        exit 1
    fi
    
    echo "Total resources checked."
}

check_info_json() {

    total_qpus=$1

    sleep 30

    for ((i=1; i<=${total_qpus}; i++))
    do
        key_count=$(jq 'keys | length' "${INFO_PATH}")
        if [ ${key_count} -gt ${total_qpus} ]; then
            echo "Problem writing on the information file. More keys than QPUS"
            return 1
        elif [ ${key_count} -lt ${total_qpus} ]; then
            echo "Information file not ready. Keep waiting..."
            sleep 10
        elif [ ${key_count} -eq ${total_qpus} ]; then
            echo "Information file is ready."
            return 0
        fi
    done

    echo "The waiting time limit has been reached."
    return 1
}