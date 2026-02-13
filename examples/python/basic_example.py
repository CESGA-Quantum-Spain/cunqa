import os, sys

# Adding path to access CUNQA module
sys.path.append(os.getenv("HOME"))

# Let's get the raised QPUs
from cunqa.qpu import get_QPUs

# List of deployed vQPUs
qpus  = get_QPUs(co_located=True)

# Let's create a circuit to run in our QPUs
from cunqa.circuit import CunqaCircuit

qc = CunqaCircuit(num_qubits = 2)
qc.h(0)
qc.cx(0,1)
qc.measure_all()

qcs = [qc] * 4

# Submitting the same circuit to all vQPUs
from cunqa.qpu import run

qjobs = run(qcs , qpus, shots = 1000)

# Gathering results
from cunqa.qjob import gather

results = gather(qjobs)

# Getting the counts
counts_list = [result.counts for result in results]

# Printing the counts
for counts in counts_list:
    print(f"Counts: {counts}" ) # Format: {'00':546, '11':454}
