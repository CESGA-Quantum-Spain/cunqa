import os, sys
import numpy as np

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

# 1. Deploy vQPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
family = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, co_located = True)
qpus   = get_QPUs(co_located = True, family = family)

# 2. Design circuits with distributed instructions between them
cc_1 = CunqaCircuit(1, id="First")
cc_2 = CunqaCircuit(1, id="Second")

cc_1.h(0)

with cc_1.expose(0, cc_2) as (rqubit, subcircuit):
    subcircuit.cx(rqubit,0)

cc_1.measure_all()
cc_2.measure_all()

# 3. Execute distributed circuits on QPUs with quantum communications
distr_jobs = run([cc_1, cc_2], qpus, shots=1024) 

# Collect the results
result_list = gather(distr_jobs)

# Print the counts
for i, result in enumerate(result_list):
    print(f"Counts {i} is {result.counts}")

# 4. Relinquish resources: drop the deployed QPUs 
qdrop(family)