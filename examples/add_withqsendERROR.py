import os, sys

# Path to CUNQA installation
sys.path.append(os.getenv("HOME"))

from cunqa.circuit import CunqaCircuit
from cunqa.circuit.transformations import add

circuit1 = CunqaCircuit(2, id = "circuit1")
circuit1.h(0)
circuit1.cx(0,1)
circuit1.qsend(0, "circuit2")

circuit2 = CunqaCircuit(2, id = "circuit2")
circuit2.qrecv(0, "circuit1")

union_circuit = add([circuit1, circuit2]) #error, because they are communicated