#!/bin/bash

#SBATCH -J wake_up_c7_18    # Job name
#SBATCH -n 1                # Number of processes
#SBATCH -c 1                # Cores per task requested
#SBATCH -t 00:10:00         # Run time (hh:mm:ss) 
#SBATCH --mem-per-cpu=6G    # Memory per core demanded
#SBATCH -w c7-18    # Specific node (also -w ) 

echo "I'm awake! :)"