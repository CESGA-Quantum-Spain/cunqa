import os, sys
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import  qraise, get_QPUs, run, qdrop
from cunqa.circuit import CunqaCircuit


family = qraise(1, "00:10:00", qmio = True)
qpus = get_QPUs(co_located = True)
qmio = qpus[0]

circuit = CunqaCircuit(2, 4)
circuit.h(0)
circuit.cx(0,1)
circuit.rz(1.555, 0)
circuit.measure_all()

qjob0 = run(circuit, qmio, shots = 100)
qjob1 = run(circuit, qmio, shots = 100)

result0 = qjob0.result
result1 = qjob1.result

print(f"Result from QMIO: {result0}")
print(f"Result from QMIO: {result1}")

qdrop(family)