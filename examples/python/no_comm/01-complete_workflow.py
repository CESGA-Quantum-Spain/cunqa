import os, sys
from time import sleep

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import qraise, get_QPUs, run, qdrop
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
# If GPU execution is desired, just add "gpu = True" as another qraise argument
family = qraise(2, "00:10:00", simulator = "Aer", co_located = True)

qpus  = get_QPUs(co_located = True, family = family)

qc = CunqaCircuit(5)
qc.h(0)
qc.cx(0, 1)
qc.measure_all()

qjobs = run([qc, qc], qpus, shots = 10) # non-blocking call


results = gather(qjobs)

# Getting the counts
counts_list = [result.counts for result in results]

# Printing the counts
for counts in counts_list:
    print(f"Counts: {counts}")

########## Drop the deployed QPUs ###########
qdrop(family)
