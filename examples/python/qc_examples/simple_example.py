import os, sys
import numpy as np

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
family = qraise(2, "00:10:00", simulator="Munich", quantum_comm=True, cloud = True)
qpus_QPE  = get_QPUs(local=False, family = family)

########## Circuits to run ##########
########## First circuit ############
cc_1 = CunqaCircuit(1, 1, id="First")
cc_1.h(0)
cc_1.qsend(qubit = 0, target_circuit = "Second")
cc_1.measure(0,0)

########## Second circuit ###########
cc_2 = CunqaCircuit(2, 2, id="Second")
cc_2.qrecv(qubit = 0, control_circuit = "First")
cc_2.cx(0, 1)
cc_2.measure(0,0)
cc_2.measure(1,1)

########## List of circuits #########
circs_QPE = [cc_1, cc_2]


########## Distributed run ##########
distr_jobs = run_distributed(circs_QPE, qpus_QPE, shots=10)

########## Collect the counts #######
result_list = gather(distr_jobs)

########## Print the counts #######
for result in result_list:
    print(result)

########## Drop the deployed QPUs #
#qdrop(family)
