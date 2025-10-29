"""
Code implementing the Iterative Quantum Phase Estimation (iQPE) algorithm with classical communications. To understand the algorithm without communications check:
    - Original paper (here referred to as Iterative Phase Estimation Algorithm): https://arxiv.org/abs/quant-ph/0610214
    - TalentQ explanation (in spanish): https://talentq-es.github.io/Fault-Tolerant-Algorithms/docs/Part_01_Fault-tolerant_Algorithms/Chapter_01_01_IPE_portada_myst.html
"""

import os, sys
import math
import numpy as np
import time
import statistics as st
import random

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather

def print_results(result_list, angle):
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

    estimated_theta = 2 * np.pi * estimated_theta

    print(f"Measured output: {binary_string}")
    print(f"Estimated theta: {estimated_theta}")
    print(f"Real theta: {angle}")
    #print(f"With probability: {counts[most_frequent_output]/total}", )
    if angle==estimated_theta:
        print(f"CORRECT: ({math.log2(angle)},{len(binary_string)})")
    else:
        print(f"FALSE  : ({math.log2(angle)},{len(binary_string)})")



def QPE_rz_circuits(angle_to_compute, n_qpus):
    """"
    Function that defines the circuits to compute the phase of a RZ.

    Args
    ---------
    angle (double): angle to compute.
    n_qpus (int): number of QPUs.
    """

    circuits = {}
    for i in range(n_qpus): 
        theta = 2**(n_qpus - i) * angle_to_compute 

        circuits[f"cc_{i}"] = CunqaCircuit(2, 2, id= f"cc_{i}")
        circuits[f"cc_{i}"].h(0)
        circuits[f"cc_{i}"].x(1)
        circuits[f"cc_{i}"].crz(theta, 0, 1)
        

        for j in range(i):
            param = -np.pi * 2**(-j - 1)
            recv_id = i - j - 1
  
            circuits[f"cc_{i}"].remote_c_if("rz", qubits = 0, param = param, control_circuit = f"cc_{recv_id}")

        circuits[f"cc_{i}"].h(0)

        for j in range(n_qpus - i - 1):
            circuits[f"cc_{i}"].measure_and_send(qubit = 0, target_circuit = f"cc_{i + j + 1}")

        circuits[f"cc_{i}"].measure(0, 0)
        circuits[f"cc_{i}"].measure(1, 1)

    
    return circuits


def get_computed_angle(results):
    counts_list = []
    for result in results:
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

    return estimated_theta


def QPE_rzxrz_circuits(angle, n_qpus):
    """"
    Function that defines the circuits to compute the phase of a RZ.

    Args
    ---------
    angle (double): angle to compute.
    n_qpus (int): number of QPUs.
    """

    circuits = {}
    for i in range(n_qpus): 
        theta = 2**(n_qpus - i - 1) * angle
        #print(f"Theta: {theta}")

        circuits[f"cc_{i}"] = CunqaCircuit(3,3, id= f"cc_{i}") #we set the same number of quantum and classical bits because Cunqasimulator requires all qubits to be measured for them to be represented on the counts
        circuits[f"cc_{i}"].h(0)
        circuits[f"cc_{i}"].rx(np.pi, 1)
        circuits[f"cc_{i}"].rx(np.pi, 2)
        circuits[f"cc_{i}"].crz(theta, 0, 1)
        circuits[f"cc_{i}"].crz(theta, 0, 2)
        

        for j in range(i):
            phase = -np.pi * 2**(j - i)
            circuits[f"cc_{i}"].remote_c_if("rz", qubits = 0, param = phase, control_circuit = f"cc_{j}")

        circuits[f"cc_{i}"].h(0)

        for k in range(n_qpus - i - 1):
            #print(f"Mandamos desde {i} a {i+1+k}.")
            circuits[f"cc_{i}"].measure_and_send(qubit = 0, target_circuit = f"cc_{i + 1 + k}") 

        circuits[f"cc_{i}"].measure(0,0)
        circuits[f"cc_{i}"].measure(1,1)
        circuits[f"cc_{i}"].measure(2,2)

    
    return circuits



def deploy_qpus(n_qpus, cores_per_qpu, mem_per_qpu, simulator = "Aer"):
    family = qraise(n_qpus, "03:00:00", simulator=simulator, classical_comm=True, co_located = True, cores = cores_per_qpu, mem_per_qpu = mem_per_qpu)
    return family


def run_iterative_QPE(circuits, qpus_name, shots, seed = 1234):

    qpus_QPE  = get_QPUs(on_node = False, family = qpus_name)
    algorithm_starts = time.time()
    distr_jobs = run_distributed(list(circuits.values()), qpus_QPE, shots=shots, seed=seed)
    
    result_list = gather(distr_jobs)
    algorithm_ends = time.time()
    algorithm_time = algorithm_ends - algorithm_starts

    return result_list, algorithm_time
        

def get_total_counts(results):
    total_counts = {}
    for result in results:
        for binary_string, counts in result.counts.items():
            if binary_string in total_counts:
                total_counts[binary_string] += counts
            else:
                total_counts[binary_string] = counts

    return total_counts 

def iqpe_benchmarking(angles_list, n_qpus_list, shots, cores_per_qpu, mem_per_qpu, seed):
    for angle in angles_list:
        for n_qpus in n_qpus_list:
            qpus = deploy_qpus(n_qpus, cores_per_qpu, mem_per_qpu)
            circuits = QPE_rz_circuits(2*np.pi * angle, n_qpus)
            results, time_taken = run_iterative_QPE(circuits, qpus, shots, seed)
            computed_angle = get_computed_angle(results)
            #total_counts = get_total_counts(results)

            dict_data = {
                "num_qpus":n_qpus,
                "total_time":time_taken,
                "qubits_per_QPU":2,
                "cores_per_qpu":cores_per_qpu,
                "mem_per_qpu":mem_per_qpu,
                "shots":shots,
                "input_theta":angle,
                "estimated_theta": computed_angle, 
                #"counts":total_counts
            }
            
            str_data =str(dict_data)
            with open(f"/mnt/netapp1/Store_CESGA/home/cesga/acarballido/repos/api-simulator/examples/python/cc_examples/results_iterative_QPE/iQPE_results.txt", "a") as f:
                f.write(str_data)

            qdrop(qpus)
            time.sleep(30)


if __name__ == "__main__":
    angles_list = [1/2**10, 1/np.pi]
    n_qpus = [16]
    shots = 1e6
    cores_per_qpu = 4
    mem_per_qpu = 60 # en GB
    seed = 13

    iqpe_benchmarking(angles_list, n_qpus, shots, cores_per_qpu, mem_per_qpu, seed)


    


