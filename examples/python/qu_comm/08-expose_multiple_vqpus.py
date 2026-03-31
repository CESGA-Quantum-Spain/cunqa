import os, sys

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather


family = qraise(3, "00:10:00", simulator="Munich", quantum_comm=True, co_located = True)
qpus = get_QPUs(co_located=True, family = family)

qc_0 = CunqaCircuit(2, 2, id="First")
qc_1 = CunqaCircuit(1, 1, id="Second")
qc_2 = CunqaCircuit(1, 1, id="Third")


qc_0.h(0)
qc_0.h(1)
qc_1.h(0)
rcontrols0 = qc_0.expose([0, 1], qc_2)
rcontrols1 = qc_1.expose(0, qc_2)

qc_2.mcx(rcontrols0[0], rcontrols0[1], rcontrols1[0], 0)

qc_2.unexpose(rcontrols0)
qc_2.unexpose(rcontrols1)

qc_0.measure(0,0)
qc_0.measure(1,1)
qc_1.measure(0,0)
qc_2.measure(0,0)


distr_jobs = run([qc_0, qc_1, qc_2], qpus, shots=1000, n_communication_qubits = 6)

result_list = gather(distr_jobs)

for i, result in enumerate(result_list):
    print(f"Counts {i} is {result.counts}")

qdrop(family)
