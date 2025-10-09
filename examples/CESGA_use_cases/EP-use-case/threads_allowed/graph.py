import os
import json
import re
import sys
import argparse
import matplotlib.pyplot as plt
parser = argparse.ArgumentParser(description="Quantum Optimization Script")
parser.add_argument("--cores", type=int, required=True, help="Number of qubits")
args = parser.parse_args()
cores = args.cores

if cores is not None:
    results_dir = f"results_{cores}"
else:
    results_dir = "results"

modes = [d for d in os.listdir(results_dir) if os.path.isdir(os.path.join(results_dir, d))]

print(modes)

plt.figure(figsize=(8, 6))

for mode in modes:
    mode_path = os.path.join(results_dir, mode)
    qubits = []
    times = []
    for fname in os.listdir(mode_path):
        if fname.endswith(".json"):
            match = re.match(r"vqe-{}_(\d+)\.json".format(re.escape(mode)), fname)
            if not match:
                continue
            num_qubits = int(match.group(1))
            with open(os.path.join(mode_path, fname), "r") as f:
                data = json.load(f)
            time_taken = data.get("simulation_time")
            if time_taken is not None:
                qubits.append(num_qubits)
                times.append(time_taken)
    if qubits and times:
        qubits, times = zip(*sorted(zip(qubits, times)))
        plt.plot(qubits, times, marker='o', label=mode)

# Eje x principal: CPUs
plt.xlabel("Number of qubits")
# Eje x secundario: Qubits

plt.yscale("log")
plt.ylabel("Tiempo total (s)")
plt.title(f"{cores} Cores")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
plt.savefig(f"time_nqubits_{cores}.png", dpi = 400)