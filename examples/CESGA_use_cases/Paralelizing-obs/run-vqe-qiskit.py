from utils.utils import Heisenberg_Hamiltonian, hardware_efficient_ansatz, estimate_observable, qwc_circuits
from multiprocessing import Pool
from qiskit import transpile
from qiskit_aer import AerSimulator
from qmiotools.integrations.qiskitqmio import FakeQmio
from scipy.optimize import minimize
import numpy as np
import time
import json

# choosing backend
backend = FakeQmio("/opt/cesga/qmio/hpc/calibrations/2025_04_02__12_00_02.json", gate_error=True, thermal_relaxation=True, readout_error = True)
# describin the VQE problem with a Hardware Efficient Ansatz
import argparse

parser = argparse.ArgumentParser(description="Quantum Optimization Script")
parser.add_argument("--num_qubits", type=int, required=True, help="Number of qubits")
parser.add_argument("--cores", type=int, required=True, help="Number of qubits")

args = parser.parse_args()

n = int(args.num_qubits)
cores = int(args.cores)

Hamiltonian = Heisenberg_Hamiltonian(n = n, Jx=1, Jy=1, Jz=1, h=0)

parametric_ansatz = hardware_efficient_ansatz(num_qubits=n, num_layers=2)

qwc, ops = qwc_circuits(parametric_ansatz, Hamiltonian)

print(ops)

transpiled_parametric_ansatzes = []

for qc in qwc:
    transpiled_parametric_ansatzes.append(transpile(qc, backend, optimization_level = 3, seed_transpiler = 34))


print(parametric_ansatz.num_parameters)

estimation_times = []

def cost_function(params):

    energy = 0

    for qc, obs in zip(transpiled_parametric_ansatzes, ops):

        assembled_circuit = qc.assign_parameters(params)

        job = backend.run(assembled_circuit, shots = 1e4, seed = 34,  method = "statevector")

        result = job.result()

        counts = result.get_counts()

        print(f"Simulation time for {obs}: ", result.time_taken)

        tick = time.time()

        energy+=estimate_observable(Hamiltonian,obs, counts)

        tack = time.time()

        estimation_times.append(tack-tick)

    return energy



bounds=[]
for i in range(parametric_ansatz.num_parameters):
    bounds.append((-np.pi,np.pi))


opt_path = []; params_path = []

def cb(xk, convergence = 1e-50):
    print(xk)
    print(f"({time.strftime('%H:%M:%S')})")
    cost = cost_function(xk)
    individual = xk.tolist()
    params_path.append(individual)
    opt_path.append(cost)


mapper = None

tick = time.time()

x0 = np.random.uniform(-np.pi, np.pi, parametric_ansatz.num_parameters)

result = minimize(fun=cost_function,
                  x0=x0,
                  bounds=bounds,
                  tol=1e-50,
                  callback=cb,
                  options={"maxiter":1,
                           "disp":True,
                           "catol":1e-50},
                  method="COBYLA")


tack = time.time()

if i <= 300:
    best_cost = result.fun
else:
    best_cost = np.mean(opt_path[-100:])

best_params = list(result.x)
time_taken = tack-tick

result = {
    "best_cost":best_cost,
    "best_params":best_params,
    "opt_path":opt_path,
    "params_path":params_path,
    "time_taken":time_taken,
    "n_steps":len(opt_path),
    "simulation_time":(time_taken-sum(estimation_times))
}

print(result)

with open(f"results_{cores}/qiskit/vqe-qiskit_{n}.json", "w") as f:
    json.dump(result, f, indent=2)