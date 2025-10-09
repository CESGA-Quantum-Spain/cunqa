import os, sys
import numpy as np

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather


qpus  = get_QPUs(local=False)

hybrid_qpus = [qpus[0], qpus[1], qpus[4]] # First two with QC and last with CC (/examples/infrastructure/dummy_infrastructure.json)
print("qpus_selected: ", [qpu.name for qpu in hybrid_qpus])

########## Circuits to run ##########
########## Circuits with QC ############
qc_1 = CunqaCircuit(10, 2, id="First_QC")
qc_1.h(0)
qc_1.measure_and_send(qubit = 0, target_circuit = "CC_circuit")
qc_1.measure_all() # WARNING! Fails if not measure_all()

qc_2 = CunqaCircuit(1, 1, id="Second_QC") 
qc_2.x(0)
qc_2.measure_all()

########## Circuit with CC ###########
cc = CunqaCircuit(2, 2, id="CC_circuit")
cc.remote_c_if("x", qubits = 0, param=None, control_circuit = "First_QC")
cc.measure_all()


########## List of circuits #########
circs = [qc_1, qc_2, cc]


########## Distributed run ##########
distr_jobs = run_distributed(circs, hybrid_qpus, shots=10) 

########## Collect the counts #######
result_list = gather(distr_jobs)

########## Print the counts #######
for result in result_list:
    print(result)

