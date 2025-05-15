import os, sys
import numpy as np
import matplotlib.pyplot as plt
# path to access c++ files
installation_path = os.getenv("INSTALL_PATH")
sys.path.append(installation_path)

from cunqa.qutils import getQPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather

# Function that ensures that -1 = n-1 (mod n)
def mod(n, m):
    return (n % m + m) % m

def cyclic_ccommunication(n):
    # Raise and get QPUs
    family = qraise(n,"00:10:00", simulator="Cunqa", classical_comm=True, cloud = True)
    os.system('sleep 5')
    qpus_comm = getQPUs(family)

    if n<5:
        print(f"This circuit relations topology needs at leats 5 circuits, but only {n} were requested.")
        raise SystemExit
    
    # Create first circuit that quickstarts the loop
    circuits = {}
    circuits["cc_0"]=CunqaCircuit(1,1, id= f"cc_0")
    circuits["cc_0"].h(0)
    circuits["cc_0"].rz(np.pi/6, 0)
    circuits["cc_0"].measure_and_send(control_qubit = 0, target_circuit = f"cc_{1}") 
    circuits["cc_0"].measure_and_send(control_qubit = 0, target_circuit = f"cc_{2}")
    circuits["cc_0"].measure_and_send(control_qubit = 0, target_circuit = f"cc_{3}")

    circuits["cc_0"].remote_c_if("x", target_qubits = 0, param=None, control_circuit = f"cc_{n-1}")

    circuits[f"cc_0"].measure(0,0)
    
    # For loop that creates the rest of the circuits and executes the communication instructions
    for i in range(n-1):
        circuits[f"cc_{i+1}"]=CunqaCircuit(1,1, id= f"cc_{i+1}")
        circuits[f"cc_{i+1}"].remote_c_if("x", target_qubits = 0, param=None, control_circuit = f"cc_{i}")
        back_2 = mod(i-1,n)
        back_3 = mod(i-2,n)
        if i > 1:
            circuits[f"cc_{i+1}"].remote_c_if("rx", target_qubits = 0, param=np.pi/5, control_circuit = f"cc_{back_2}")

        circuits[f"cc_{i+1}"].h(0)

        if i > 2:
            circuits[f"cc_{i+1}"].remote_c_if("ry", target_qubits = 0, param=np.pi/3, control_circuit = f"cc_{back_3}")
        
        ############# NOW WE SEND ##################

        next = mod(i+2,n)
        next_2 = mod(i+3,n)
        next_3 = mod(i+4,n)
        circuits[f"cc_{i+1}"].measure_and_send(control_qubit = 0, target_circuit = f"cc_{next}") 

        if i < n-2:
            circuits[f"cc_{i+1}"].measure_and_send(control_qubit = 0, target_circuit = f"cc_{next_2}")
            if i < n-3:
                circuits[f"cc_{i+1}"].measure_and_send(control_qubit = 0, target_circuit = f"cc_{next_3}")

        circuits[f"cc_{i+1}"].measure(0,0)
        


    distr_jobs = run_distributed(list(circuits.values()), qpus_comm, shots=1024)
    
    counts_list = [[circ_res[0],circ_res[1].counts] for circ_res in gather(distr_jobs, True)]
    qdrop(family)
    return counts_list

print(cyclic_ccommunication(5))