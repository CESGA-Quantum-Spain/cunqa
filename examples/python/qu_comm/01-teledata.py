import os, sys

# Append path to access CUNQA installation
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
family = qraise(2, "00:10:00", simulator="Maestro", quantum_comm=True, co_located = True)
qpus   = get_QPUs(co_located=True, family = family)

########## Circuits to run ##########
# First circuit 
cc_1 = CunqaCircuit(1, 1, id="First")
cc_1.h(0)
cc_1.qsend(qubit = 0, recving_circuit = "Second")
cc_1.measure(0,0)

# Second circuit 
cc_2 = CunqaCircuit(2, 2, id="Second")
cc_2.qrecv(qubit = 0, control_circuit = "First")
cc_2.cx(0, 1)
cc_2.measure(0,0)
cc_2.measure(1,1)

########## Distributed run ##########
distr_jobs = run([cc_1, cc_2], qpus, shots=1024)

# Collect the results
result_list = gather(distr_jobs)

# Print the counts
for i, result in enumerate(result_list):
    print(f"Counts {i} is {result.counts}")

# Drop the deployed QPUs #
qdrop(family)
