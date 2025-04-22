import os
import sys
import json
import time

install_path = os.getenv("INSTALL_PATH")
sys.path.insert(0, install_path)

from cunqa.circuit import CunqaCircuit

from qiskit import QuantumRegister,ClassicalRegister

QR1 = QuantumRegister(2); CR1 = ClassicalRegister(2)
qc1 = CunqaCircuit("circuit_1", QR1, CR1)

qc2 = CunqaCircuit("circuit_2", 3, 3)



qc1.x(0)

qc2.h(1)

# = ========

qc1.send_gate("z", control_qubit=0, target_circuit="circuit_2", target_qubit=1)

qc2.rcv_gate("z", target_qubit=1, control_circuit="circuit_1", control_qubit=0)

# ===========

qc1.measure(QR1,CR1)

qc2.measure_all()



print(f"cunqa info cirtuit 1: ")
for q in qc1.cunqa_info["instructions"]:
    print(q)
print()
print(f"cunqa info cirtuit 2: ")
for q in qc2.cunqa_info["instructions"]:
    print(q)

# run_distributed([qc1,qc2], qpus)