import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.transformations import union

# ---------------------------
# Acquiring resources
# ---------------------------
family_separated = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, co_located=True)
qpus_separated = get_QPUs(co_located=True, family=family_separated)

family_union = qraise(1, "00:10:00", simulator="Aer", co_located=True)
[qpu_union]  = get_QPUs(co_located=True, family=family_union)

# ---------------------------
# Original circuits created and executed
# circuit1.q0: ─[H]──●──[M]─
#                    $
# circuit2.q0: ─────[X]─[M]─
# Where $ represents the remote control of the gate
# ---------------------------
circuit1 = CunqaCircuit(1, id="circuit1") # adding ancilla
circuit1.h(0)

circuit2 = CunqaCircuit(1, id="circuit2")

with circuit1.expose(0, circuit2) as (rqubit, subcircuit):
    subcircuit.cx(rqubit,0)

circuit1.measure_all()
circuit2.measure_all()

qjobs = run([circuit1, circuit2], qpus_separated, shots=1024)
results = gather(qjobs)

print(f"\nResult before union: {results[0].counts}") # taking only the results from the 0 circuit
                                                     # because they both get the same due to 
                                                     # quantum communications

# ---------------------------
# Take the union of the circuits and execute it
# union_circuit.q0: ─[H]──●──[M]─
#                         |
# union_circuit.q1: ─────[X]─[M]─
# ---------------------------
union_circuit = union([circuit1, circuit2])

qjob = run(union_circuit, qpu_union, shots=1024)# non-blocking call
results = qjob.result

print(f"Result after union: {results.counts}\n")

# ---------------------------
# Relinquishing resources
# ---------------------------
qdrop(family_union)
qdrop(family_separated)