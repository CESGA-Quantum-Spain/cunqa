import os, sys
# path to access c++ files
sys.path.append(os.getenv("HOME"))
sys.path.append(os.getenv("STORE") + "/repos/pyzmq/")

from cunqa import get_QPUs, gather
from cunqa.circuit import CunqaCircuit
from cunqa.qutils import qraise, qdrop
from qiskit.qasm3 import dumps as dumps3

import zmq

# qraise -t 00:10:00 --qmio
family = qraise(1, "00:10:00", qmio = True)
qpus = get_QPUs(on_node = False)
qmio = qpus[0]

print(qmio)

circuit = CunqaCircuit(2,2)
circuit.h(0)
circuit.cx(0,1)
circuit.measure_all()

#qjob = qmio.run(circuit, shots = 100)
""" counts = qjob.result.counts
time = qjob.time_taken

print(f"Result: \n{counts}\n Time taken: {time} s.") """

qdrop(family)