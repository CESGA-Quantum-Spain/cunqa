#!/bin/bash

jq --arg SLURM_LOCALID "$SLURM_LOCALID" 'del(.[$SLURM_LOCALID])' $INFO_PATH > tmp_qpu.json && mv tmp_qpu.json $INFO_PATH
