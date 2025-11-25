import os, sys
import numpy as np
import matplotlib.pyplot as plt

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather

def mod(n, m):
    return (n % m + m) % m

def cyclic_ccommunication(n):
    family_0 = qraise(n,"00:10:00", simulator="Munich", classical_comm=True, co_located = True)
    #family_1 = qraise(n,"00:10:00", simulator="Cunqa", classical_comm=True, co_located = True)
    qpus_comm_0 = get_QPUs(on_node = False, family = family_0)
    #qpus_comm_1 = get_QPUs(family_1)
    qpus_comm = qpus_comm_0 #+ qpus_comm_1
    

    circuits = {}
    circuits["cc_0"]=CunqaCircuit(2,2, id= f"cc_0")
    circuits["cc_0"].h(1)
    circuits["cc_0"].cx(1,0)
    circuits["cc_0"].measure_and_send(qubit = 1, target_circuit = f"cc_{1}") 
    circuits["cc_0"].remote_c_if("x", qubits = 0, param=None, control_circuit = f"cc_{n-1}")

    circuits[f"cc_0"].measure(0,0)
    circuits[f"cc_0"].measure(1,1)
    
    for i in range(n-1):
        circuits[f"cc_{i+1}"]=CunqaCircuit(2,2, id= f"cc_{i+1}")
        
        circuits[f"cc_{i+1}"].remote_c_if("x", qubits = 0, param=None, control_circuit = f"cc_{i}")
        circuits[f"cc_{i+1}"].h(1)
        circuits[f"cc_{i+1}"].cx(1,0)

        siguiente = mod(i+2,n)
        circuits[f"cc_{i+1}"].measure_and_send(qubit = 1, target_circuit = f"cc_{siguiente}") 

        circuits[f"cc_{i+1}"].measure(0,0)
        circuits[f"cc_{i+1}"].measure(1,1)


    distr_jobs = run_distributed(list(circuits.values()), qpus_comm, shots=100)

    results_list = gather(distr_jobs)
    qdrop(family_0) #, family_1)
    return results_list

for result in cyclic_ccommunication(5):
    print(result)