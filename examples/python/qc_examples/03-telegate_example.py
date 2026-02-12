import os, sys
import numpy as np

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
family = qraise(2, "00:10:00", simulator="Qulacs", quantum_comm=True, co_located = True)
qpus  = get_QPUs(on_node=False, family = family)

#qpus_QPE  = get_QPUs(on_node=False)
########## Circuits to run ##########
########## First circuit ############
cc_1 = CunqaCircuit(1, id="First")
cc_2 = CunqaCircuit(1, id="Second")

cc_1.h(0)

with cc_1.expose(0, cc_2) as (rqubit, subcircuit):
    subcircuit.cx(rqubit,0)


cc_1.measure_all()
cc_2.measure_all()



########## List of circuits #########
circs_QPE = [cc_1, cc_2]

""" for circ in circs_QPE:
    for c in circ.instructions:
        print(c)
    print() """

########## Distributed run ##########
distr_jobs = run_distributed(circs_QPE, qpus, shots=100) 

########## Collect the counts #######
result_list = gather(distr_jobs)

########## Print the counts #######
for result in result_list:
    print(result)

########## Drop the deployed QPUs #
qdrop(family)