"""
Code implementing the Iterative Quantum Phase Estimation (iQPE) algorithm with classical communications. To understand the algorithm without communications check:
    - Original paper (here referred to as Iterative Phase Estimation Algorithm): https://arxiv.org/abs/quant-ph/0610214
    - TalentQ explanation (in spanish): https://talentq-es.github.io/Fault-Tolerant-Algorithms/docs/Part_01_Fault-tolerant_Algorithms/Chapter_01_01_IPE_portada_myst.html
"""
import os, sys
import time
import numpy as np
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

# Global variables
N_QPUS = 7                  # Determines the number of bits of the phase that will be computed
PHASE_TO_COMPUTE = 1/2**5 
SHOTS = 1024
SEED = 18                   # Set seed for reproducibility

try:

    # 1. QPU deployment
    family_name = qraise(N_QPUS, "03:00:00", classical_comm=True, co_located = True)
    qpus  = get_QPUs(co_located = True, family = family_name)

    # 2. Circuit design: multiple circuits implementing the classically distributed Iterative Phase Estimation
    circuits = []
    for i in range(N_QPUS): 
        theta = 2**(N_QPUS - i) * PHASE_TO_COMPUTE 

        circuits.append(CunqaCircuit(2, 2, id= f"cc_{i}"))
        circuits[i].h(0)
        circuits[i].x(1)
        circuits[i].crz(theta, 0, 1)
        

        for j in range(i):
            param = -np.pi * 2**(-j - 1)
            recv_id = i - j - 1
  
            circuits[i].recv(0, sending_circuit = f"cc_{recv_id}")

            # Gate conditioned by the received bit
            with circuits[i].cif(0) as cgates:
                cgates.rz(param, 0)

        circuits[i].h(0)

        circuits[i].measure(0, 0)
        for j in range(N_QPUS - i - 1):

            circuits[i].send(0, recving_circuit = f"cc_{i + j + 1}")

        circuits[i].measure(1, 1)

    # 3. Execution
    algorithm_starts = time.time()
    distr_jobs = run(circuits, qpus, shots=SHOTS, seed=SEED)
    
    result_list = gather(distr_jobs)
    algorithm_ends = time.time()
    algorithm_time = algorithm_ends - algorithm_starts

    # 4. Post processing results
    counts_list = []
    for result in result_list:
        counts_list.append(result.counts)

    binary_string = ""
    for counts in counts_list:
        # Extract the most frequent measurement (the best estimate of theta)
        most_frequent_output = max(counts, key=counts.get)
        binary_string += most_frequent_output[0]

    estimated_theta = 0.0
    for i, digit in enumerate(reversed(binary_string)):
        if digit == '1':
            exponent = i + 1
            estimated_theta += 1 / (2**exponent)

    print(f"Estimated angle: {estimated_theta}")
    print(f"Real angle: {PHASE_TO_COMPUTE}")

    # 5. Release resources
    qdrop(family_name)

except Exception as error:
    qdrop(family_name)
    raise error