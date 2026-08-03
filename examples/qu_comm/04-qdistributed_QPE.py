"""
Code implementing a simple distribution variant of the QPE algorithm. 
We use the telegate protocol to apply gates on a circuit that has the eigenvector from a circuit where the bits of the phase will be measured.
"""

import os, sys
import numpy as np

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.qc_protocols import cat_entangler, cat_disentangler
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

N_QPUS = 2
CORES_PER_QPU = 4
MEM_PER_QPU = 60 # in GB
N_ANCILLA_QUBITS = 10
N_REGISTER_QUBITS = 1
PHASE_TO_COMPUTE = 1 / 2**3

shots = 1
SEED = 18

try:
    file_dir = os.path.dirname(os.path.abspath(__file__))
    backend_path = file_dir + "/04-qdistributed_QPE.json"
    # 1. Deploy vQPUs
    family = qraise(N_QPUS, "00:10:00", 
                    simulator="Aer",
                    quantum_comm = True, 
                    co_located = True, 
                    cores_per_qpu = CORES_PER_QPU, 
                    mem_per_qpu = MEM_PER_QPU,
                    backend=backend_path)
except Exception as error:
    raise error

try:
    qpus = get_QPUs(co_located = True, family = family)

    # 2. Design circuits modelling the QPE 
    ancilla_circuit  = CunqaCircuit((N_ANCILLA_QUBITS, 1), N_ANCILLA_QUBITS, id = "ancilla_circuit")
    register_circuit = CunqaCircuit((N_REGISTER_QUBITS, 1), 1, id = "register_circuit")

    data_qubits_anc, comm_qubits_anc = ancilla_circuit.get_qubits()
    data_qubits_reg, comm_qubits_reg = register_circuit.get_qubits()

    register_circuit.x(data_qubits_reg[0]) # Rz statevector

    for i in range(N_ANCILLA_QUBITS):
        ancilla_circuit.h(data_qubits_anc[i])

    for i in range(N_ANCILLA_QUBITS):
        ### TELEGATE ###
        # Every telegate block needs its own tag, so that each entanglement
        # generation is matched with the right one at the other circuit
        cat_entangler(
            target_circuits = [ancilla_circuit, register_circuit],
            data_qubit      = data_qubits_anc[N_ANCILLA_QUBITS - 1 - i],
            comm_qubits     = [comm_qubits_anc[0], comm_qubits_reg[0]],
            clbits          = [0, 0],
            tag             = f"telegate_{i}"
        )
        
        param = (2**i) * 2 * 2 * np.pi * PHASE_TO_COMPUTE
        register_circuit.crz(param, comm_qubits_reg[0], data_qubits_reg[0])

        cat_disentangler(
            target_circuits = [ancilla_circuit, register_circuit],
            data_qubit      = data_qubits_anc[N_ANCILLA_QUBITS - 1 - i],
            comm_qubits     = [comm_qubits_reg[0]],
            recv_clbits     = [0],
            send_clbits     = [0]
        )

    # Swap qubits
    if (N_ANCILLA_QUBITS % 2) == 0:
        swap_range = int(N_ANCILLA_QUBITS / 2)
    else:
        swap_range = int((N_ANCILLA_QUBITS - 1) / 2)

    for i in range(swap_range):
        ancilla_circuit.swap(data_qubits_anc[i], data_qubits_anc[N_ANCILLA_QUBITS - 1 - i])

    # QFT dagger
    for i in range(N_ANCILLA_QUBITS):
        for j in range(i):
            angle  = (-np.pi) / (2**(i - j)) 
            ancilla_circuit.crz(angle, data_qubits_anc[N_ANCILLA_QUBITS - 1 - j], data_qubits_anc[N_ANCILLA_QUBITS - 1 - i])
        ancilla_circuit.h(data_qubits_anc[N_ANCILLA_QUBITS - 1 - i])

    # Measure
    for i in range(N_ANCILLA_QUBITS):
        ancilla_circuit.measure(data_qubits_anc[i], i)


    # 3. Execute distributed QPE circuit on communicated QPUs
    distr_jobs = run([ancilla_circuit, register_circuit], qpus, shots=shots, seed=SEED)
    result_list = gather(distr_jobs)
    

    # 4. Post-processing results to extract estimated phase 
    counts = result_list[0].counts
    print(f"Counts: {counts}")
    print(f"Time taken: {result_list[0].time_taken}")

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