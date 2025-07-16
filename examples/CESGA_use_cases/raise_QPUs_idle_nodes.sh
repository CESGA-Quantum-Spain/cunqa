#!/bin/bash

IDLE_NODES=$(sinfo -h -N -R -t idle | awk '{print $1}')
filtered_nodes=$(echo "$IDLE_NODES" | awk -F'-' '$2 > 3 && $2 < 23 {print $0}')

counter=0
for node in $filtered_nodes; do
    qraise -n 32 -t 00:00:01 --cloud --node_list $node --family=$node

    ((counter++))
    if [ $counter -eq 6 ]; then
        break
    fi
done
