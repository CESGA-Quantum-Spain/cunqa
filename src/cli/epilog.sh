#!/bin/bash

SLURM_TASK_ID="${SLURM_JOB_ID}_${SLURM_LOCALID}"
/mnt/netapp1/Optcesga_FT2_RHEL7/2020/gentoo/22072020/usr/bin/jq --arg SLURM_TASK_ID "$SLURM_TASK_ID" 'del(.[$SLURM_TASK_ID])' $INFO_PATH > tmp_qpu.json && mv tmp_qpu.json $INFO_PATH
