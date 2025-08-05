import os, sys
import math
import numpy as np
import matplotlib.pyplot as plt

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import getQPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather

def print_results(result_list, angle):
    counts_list = []
    for result in result_list:
        counts_list.append(result.counts)

    estimation = ""
    for counts in counts_list:
        # Extract the most frequent measurement (the best estimate of theta)
        most_frequent_output = max(counts, key=counts.get)
        total = sum([v for v in counts.values()])
        estimation += most_frequent_output[0]

    # Convert the (reversed) binary string to a fraction
    estimated_theta = int(estimation[::-1], 2) / (2 ** len(estimation))

    print(f"Measured output: {estimation[::-1]}")
    print(f"Estimated theta: {estimated_theta}")
    print(f"Real theta: {angle}")

    #print(f"With probability: {counts[most_frequent_output]/total}", )
    # if angle==estimated_theta:
    #     print(f"CORRECT: ({math.log2(angle)},{len(estimation)})")
    # else:
    #     print(f"FALSE  : ({math.log2(angle)},{len(estimation)})")



def distr_rz2_QPE(angle, n_precision):
    """"
    Function for distributed quantum phase estimation.

    Args
    ---------
    angle (float): percentage of a full turn to apply on the Rz \otimes Rz.
    n_precision (str): number of digits of the phase to extract. We will create the same number of circuits that run in different QPUs to run the distributed QPE.
    """
    family = qraise(n_precision,"00:10:00", simulator="Munich", classical_comm=True, cloud = True)
    
    qpus_QPE  = getQPUs(local = False, family = family)

    circuits = {}
    for i in range(n_precision): 
        theta = angle*np.pi*2**(n_precision-i) 
        #print(f"Theta: {theta}")

        circuits[f"cc_{i}"] = CunqaCircuit(3,3, id= f"cc_{i}") #we set the same number of quantum and classical bits because Cunqasimulator requires all qubits to be measured for them to be represented on the counts
        circuits[f"cc_{i}"].h(0)
        circuits[f"cc_{i}"].rx(np.pi,1)
        circuits[f"cc_{i}"].rx(np.pi,2)
        circuits[f"cc_{i}"].crz(theta,0,1)
        circuits[f"cc_{i}"].crz(theta,0,2)
        

        for j in range(i):
            #print(f"Receving from {j} on {i}.")
            circuits[f"cc_{i}"].remote_c_if("rz", target_qubits = 0, param = -np.pi*2**(i-j), control_circuit = f"cc_{j}")

        circuits[f"cc_{i}"].h(0)

        for k in range(n_precision-i-1): # -1 due to zero indexing
            #print(f"Sending from {i} to {i+1+k}.")
            circuits[f"cc_{i}"].measure_and_send(control_qubit = 0, target_circuit = f"cc_{i+1+k}") # +1 due to zero indexing 

        circuits[f"cc_{i}"].measure(0,0)
        circuits[f"cc_{i}"].measure(1,1)
        circuits[f"cc_{i}"].measure(2,2)

    
    distr_jobs = run_distributed(list(circuits.values()), qpus_QPE, shots=3000)
    
    result_list = gather(distr_jobs)
    qdrop(family)
    return result_list

angle = 1/2**5
result_list = distr_rz2_QPE(angle, 8)

for result in result_list:
    print(result)

print_results(result_list, angle)


