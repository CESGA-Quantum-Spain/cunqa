#!/bin/bash

CUNQA_PATH="${STORE}/.cunqa"
QPUS_FILEPATH="${CUNQA_PATH}/qpus.json"
COMM_FILEPATH="${CUNQA_PATH}/communications.json"

erase_key $SLURM_JOB_ID $QPUS_FILEPATH
if compgen -G $COMM_FILEPATH > /dev/null; then
    erase_key $SLURM_JOB_ID $COMM_FILEPATH
fi
