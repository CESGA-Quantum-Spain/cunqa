#!/bin/bash

IDLE_NODES=$(sinfo -h -N -R -t idle | awk '{print $1}') # Extract the name of all idle nodes
filtered_nodes=$(echo "$IDLE_NODES" | awk -F'-' '$2 > 3 && $2 < 23 {print $0}') # Select only the nodes between c7-4 and c7-22

# Perform a random shuffle of the selected nodes
shuffled_nodes=$(echo $filtered_nodes | awk 'BEGIN{srand()} {n=split($0,a); for(i=n;i>0;i--) {j=int(rand()*i)+1; t=a[i]; a[i]=a[j]; a[j]=t}} {for(i=1;i<=n;i++) printf "%s ", a[i]; print ""}') 


# Here we deploy QPUs in some nodes 
counter=0
for node in $shuffled_nodes; do
    qraise -n 32 -t 00:00:01 --cloud --node_list $node --family=$node

    ((counter++))
    if [ $counter -eq 6 ]; then
        break
    fi
done


# Wait until the resources are deployed to continue execution
sleep 1                                            # Necessary in order to give the jobs time to enter the queue

while (( $(squeue -t CF -h | wc -l) != 0 )); do    # While there are jobs configuring (CF), we wait
    sleep 1
done
