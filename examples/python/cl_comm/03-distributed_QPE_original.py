"""
Code implementing the Iterative Quantum Phase Estimation (iQPE) algorithm with classical communications. To understand the algorithm without communications check:
    - Original paper (here referred to as Iterative Phase Estimation Algorithm): https://arxiv.org/abs/quant-ph/0610214
    - TalentQ explanation (in spanish): https://talentq-es.github.io/Fault-Tolerant-Algorithms/docs/Part_01_Fault-tolerant_Algorithms/Chapter_01_01_IPE_portada_myst.html
"""
import os, sys
import math
import numpy as np
import time

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

# 1. Deploy QPUS
def deploy_qpus(n_qpus, cores_per_qpu, mem_per_qpu, simulator = "Aer"):
    family = qraise(n_qpus, "03:00:00", simulator=simulator, classical_comm=True, co_located = True, cores = cores_per_qpu, mem_per_qpu = mem_per_qpu)
    return family

# 2. Circuit design: multiple circuits implementing the classically distributed Iterative Phase Estimation
def QPE_rz_circuits(angle_to_compute, n_qpus):
    """"
    Function that defines the circuits to compute the phase of a RZ.

    Args
    ---------
    angle (double): angle to compute.
    n_qpus (int): number of QPUs.
    """

    circuits = []
    for i in range(n_qpus): 
        theta = 2**(n_qpus - i) * angle_to_compute 

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
        for j in range(n_qpus - i - 1):

            circuits[i].send(0, recving_circuit = f"cc_{i + j + 1}")

        circuits[i].measure(1, 1)

    
    return circuits

# 3. Execution
def run_iterative_QPE(circuits, qpus_name, shots, seed = 1234):

    qpus_QPE  = get_QPUs(co_located = True, family = qpus_name)
    algorithm_starts = time.time()
    distr_jobs = run(circuits, qpus_QPE, shots=shots, seed=seed)
    
    result_list = gather(distr_jobs)
    algorithm_ends = time.time()
    algorithm_time = algorithm_ends - algorithm_starts

    return result_list, algorithm_time
        
# 4. Post processing results to obtain the estimated phase
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

# Full workflow
def iqpe_benchmarking(angles_list, n_qpus_list, shots, cores_per_qpu, mem_per_qpu, seed):
    try:
        for angle in angles_list:
            for n_qpus in n_qpus_list:
                qpus                = deploy_qpus(n_qpus, cores_per_qpu, mem_per_qpu)
                circuits            = QPE_rz_circuits(2*np.pi * angle, n_qpus)
                results, time_taken = run_iterative_QPE(circuits, qpus, shots, seed)
                computed_angle      = get_computed_angle(results)

                dict_data = {
                    "num_qpus":n_qpus,
                    "total_time":time_taken,
                    "qubits_per_QPU":2,
                    "cores_per_qpu":cores_per_qpu,
                    "mem_per_qpu":mem_per_qpu,
                    "shots":shots,
                    "input_theta":angle,
                    "estimated_theta": computed_angle, 
                }
                
                str_data =str(dict_data)
                with open(f"results_iterative_QPE/iQPE_results.txt", "a") as f:
                    f.write(str_data)

                # 5. Release resources at the end of execution 
                qdrop(qpus)
                time.sleep(10)

    except Exception as error:
        # 5. Release resources even if an error is raised
        qdrop(qpus)
        raise error

if __name__ == "__main__":
    angles_list = [1/2**10, 1/np.pi]
    n_qpus = [16]
    shots = 1e6
    cores_per_qpu = 4
    mem_per_qpu = 60 # units: GB
    seed = 13

    iqpe_benchmarking(angles_list, n_qpus, shots, cores_per_qpu, mem_per_qpu, seed)

