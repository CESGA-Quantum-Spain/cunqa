#!/bin/bash
# Removes the registry entries of the job (see src/utils/registry.hpp)

CUNQA_PATH="${STORE}/.cunqa"

for qpu_file in "${CUNQA_PATH}"/qpus/*/"${SLURM_JOB_ID}"_*.json; do
    [ -e "$qpu_file" ] || continue
    rm -f "$qpu_file"
    # Only removed once the family has no vQPUs left
    rmdir "$(dirname "$qpu_file")" 2>/dev/null
done

rm -rf "${CUNQA_PATH}/communications/${SLURM_JOB_ID}"
