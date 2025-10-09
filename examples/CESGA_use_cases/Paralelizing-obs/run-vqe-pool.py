from utils.utils import Heisenberg_Hamiltonian, hardware_efficient_ansatz, estimate_observable, qwc_circuits
from multiprocessing.pool import Pool
from qiskit import transpile
from qiskit_aer import AerSimulator
from qmiotools.integrations.qiskitqmio import FakeQmio
from scipy.optimize import differential_evolution
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

        job = backend.run(assembled_circuit, shots = 1e4, seed = 34, method = "statevector", max_parallel_threads = 1)

        result = job.result()

        counts = result.get_counts()

        print(f"Simulation time for {obs}: ", result.time_taken)

        tick = time.time()

        energy+=estimate_observable(Hamiltonian,obs, counts)

        tack = time.time()

        estimation_times.append(tack-tick)

    return energy


bounds=[]
for i in range(transpiled_parametric_ansatzes[0].num_parameters):
    bounds.append((-np.pi,np.pi))


opt_path = []; params_path = []

def cb(xk, convergence = 1e-50):
    print(f"({time.strftime('%H:%M:%S')})")
    cost = cost_function(xk)
    params_path.append(list(xk))
    opt_path.append(cost)


# mapper = Pool(transpiled_parametric_ansatz.num_parameters).map

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
                                            seed=34,
                                            callback=cb,
                                            updating='deferred',
                                            workers = transpiled_parametric_ansatz.num_parameters)


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
    "simulation_time":(time_taken-sum(estimation_times)/len(estimation_times))
}

with open(f"results_{cores}/pool/vqe-pool_{n}.json", "w") as f:
    json.dump(result, f, indent=2)