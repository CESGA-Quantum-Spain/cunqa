#!/bin/bash

json_file="$STORE/.cunqa/qpus.json"
for i in {1..1000}
do
    qraise_output=$(qraise -n 20 -t 00:10:00)
    qraise_line=$(echo "$qraise_output" | grep "Submitted batch job")
    qraise_id=$(echo "$qraise_line" | awk '{print $4}')

    qraise_state=$(squeue -j "$qraise_id" -h -o "%T")
    echo "Qraise $qraise_id is $qraise_state"
    while [ "${qraise_state// /}" != "RUNNING" ]; do
        sleep 1
        qraise_state=$(squeue -j "$qraise_id" -h -o "%T")
    done
    echo "Qraise $qraise_id is $qraise_state"   

    job_output=$(sbatch -t 00:02:00 --mem 1G execute_example.sbatch 2>&1)
    job_line=$(echo "$job_output" | grep "Submitted batch job")
    job_id=$(echo "$job_line" | awk '{print $4}')
    job_state=$(squeue -j "$job_id" -h -o "%T")
    echo "Job $job_id is $job_state"
    
    while [ "${job_state// /}" != "RUNNING" ]; do
        if [ "${job_state// /}" = "COMPLETED" ]; then
            break
        fi
        sleep 1
        job_state=$(sacct -j "$job_id" --format=State --noheader | head -n 1)
    done
    echo "Job $job_id is $job_state"
    if grep -q "bus error" qraise_$qraise_id; then
        echo "Bus error found"
        exit
    else
        echo "No bus error found"
    fi

    qdrop --all
    
    while [ ! "$(jq -r 'length' "$json_file")" -eq 0 ]; do
        sleep 1
    done
done