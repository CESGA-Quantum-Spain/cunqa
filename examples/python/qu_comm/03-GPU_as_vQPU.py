import os, sys

# Append path to access CUNQA installation
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

family = qraise(3, "00:10:00", cores = 8, simulator="Aer", quantum_comm=True, co_located = True, gpu = True)
qpus   = get_QPUs(on_node=False)

########## Circuits to run ##########
circuit1 = CunqaCircuit(2, id = "circuit1") 
circuit2 = CunqaCircuit(1, id = "circuit2")
circuit3 = CunqaCircuit(1, id = "circuit3") # Only to match the number of vQPUs

circuit1.h(0)
circuit1.cx(0,1)
circuit1.qsend(1, "circuit2")# this qubit that is sent is reset
circuit2.qrecv(0, "circuit1")

circuit1.measure_all()
circuit2.measure_all()

########## Distributed run ##########
qjobs = run([circuit1, circuit2, circuit3], qpus, shots = 100)

# Collect the results
results = gather(qjobs)

# Print the counts
for result in [results[0], results[1]]:
    print(f"Counts is {result.counts}")

# Drop the deployed QPUs #
qdrop(family)