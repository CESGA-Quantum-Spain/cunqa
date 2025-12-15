import os, sys
sys.path.append(os.getenv("HOME"))

from cunqa import  get_QPUs
from cunqa.circuit import CunqaCircuit
from cunqa.qutils import qraise, qdrop


from qiskit import QuantumCircuit

""" family = qraise(1, "00:10:00", qmio = True)
qpus = get_QPUs(on_node = False)
qmio = qpus[0] """

circuit = CunqaCircuit(2,4)
circuit.h(0)
circuit.cx(0,1)
circuit.rz(1.555, 0)
circuit.measure_all()

print(circuit.info)

""" qjob0 = qmio.run(circuit, shots = 100)
qjob1 = qmio.run(circuit, shots = 100)

result0 = qjob0.result
result1 = qjob1.result

print(f"Result from QMIO: {result0}")
print(f"Result from QMIO: {result1}")

qjob0.upgrade_parameters([1])

qdrop(family) """