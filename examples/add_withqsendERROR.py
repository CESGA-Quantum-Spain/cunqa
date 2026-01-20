import os, sys

home = os.getenv("HOME")
sys.path.append(home)

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.transformations import add
from pprint import pprint

circuit1 = CunqaCircuit(2, id = "circuit1")
circuit1.h(0)
circuit1.cx(0,1)
circuit1.qsend(0, "circuit2")
circuit1.measure_all()

circuit2 = CunqaCircuit(2, id = "circuit2")
circuit2.qrecv(0, "circuit1")
circuit2.measure_all()

union_circuit = add([circuit1, circuit2]) #error, because they are communicated