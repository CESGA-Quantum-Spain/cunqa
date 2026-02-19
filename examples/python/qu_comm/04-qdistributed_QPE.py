"""
Code implementing a simple distribution variant of the QPE algorithm. 
We use the telegate protocol to apply gates on a circuit that has the eigenvector from a circuit where the bits of the phase will be measured.
"""

import os, sys
import numpy as np

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

N_QPUS = 2
CORES_PER_QPU = 4
MEM_PER_QPU = 60 # in GB
N_ANCILLA_QUBITS = 16
N_REGISTER_QUBITS = 1
PHASE_TO_COMPUTE = 1 / 2**12

shots = 10
SEED = 18

try:
    # 1. Deploy vQPUs
    family = qraise(N_QPUS, "10:00:00", 
                    simulator="Aer",
                    quantum_comm = True, 
                    co_located = True, 
                    cores = CORES_PER_QPU, 
                    mem_per_qpu = MEM_PER_QPU)
    qpus = get_QPUs(co_located = True, family = family)

    # 2. Design circuits modelling the QPE 
    ancilla_circuit  = CunqaCircuit(N_ANCILLA_QUBITS, id = "ancilla_circuit")
    register_circuit = CunqaCircuit(N_REGISTER_QUBITS, id = "register_circuit")

    register_circuit.x(0) # Rz statevector

    for i in range(N_ANCILLA_QUBITS):
        ancilla_circuit.h(i)

    for i in range(N_ANCILLA_QUBITS):
        ### TELEGATE ###
        with ancilla_circuit.expose(N_ANCILLA_QUBITS - 1 - i, register_circuit) as (rqubit, subcircuit):
            param = (2**i) * 2 * 2 * np.pi * PHASE_TO_COMPUTE
            subcircuit.crz(param, rqubit, 0)

    # Swap qubits
    if (N_ANCILLA_QUBITS % 2) == 0:
        swap_range = int(N_ANCILLA_QUBITS / 2)
    else:
        swap_range = int((N_ANCILLA_QUBITS - 1) / 2)

    for i in range(swap_range):
        ancilla_circuit.swap(i, N_ANCILLA_QUBITS - 1 - i)

    # QFT dagger
    for i in range(N_ANCILLA_QUBITS):
        for j in range(i):
            angle  = (-np.pi) / (2**(i - j)) 
            ancilla_circuit.crz(angle, N_ANCILLA_QUBITS - 1 - j, N_ANCILLA_QUBITS - 1 - i)
        ancilla_circuit.h(N_ANCILLA_QUBITS - 1 - i)

    ancilla_circuit.measure_all()


    # 3. Execute distributed QPE circuit on communicated QPUs
    distr_jobs = run([ancilla_circuit, register_circuit], qpus, shots=shots, seed=SEED)
    result_list = gather(distr_jobs)
    

    # 4. Post-processing results to extract estimated phase 
    counts = result_list[0].counts

    most_frequent_output = max(counts, key=counts.get)
    print(f"Most frequent output is {most_frequent_output}")

    estimated_theta = 0.0
    for i, digit in enumerate(most_frequent_output):
        if digit == '1':
            estimated_theta += 1 / (2 ** (N_ANCILLA_QUBITS - i))

    
    print(f"Estimated angle: {estimated_theta}")
    print(f"Real angle: {PHASE_TO_COMPUTE}")

    # 5. Drop the deployed QPUs 
    qdrop(family)
except Exception as error:
    # 5. Release resources even if an error is raised
    qdrop(family)
    raise error