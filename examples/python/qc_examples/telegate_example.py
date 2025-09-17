import os, sys
import numpy as np

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
""" family = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, cloud = True)
qpus_QPE  = get_QPUs(local=False, family = family)
 """
qpus_QPE  = get_QPUs(local=False)
########## Circuits to run ##########
########## First circuit ############
cc_1 = CunqaCircuit(1, id="First")
cc_2 = CunqaCircuit(1, id="Second")

cc_1.h(0)

with cc_1.expose(0, cc_2) as rcontrol:
    cc_2.cx(rcontrol,0)


cc_1.measure_all()
cc_2.measure_all()



########## List of circuits #########
circs_QPE = [cc_1, cc_2]

""" for circ in circs_QPE:
    for c in circ.instructions:
        print(c)
    print() """

########## Distributed run ##########
distr_jobs = run_distributed(circs_QPE, qpus_QPE, shots=100) 

########## Collect the counts #######
result_list = gather(distr_jobs)

########## Print the counts #######
for result in result_list:
    print(result)

########## Drop the deployed QPUs #
#qdrop(family)