import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.transformations import hsplit

# ---------------------------
# Acquiring resources
# ---------------------------
family_original = qraise(1, "00:10:00", simulator="Aer", co_located=True)
[qpu_original]  = get_QPUs(co_located=True, family=family_original)

family_separated = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, co_located=True)
qpus_separated = get_QPUs(co_located=True, family=family_separated)

# ---------------------------
# Original circuit
# union_circuit.q0: ─[H]──●──[M]─
#                         |
# union_circuit.q1: ─────[X]─[M]─
# ---------------------------
circuit = CunqaCircuit(2, id="circuit")
circuit.h(0)
circuit.cx(0, 1)
circuit.measure_all()

qjob = run(circuit, qpu_original, shots=1024) # non-blocking call
results = qjob.result

print(f"\nResult before hsplit: {results.counts}")

# ---------------------------
# Split original circuit to create two communicated circuits, and execute them
# circuit1.q0: ─[H]──●──[M]─
#                    $
# circuit2.q0: ─────[X]─[M]─
# Where $ represents the remote control of the gate
# ---------------------------
[circuit1, circuit2] = hsplit(circuit, 2)

qjobs = run([circuit1, circuit2], qpus_separated, shots=1024)
results = gather(qjobs)

print(f"Result after split: {results[0].counts}\n") # taking only the results from the 0 circuit
                                                    # as both are the same, due to entanglement

# ---------------------------
# Relinquishing resources
# ---------------------------
qdrop(family_original)
qdrop(family_separated)
