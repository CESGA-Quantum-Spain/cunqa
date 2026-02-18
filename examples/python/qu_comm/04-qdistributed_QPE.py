"""
Code implementing a simple distribution variant of the QPE algorithm. 
We use the telegate protocol to apply gates on a circuit that has the eigenvector from a circuit where the bits of the phase will be measured.
"""
import os, sys
import numpy as np
import time

# Append path to access CUNQA installation
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

### Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
def deploy_qpus(n_qpus, cores_per_qpu, mem_per_qpu, simulator = "Aer"):
    family = qraise(n_qpus, "10:00:00", simulator=simulator, quantum_comm = True, co_located = True, cores = cores_per_qpu, mem_per_qpu = mem_per_qpu)
    return family

### Create circuits modelling the QPE ###
def dist_QPE_rz_circuits(n_ancilla_qubits, angle_to_compute):
    ancilla_circuit  = CunqaCircuit(n_ancilla_qubits, id = "ancilla_circuit")
    register_circuit = CunqaCircuit(1, id = "register_circuit")

    register_circuit.x(0) # Rz statevector

    for i in range(n_ancilla_qubits):
        ancilla_circuit.h(i)

    for i in range(n_ancilla_qubits):
        ### TELEGATE ###
        with ancilla_circuit.expose(n_ancilla_qubits - 1 - i, register_circuit) as rcontrol:
            param = (2**i) * angle_to_compute
            register_circuit.crz(param, rcontrol, 0)

    # Swap qubits
    if (n_ancilla_qubits % 2) == 0:
        swap_range = int(n_ancilla_qubits / 2)
    else:
        swap_range = int((n_ancilla_qubits - 1) / 2)

    for i in range(swap_range):
        ancilla_circuit.swap(i, n_ancilla_qubits - 1 - i)

    # QFT dagger
    for i in range(n_ancilla_qubits):
        for j in range(i):
            angle  = (-np.pi) / (2**(i - j)) 
            ancilla_circuit.crz(angle, n_ancilla_qubits - 1 - j, n_ancilla_qubits - 1 - i)
        ancilla_circuit.h(n_ancilla_qubits - 1 - i)

    ancilla_circuit.measure_all()

    return [ancilla_circuit, register_circuit]


### Distributed run ###
def run_distributed_QPE(circuits, qpus_name, shots, seed = 1234):

    qpus_QPE  = get_QPUs(co_located = True, family = qpus_name)
    algorithm_starts = time.time()
    distr_jobs = run(circuits, qpus_QPE, shots=shots, seed=seed)
    
    result_list = gather(distr_jobs)
    algorithm_ends = time.time()
    elapsed_time = algorithm_ends - algorithm_starts

    return result_list, elapsed_time

#### Post-processing results to extract estimated phase ###
def get_estimated_angle(results):
    counts = results[0].counts

    most_frequent_output = max(counts, key=counts.get)
    print(f"Most frequent output is {most_frequent_output}")

    estimated_theta = 0.0
    for i, digit in enumerate(most_frequent_output):
        if digit == '1':
            estimated_theta += 1 / (2 ** (i + 1))

    return estimated_theta

### Full algorithm ###
def dist_qpe_benchmarking(angles_list, n_ancilla_qubits, shots, cores_per_qpu, mem_per_qpu, seed):
    for angle in angles_list:
        qpus = deploy_qpus(2, cores_per_qpu, mem_per_qpu)

        circuits = dist_QPE_rz_circuits(n_ancilla_qubits, 2 * 2 * np.pi * angle) # The first 2 is because we are using Rz whose eigenvalue is exp(i*theta/2)
        results, time_taken = run_distributed_QPE(circuits, qpus, shots, seed)
        estimated_angle = get_estimated_angle(results)

        # Write result data into a file
        result_data = str({
            "num_qpus":2,
            "total_time":time_taken,
            "n_ancilla_qubits":n_ancilla_qubits,
            "cores_per_qpu":cores_per_qpu,
            "mem_per_qpu":mem_per_qpu,
            "shots":shots,
            "input_theta":angle,
            "estimated_theta": estimated_angle, 
            "counts": results[0].counts
        })
        
        with open(f"./dist_QPE_results.txt", "a") as f:
            f.write(result_data)

        # Drop the deployed QPUs #
        qdrop(qpus)
        time.sleep(30)


if __name__ == "__main__":
    n_ancilla_qubits = 16
    n_register_qubits = 1
    angles_to_compute = [1 / 2**10, 1 / np.pi]   
    shots = 10
    cores_per_qpu = 4
    mem_per_qpu = 60 # en GB
    seed = 13

    dist_qpe_benchmarking(angles_to_compute, n_ancilla_qubits, shots, cores_per_qpu, mem_per_qpu, seed)
