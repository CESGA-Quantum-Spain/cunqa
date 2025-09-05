import os, sys
# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa import get_QPUs, gather
from cunqa.circuit import CunqaCircuit
from cunqa.converters import convert, _cunqac_to_qc, _qc_to_qasm
from qiskit.qasm3 import dumps as dumps3

from qiskit import QuantumCircuit

# qraise -t 00:10:00 --qmio
qpus = get_QPUs(local=False)
qmio = qpus[0]

circuit = CunqaCircuit(2,2)
circuit.h(0)
circuit.cx(0,1)
circuit.measure_all()

qjob = qmio.run(circuit, shots = 100)
counts = qjob.result.counts
time = qjob.time_taken

print(f"Result: \n{counts}\n Time taken: {time} s.")