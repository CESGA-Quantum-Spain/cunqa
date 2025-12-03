import os, sys
sys.path.append(os.getenv("HOME"))

from cunqa import get_QPUs
from cunqa.circuit import CunqaCircuit
from cunqa.qutils import qraise, qdrop

from qiskit import QuantumCircuit

family = qraise(1, "00:10:00", qmio = True)
qpus = get_QPUs(on_node = False)
qmio = qpus[0]

print(qmio)

circuit = QuantumCircuit(2,2)
circuit.h(0)
circuit.cx(0,1)
circuit.measure_all()

result = qmio.run(circuit, shots = 100)

print(f"Result from QMIO: {result}")

qdrop(family)