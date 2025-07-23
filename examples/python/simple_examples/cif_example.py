import os, sys
# path to access c++ files
#sys.path.append(os.getenv("HOME"))

from cunqa.mappers import run_distributed
from cunqa.qjob import gather

from cunqa.qutils import getQPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit

from qiskit import QuantumCircuit

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using getQPUs
family = qraise(2, "00:10:00", simulator = "Aer",  cloud = True)

qpus  = getQPUs(local = False, family = family)

""" qc = CunqaCircuit(2, 2)
qc.h(0)
qc.c_if("x", 0, 1)
qc.measure_all() """

qc = QuantumCircuit(2, 2)
qc.h(0)
qc.measure(0, 1)
qc.x(1).c_if(1, 1)
qc.measure_all()


qpu = qpus[0]
qjob = qpu.run(qc, shots = 1024)# non-blocking call
print(qjob.result.counts)

""" distr_jobs = run_distributed([qc], [qpu], shots=20) 

########## Collect the counts #######
result_list = gather(distr_jobs)

########## Print the counts #######
for result in result_list:
    print(result) """
########## Drop the deployed QPUs #
qdrop(family)
