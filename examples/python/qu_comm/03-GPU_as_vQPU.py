"""
Currently, the GPU option is only available to the GPU architecture at CESGA.
For executing using GPUs, CUNQA must be compiled with the cmake flag -DAER_GPU=TRUE,
and with the specific GPU architecture. Check the installtion guide for details:
https://cesga-quantum-spain.github.io/cunqa/getting_started.html
"""
import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

# 1. Deploy vQPUs and retrieve them using get_QPUs
# The number of cores must match the ones given in the warning at https://cesga-docs.gitlab.io/ft3-user-guide/gpu_nodes.html#nvidia-a100
# Number of cores should be modified if more QPUs are requested
family = qraise(3, "00:10:00", cores = 8, simulator="Aer", quantum_comm=True, co_located = True, gpu = True)
qpus   = get_QPUs(on_node=False)

# 2. Design circuits with distributed instructions between them
circuit1 = CunqaCircuit(2, id = "circuit1") 
circuit2 = CunqaCircuit(1, id = "circuit2")
circuit3 = CunqaCircuit(1, id = "circuit3") # blank circuit to match QPU number

circuit1.h(0)
circuit1.cx(0,1)
circuit1.qsend(1, "circuit2")# this qubit that is sent is reset
circuit2.qrecv(0, "circuit1")

circuit1.measure_all()
circuit2.measure_all()

# 3. Execute distributed circuits on QPUs with quantum communications and GPU enabled
qjobs = run([circuit1, circuit2, circuit3], qpus, shots = 100)

# Collect the results
results = gather(qjobs)

# Print the counts
for result in [results[0], results[1]]:
    print(f"Counts is {result.counts}")

# 4. Relinquish resources: drop the deployed QPUs 
qdrop(family)