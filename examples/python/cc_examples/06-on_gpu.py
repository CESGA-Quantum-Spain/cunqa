import os, sys
import numpy as np

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs

#-----------------------------------------------------------------------------------
# qraise -n 2 -t 00:10:00 -c 32 --co-located --classical_comm --gpu --fam="ccongpu"
#-----------------------------------------------------------------------------------

family_name = "ccongpu"
qpus  = get_QPUs(on_node=False, family = family_name)



########## Circuits to run ##########
########## First circuit ############
cc_1 = CunqaCircuit(10, 2, id="First")
cc_1.h(0)
cc_1.measure_and_send(qubit = 0, target_circuit = "Second")
cc_1.measure(0,0)
cc_1.measure(1,1)

########## Second circuit ###########
cc_2 = CunqaCircuit(2, 2, id="Second")
cc_2.remote_c_if("x", qubits = 0, param=None, control_circuit = "First")
cc_2.measure(0,0)
cc_2.measure(1,1)


########## List of circuits #########
circs = [cc_1, cc_2]


########## Distributed run ##########
distr_jobs = run_distributed(circs, qpus, shots=10) 

########## Collect the counts #######
result_list = gather(distr_jobs)

########## Print the counts #######
for result in result_list:
    print(result)

########## Drop the deployed QPUs #
qdrop(family_name)
