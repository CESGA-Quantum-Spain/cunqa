import numpy as np
import time
import json
import warnings, logging
import os,sys
sys.path.append(os.getenv("HOME"))
from cunqa.converters import convert
from cunqa import get_QPUs, gather
from cunqa.circuit import CunqaCircuit
from qmiotools.integrations.qiskitqmio import FakeQmio
from qiskit import transpile
backend = FakeQmio("/opt/cesga/qmio/hpc/calibrations/2025_04_02__12_00_02.json", gate_error=True, thermal_relaxation=True, readout_error = True, logging_level=logging.NOTSET )
import argparse
from cunqa import transpiler
from qiskit import transpile
from cunqa.qutils import qraise



parser = argparse.ArgumentParser(description="Quantum Optimization Script")
parser.add_argument("--num_qpus", type=int, required=True, help="Number of QPUs")
parser.add_argument("--cores", type=int, required=True, help="Number of qubits")
logging.getLogger("stevedore.extension").setLevel(logging.CRITICAL)
# Oculta solo este deprecado concreto
warnings.filterwarnings(
    "ignore",
    message=r".*BackendConfiguration.*",
    category=DeprecationWarning,
)
args = parser.parse_args()

num_qpus = int(args.num_qpus)
cores = int(args.cores)

print(f"\nEXECUTING PROGRAM FOR {num_qpus} QPUs with {cores} cores/QPU.")

# creating QFT circuit for n qubits

def QFT(n):
    qft = CunqaCircuit(n)
    for i in range(n):
        qft.h(n-1-i)
        for j in range(0,n-1-i)[::-1]:
            qft.cp(np.pi/(2**(n-1-i-j)), j, n-1-i)
    for i in range(0,int(n/2)):
        qft.swap(i,n-1-i)
    return qft

def IQFT(n):
    iqft = CunqaCircuit(n)
    for i in range(0, int(n/2)):
        iqft.swap(i,n-1-i)
    for i in range(n):
        for j in range(i):
            iqft.cp(-np.pi/(2**(i-j)), j, i)
        iqft.h(i)
    return iqft

def merge_circuits(circ1, circ2):
    circuit = circ1.from_instructions(circ2.instructions)
    return circuit

n = 12

complete_circuit = QFT(n)
complete_circuit.measure_all()

QPUs = get_QPUs(local = False, family=f"QFT-{num_qpus}")

transpiled_circuit = transpiler(complete_circuit,QPUs[0].backend, initial_layout=[30, 31, 28, 29, 23, 12, 9, 14, 10, 20, 11, 27]) # , 19, 24, 18, 17

shots = 1e7

par_shots = [shots // num_qpus for i in range(num_qpus)]

par_shots[0] += shots % num_qpus

print("Shots partitions: ", par_shots, "\n")

qjobs = []

tick = time.time()
for par,qpu in zip(par_shots,QPUs):
    qjobs.append(qpu.run(transpiled_circuit, shots = par, method = "statevector"))

results = gather(qjobs)
tack = time.time()

total_time = tack-tick

mean_simulation_time = sum([result.time_taken for result in results])/len(results)

maximum_simulation_time = max([result.time_taken for result in results])

sequeltial_time = sum([result.time_taken for result in results])

fidelity = 0
for result in results:
    try:
        fidelity += result.counts["0" * n] / shots

    except KeyError:
        pass


result = {
    "num_qpus":num_qpus,
    "total_time":total_time,
    "mean_simulation_time":mean_simulation_time,
    "maximum_simulation_time":maximum_simulation_time,
    "overhead":abs(total_time-maximum_simulation_time),
    "qubits":n,
    "fidelity":fidelity
}

with open(f"results/QFT_n{n}_{num_qpus}QPUs.json", "w") as f:
    json.dump(result, f, indent=2)