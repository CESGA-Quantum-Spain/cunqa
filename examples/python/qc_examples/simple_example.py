import os, sys
import numpy as np

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import getQPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using getQPUs
family = qraise(2, "00:10:00", simulator="Munich", quantum_comm=True, cloud = True)
qpus_QPE  = getQPUs(local=False, family = family)

########## Circuits to run ##########
########## First circuit ############
cc_1 = CunqaCircuit(1, 2, id="First")
cc_1.h(0)
cc_1.qsend(sent_qubit = 0, target_circuit = "Second")

########## Second circuit ###########
cc_2 = CunqaCircuit(2, 2, id="Second")
cc_2.qrecv(recv_qubit = 0, target_circuit = "First")
cc_2.cx(0, 1)
cc_2.measure(0,0)
cc_2.measure(1,1)


########## List of circuits #########
circs_QPE = [cc_1, cc_2]


########## Distributed run ##########
distr_jobs = run_distributed(circs_QPE, qpus_QPE, shots=1000) 

########## Collect the counts #######
result_list = gather(distr_jobs)

########## Print the counts #######
for result in result_list:
    print(result)

########## Drop the deployed QPUs #
qdrop(family)
