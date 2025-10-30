import os, sys
import numpy as np
import random
from collections import defaultdict
from pathlib import Path

# path to access c++ files
sys.path.append(os.getenv("HOME"))

examples_path: str = str(Path(__file__).resolve().parent.parent)

from cunqa.logger import logger
from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather

def n_plus(k):
    return 2**(2**k)-1

def n_minus(k):
    return 4**(k)-2**(k)

def t_k(k):
    return 2**k-1

def how_big_a_combination(k):
    #print(f"For generating a {k}-Cut Bell Pair Factory one needs to specify {4**(k)-2**(k)+2**(2**k)-1} parameter sets.")
    return 4**(k)-2**(k) + 2**(2**k)-1


# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs #
family = qraise(2,"00:10:00", simulator="Munich", classical_comm=True, co_located = True)
qpus  = get_QPUs(on_node = False, family = family)

# Params for the gates in the Cut Bell Pair Factory #
with open(examples_path + "/cc_examples/two_qpd_bell_pairs_param_values.txt") as fin:
    params2 = [[float(val) for val in line.replace("\n","").split(" ")] for line in fin.readlines()]
z = params2[0] # This one doesn't matter, it will be overwritten afterwards

############ CIRCUIT 1 #########################
Alice = CunqaCircuit(3,3, id="Alice")
# Cut Bell Pair Factory 1 #
Alice.rz(z[0] + np.pi/2 ,1)   
Alice.rz(z[1] + np.pi/2 ,2)   
Alice.sx(1)                       
Alice.sx(2)
Alice.rz(z[4],1)
Alice.rz(z[5],2)
Alice.sx(1)
Alice.sx(2)
Alice.rz(5*np.pi/2 + z[8],1)   
Alice.rz(5*np.pi/2 + z[9],2)   
Alice.cx(1,2)
Alice.rz(z[12],2)
Alice.cx(1,2)
Alice.rz(z[14],1)


# First telegate #
Alice.cx(0,1)
Alice.remote_c_if("z", qubits = 0, param=None, control_circuit = "Bobby")
Alice.measure_and_send(qubit = 1, target_circuit = "Bobby")
# Second telegate #
Alice.cx(0,2)
Alice.remote_c_if("z", qubits = 0, param=None, control_circuit = "Bobby")
Alice.measure_and_send(qubit = 2, target_circuit = "Bobby")

Alice.measure(0,0)
Alice.measure(1,1)
Alice.measure(2,2)

############# CIRCUIT 2 ######################

Bob = CunqaCircuit(3,3, id="Bobby")
# Cut Bell Pair Factory 2 #
Bob.rz(z[2]+ np.pi/2 ,0)   
Bob.rz(z[3]+ np.pi/2 ,1)   
Bob.sx(0)
Bob.sx(1)
Bob.rz(z[6],0)
Bob.rz(z[7],1)
Bob.sx(0)
Bob.sx(1)
Bob.rz(5*np.pi/2 + z[10],0)   
Bob.rz(5*np.pi/2 + z[11],1)   
Bob.cx(1,0)
Bob.rz(z[13],0)
Bob.cx(1,0)
Bob.rz(z[15],1)

# First telegate #
Bob.cx(1,2)
Bob.h(1)
Bob.measure_and_send(qubit = 1, target_circuit = "Alice")
Bob.remote_c_if("x", qubits = 2, param=None, control_circuit = "Alice")
# Second telegate #
Bob.cx(0,2)
Bob.h(0)
Bob.measure_and_send(qubit = 0, target_circuit = "Alice")
Bob.remote_c_if("x", qubits = 2, param=None, control_circuit = "Alice")

Bob.measure(0,0)
Bob.measure(1,1)
Bob.measure(2,2)



circs = [Alice, Bob]
distr_jobs = run_distributed(circs, qpus, shots=1) # create the jobs to store the circuits for upgrade_parameters,
                                                           # but the results of this first submission will be discarded.

########## Circuit combination for successful circuit cutting ##########
shots = 1024
Alice_counts = defaultdict(int)
Bob_counts = defaultdict(int)

# Calculate the probability of each circuit in the linear combination
probs_aux_1 = [(1+t_k(2))/n_plus(2) for _ in range(n_plus(2)-1)]
probs_aux_2 =  probs_aux_1 + [abs(-t_k(2)/n_minus(2)) for _ in range(n_minus(2)-1)]
gamma = sum(probs_aux_2)
probs =[ x /gamma for x in probs_aux_2]


for _ in range(shots): # Each shot uses a different circuit of the linear combination, which is randomly chosen in the next line depending on the weights on the linear combination
    z = params2[random.choices(range(len(probs)), weights=probs, k=1)[0]] 
    distr_jobs[0].upgrade_parameters([z[0]+ np.pi/2, z[1]+ np.pi/2, z[4], z[5], 5*np.pi/2 +z[8], 5*np.pi/2 +z[9], z[12], z[14]]) #Alice
    distr_jobs[1].upgrade_parameters([z[2]+ np.pi/2, z[3]+ np.pi/2, z[6], z[7], 5*np.pi/2 +z[10], 5*np.pi/2 +z[11], z[13], z[15]]) #Bob
    result_list = gather(distr_jobs)
    
    # Recombines the counts, adding the result of this shot
    for key, value in result_list[0].counts.items():
        Alice_counts[key] += int(value)
    for key, value in result_list[1].counts.items():
        Bob_counts[key] += int(value)


YELLOW = "\033[33m"
RESET = "\033[0m"
print(f"{YELLOW}Alice:{RESET} {dict(Alice_counts)}\n{YELLOW}Bobby:{RESET} {dict(Bob_counts)}")

# Drop the deployed QPUs #
qdrop(family)
