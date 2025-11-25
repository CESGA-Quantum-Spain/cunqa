import os
import sys
import numpy as np
from typing import Union

sys.path.append(os.getenv("HOME"))
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather
from cunqa.qpu import qraise, qdrop, get_QPUs
from cunqa.mappers import run_distributed
#from distr_Grover import distrGrover

#distrGrover("00101101", n_nodes = 3, qubits_per_circuit = [3,3,2], n_layers = 1)


CUNQAAAAAAA = CunqaCircuit(1)
OTROOOO = CunqaCircuit(1)


with CUNQAAAAAAA.expose(0, OTROOOO) as rcontrol:
            CUNQAAAAAAA.h(0)
            OTROOOO.cx(rcontrol, 0)

CUNQAAAAAAA.measure_and_send(qubit = 0, target_circuit = OTROOOO) 
OTROOOO.remote_c_if("x", qubits = 0, control_circuit = CUNQAAAAAAA)

CUNQAAAAAAA.measure_all()
OTROOOO.measure_all()

# Raise the required QPUs
qpus_to_drop = qraise(2, "00:10:00", cloud=True, simulator="Munich", quantum_comm=True)
qpus_test = get_QPUs(local=False)
print("before run_distributed")

# Distributed run
distr_jobs = run_distributed([CUNQAAAAAAA, OTROOOO], qpus_test, shots=1000) 
result_list = gather(distr_jobs)
print("after run_distributed")

# Print counts
for result in result_list:
    print(result)

# drop the deployed QPUs
qdrop(qpus_to_drop)