from utils.utils import Ising_Hamiltonian, hardware_efficient_ansatz, estimate_observable
from multiprocessing import Pool
from qiskit import transpile
from qiskit_aer import AerSimulator
from qmiotools.integrations.qiskitqmio import FakeQmio
from scipy.optimize import differential_evolution
import numpy as np
import time
import json
import os

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

Hamiltonian = Ising_Hamiltonian(n = n, J = 1)

parametric_ansatz = hardware_efficient_ansatz(num_qubits=n, num_layers=2)

transpiled_parametric_ansatz = transpile(parametric_ansatz, backend, optimization_level = 3, seed_transpiler = 34)

print(transpiled_parametric_ansatz.num_parameters)

estimation_times = []

def cost_function(params):

    assembled_circuit = transpiled_parametric_ansatz.assign_parameters(params)

    job = backend.run(assembled_circuit, shots = 1e4, seed = 34, max_parallel_shots = 0, method = "statevector")

    print(os.getenv("OMP_NUM_THREADS"))

    result = job.result()

    counts = result.get_counts()

    print(f"Simulation time: ", result.time_taken)

    tick = time.time()

    obs = estimate_observable(Hamiltonian, counts)

    tack = time.time()

    estimation_times.append(tack-tick)

    return obs



bounds=[]
for i in range(transpiled_parametric_ansatz.num_parameters):
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

result = differential_evolution(func = cost_function,
                                            popsize=1,
                                            strategy='best1bin',
                                            bounds=bounds,
                                            maxiter=1,
                                            disp=True,
                                            init="halton", # halton, x0
                                            polish=False,
                                            tol=1e-50,
                                            seed=33,
                                            callback=cb,
                                            updating='deferred')


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