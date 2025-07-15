"""
 Title: Distributed Grover Algorithm.
 Description: implementation of the distributed Gover algorithm from https://arxiv.org/abs/2502.19118 using CUNQA.

Created 14/07/2025
@author: dexposito
"""

import os, sys
from typing import  Union, Any, Optional

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.circuit import CunqaCircuit
from cunqa.logger import logger
from cunqa.qpu import QPU
from cunqa.mappers import run_distributed
from cunqa.qutils import qraise, qdrop, getQPUs
from cunqa.qjob import QJob, gather

def distrGrover(target, n_nodes: int, qubits_per_node: list, n_layers: int = None):
    """
    Distributed Grover algorithm presented in https://arxiv.org/abs/2502.19118. 

    Args:
        target (str): bitstring with the state to be targeted, needed to build the oracle
        n_nodes (int): number of nodes through which to distribute the algorithm
        qubits_per_node (list[int]): list specifying how many qubits are on each node
        n_layers (int): specifies how many passes of the two grover blocks (oracle and diffusor) should be applied
    
    Return:

    """

    router = CunqaCircuit(n_nodes, id="router")

    circuits = {}
    for i in range(n_nodes):
        circuits[f"circ_{i}"] = CunqaCircuit(qubits_per_node[i]+1, qubits_per_node[i]+1, id=f"circ_{i}")

    for layer in n_layers:
        ################ ORACLE BLOCK ################

        # TODO

        ################ DIFFUSION BLOCK ################

        for i in range(n_nodes):
            for j in range(qubits_per_node[i]-1):
                circuits[f"circ_{i}"].h(j+1)

            # Entagle first qubit of the circuit with the router
            circuits[f"circ_{i}"].h(0)
            circuits[f"circ_{i}"].qsend(sent_qubit = 0, target_circuit = router)
            router.qrecv(recv_qubit = i, target_circuit = f"circ_{i}")
            # TODO: continue writing the diffusor. The problem i found is that quantum communication
            # is only implemented in the form of teledata so far, while i need telegate here.

    for i in range(n_nodes):
        for j in range(qubits_per_node):
            circuits[f"circ_{i}"].measure(j,j)

    ######### Execution part #########
    # Raise the required QPUs
    qpus_to_drop = qraise(n_nodes+1, "00:10:00", cloud=True, simulator="Munich", quantum_comm=True)
    qpus_Grover = getQPUs(local=False)

    # Distributed run
    distr_jobs = run_distributed([router] + list(circuits.values()), qpus_Grover, shots=1000) 
    result_list = gather(distr_jobs)

    # Print counts
    for result in result_list:
        print(result)

    # drop the deployed QPUs
    qdrop(qpus_to_drop)

