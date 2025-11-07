#!/bin/bash

JOB_ID="${SLURM_JOB_ID}"

flock $INFO_PATH -c "jq $SLURM_JOB_ID $INFO_PATH"

if compgen -G "$STORE/.cunqa/communications.json" > /dev/null; then
    flock $COMM_PATH -c "jq $SLURM_JOB_ID $COMM_PATH"
fi

if compgen -G "$STORE/.cunqa/tmp_noisy_backend_$SLURM_JOB_ID.json" > /dev/null; then
    rm $STORE/.cunqa/tmp_noisy_backend_$SLURM_JOB_ID.json
fi

