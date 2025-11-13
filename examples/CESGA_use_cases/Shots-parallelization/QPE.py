import numpy as np
import time
import json
import warnings, logging
import os,sys
sys.path.append(os.getenv("HOME"))
from cunqa.circuit.converters import convert
from cunqa import get_QPUs, gather
from cunqa.circuit import CunqaCircuit
from qmiotools.integrations.qiskitqmio import FakeQmio
from qiskit import transpile
backend = FakeQmio("/opt/cesga/qmio/hpc/calibrations/2025_04_02__12_00_02.json", gate_error=True, thermal_relaxation=True, readout_error = True, logging_level=logging.NOTSET)
import argparse
from cunqa import transpiler
from qiskit import transpile
from cunqa.qutils import qraise

parser = argparse.ArgumentParser(description="Quantum Optimization Script")
parser.add_argument("--num_qpus", type=int, required=True, help="Number of QPUs")
logging.getLogger("stevedore.extension").setLevel(logging.CRITICAL)
# Oculta solo este deprecado concreto
warnings.filterwarnings(
    "ignore",
    message=r".*BackendConfiguration.*",
    category=DeprecationWarning,
)
args = parser.parse_args()

num_qpus = int(args.num_qpus)

print(f"\nEXECUTING PROGRAM FOR {num_qpus} QPU.")

# creating QFT circuit for n qubits

def IQFT(n):
    iqft = CunqaCircuit(n)
    for i in range(0, int(n/2)):
        iqft.swap(i,n-1-i)
    print("\tAdded swaps...")
    for i in range(n):
        for j in range(i):
            iqft.cp(-np.pi/(2**(i-j)), j, i)
        iqft.h(i)
    return iqft


def merge_circuits(circ1, circ2):
    circuit = circ1.from_instructions(circ2.instructions)
    return circuit

def QPE(phase, n):
    qpe = CunqaCircuit(n+1, n)

    qpe.x(n)

    for i in range(n):
        qpe.h(i)

    print("Added hadamard gates...")
    
    for i in range(n):
        rep = 2**i
        for j in range(rep):
            qpe.crz(phase, i, n)
    
    print("Added rzz gates...")
    
    qpe = merge_circuits(qpe, IQFT(n))

    print("Added IQFT...")

    for i in range(n):
        qpe.measure(i,i)
    
    return qpe


n = 6
result = []
for theta in [2**(-10), 1/np.pi]:

    theta = 2*np.pi * theta * 2

    print("Creating circuit...")
    complete_circuit = QPE(theta,n)

    print(complete_circuit.instructions)

    print("Circuit created!")

    QPUs = get_QPUs(on_node = False, family=f"QPE-{num_qpus}")

    print("QPUs: ", QPUs)

    # print(complete_circuit.instructions)

    shots = 1e5

    par_shots = [shots // num_qpus for i in range(num_qpus)]

    par_shots[0] += shots % num_qpus

    print("Shots partitions: ", par_shots, "\n")

    qjobs = []

    tick = time.time()
    for par,qpu in zip(par_shots,QPUs):
        qjobs.append(qpu.run(complete_circuit, shots = par, method = "statevector"))

    results = gather(qjobs)
    tack = time.time()

    total_counts = {}

    for r in results:
        for key, count in r.counts.items():
            if key in total_counts:
                total_counts[key]+= count
            else:
                total_counts[key] = count

    total_time = tack-tick

    mean_simulation_time = sum([result.time_taken for result in results])/len(results)

    maximum_simulation_time = max([result.time_taken for result in results])

    sequeltial_time = sum([result.time_taken for result in results])


    result.append( {
        "num_qpus":num_qpus,
        "total_time":total_time,
        "mean_simulation_time":mean_simulation_time,
        "maximum_simulation_time":maximum_simulation_time,
        "overhead":abs(total_time-sequeltial_time),
        "qubits":n,
        "shots":shots,
        "theta":theta,
        "counts":total_counts
    })

result = {"theta1":result[0], "theta2":result[1]}

os.makedirs("./results_QPE", exist_ok=True)

with open(f"./results_QPE/QPE{n}_{num_qpus}QPUs.json", "w") as f:
    json.dump(result, f, indent=2)
