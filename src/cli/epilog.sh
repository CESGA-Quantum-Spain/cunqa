#!/bin/bash

JOB_ID="${SLURM_JOB_ID}"

flock $INFO_PATH -c "/mnt/netapp1/Optcesga_FT2_RHEL7/2020/gentoo/22072020/usr/bin/jq --arg job_id \"$SLURM_JOB_ID\" 'with_entries(select(.key | startswith(\$job_id) | not))' $INFO_PATH > tmp_info.json && mv tmp_info.json $INFO_PATH"

if compgen -G "$STORE/.cunqa/communications.json" > /dev/null; then
    flock $COMM_PATH -c "/mnt/netapp1/Optcesga_FT2_RHEL7/2020/gentoo/22072020/usr/bin/jq --arg job_id \"$SLURM_JOB_ID\" 'with_entries(select(.key | startswith(\$job_id) | not))' $COMM_PATH > tmp_comm.json && mv tmp_comm.json $COMM_PATH"
fi

if compgen -G "$STORE/.cunqa/tmp_noisy_backend_$SLURM_JOB_ID.json" > /dev/null; then
    rm $STORE/.cunqa/tmp_noisy_backend_$SLURM_JOB_ID.json
fi

