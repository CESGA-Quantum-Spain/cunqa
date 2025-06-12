#!/bin/bash

JOB_ID="${SLURM_JOB_ID}"

flock $INFO_PATH -c "/mnt/netapp1/Optcesga_FT2_RHEL7/2020/gentoo/22072020/usr/bin/jq --arg job_id \"$SLURM_JOB_ID\" 'with_entries(select(.key | startswith(\$job_id) | not))' $INFO_PATH > tmp.json && mv tmp.json $INFO_PATH"

if compgen -G "$STORE/.cunqa/communications.json" > /dev/null; then
    flock $COMM_PATH -c "/mnt/netapp1/Optcesga_FT2_RHEL7/2020/gentoo/22072020/usr/bin/jq --arg job_id \"$SLURM_JOB_ID\" 'with_entries(select(.key | startswith(\$job_id) | not))' $COMM_PATH > comm_tmp.json && mv comm_tmp.json $COMM_PATH"
fi

if compgen -G "$STORE/.cunqa/tmp_fakeqmio_backend_$SLURM_JOB_ID.json" > /dev/null; then
    rm $STORE/.cunqa/tmp_fakeqmio_backend_$SLURM_JOB_ID.json
fi

