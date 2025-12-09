import os
import re
import json
import matplotlib.pyplot as plt

n = 16

def parse_filename(filename):
    match = re.match(r"QPE"+str(n)+r"_(\d+)QPUs\.json", filename, re.IGNORECASE)
    if match:
        num_qpus = int(match.group(1))
        return num_qpus
    return None

directory = "/mnt/netapp1/Store_CESGA/home/cesga/mlosada/api/api-simulator/examples/CESGA_use_cases/Shots-parallelization/results_QPE/"

num_qpus_list = []
time_taken_list = []

num_qpus_list_ = []
time_taken_list_ = []

for filename in os.listdir(directory):
    num_qpus = parse_filename(filename)
    if num_qpus is not None:
        filepath = os.path.join(directory, filename)
        with open(filepath, "r") as f:
            data = json.load(f)
            for k, v in data.items():
                time_taken = v.get("total_time")
                if time_taken is not None:
                    if k == "theta1":
                        num_qpus_list.append(num_qpus)
                        time_taken_list.append(time_taken)
                    elif k == "theta2":
                        num_qpus_list_.append(num_qpus)
                        time_taken_list_.append(time_taken)

# Ordenar por num_qpus
sorted_pairs = sorted(zip(num_qpus_list, time_taken_list))
sorted_pairs_ = sorted(zip(num_qpus_list_, time_taken_list_))

num_qpus_sorted, time_taken_sorted = zip(*sorted_pairs)
num_qpus_sorted_, time_taken_sorted_ = zip(*sorted_pairs_)

# study of acceleration


acceleration = [time_taken_sorted[0]/time for time in time_taken_sorted]
acceleration_ = [time_taken_sorted_[0]/time for time in time_taken_sorted_]

fig, ax1 = plt.subplots(figsize=(6, 4))

# --- Eje izquierdo: simulation time ---
ax1.plot(num_qpus_sorted, time_taken_sorted, marker='*', markersize = 6, linewidth = 0.9, color="navy", label=r"$\phi = 1/\pi$ (time)")
ax1.plot(num_qpus_sorted_, time_taken_sorted_, marker='o', markersize = 4, linewidth = 0.9, color="navy", label=r"$\phi = 1/\pi$ (time)")
ax1.set_xlabel('Number of QPUs')
ax1.set_ylabel('Simulation time (s)', color = "navy")
for label in ax1.get_yticklabels():
    label.set_color("navy")
ax1.set_yticks([50,100,150,200,250,300], color="navy")
ax1.set_xticks([i for i in range(0, 130, 16)])
ax1.tick_params(axis="x", labelsize=10)
ax1.grid(True)

# --- Eje derecho: acceleration ---
ax2 = ax1.twinx()
ax2.plot(num_qpus_sorted, acceleration, marker='*', markersize = 6, linewidth = 0.9, linestyle='--', color="red", label=r"$\theta_1$ (accel.)")
ax2.plot(num_qpus_sorted_, acceleration_, marker='o', markersize = 4, linewidth = 0.9, linestyle='--', color="red", label=r"$\theta_2$ (accel.)")
ax2.plot(num_qpus_sorted, num_qpus_sorted, "k--", label = "Ideal acceleration")
ax2.set_ylabel('Acceleration', color = "red")
for label in ax2.get_yticklabels():
    label.set_color("red")
ax2.set_yticks([i for i in range(0, 130, 16)], color="red")

# --- Leyenda combinada ---
lines1, labels1 = ax1.get_legend_handles_labels()
lines2, labels2 = ax2.get_legend_handles_labels()
ax1.legend(lines1 + lines2, labels1 + labels2, loc=(0.12,0.6))
plt.tight_layout()
plt.savefig(f"QPE_{n}_qubits.png", dpi=300)
plt.show()

### JUST ONE THETA SINCE IT IS REDUNDANT

fig, ax1 = plt.subplots(figsize=(6, 4))
ax1.plot(num_qpus_sorted, time_taken_sorted, marker='o', markersize = 4, color="navy")
ax1.set_xlabel('Number of vQPUs')
ax1.set_ylabel('Simulation time (s)', color = "navy")
ax1.set_xticks([i for i in range(0, 130, 16)])
ax1.tick_params(axis="x", labelsize=10)
for label in ax1.get_yticklabels():
    label.set_color("navy")
ax1.set_yticks([50,100,150,200,250], color="navy")
ax1.grid(True)

# --- Eje derecho: acceleration ---
ax2 = ax1.twinx()
ax2.plot(num_qpus_sorted, acceleration, marker='*', markersize = 6, linestyle='-', color="red")
ax2.plot(num_qpus_sorted, num_qpus_sorted, "k--", label = "Ideal acceleration")
ax2.set_ylabel('Acceleration', color = "red")
for label in ax2.get_yticklabels():
    label.set_color("red")
ax2.set_yticks([i for i in range(0, 130, 16)], color="red")

lines1, labels1 = ax1.get_legend_handles_labels()
lines2, labels2 = ax2.get_legend_handles_labels()
ax1.legend(lines1 + lines2, labels1 + labels2, loc=(0.1,0.88))
plt.tight_layout()
plt.savefig(f"QPE_{n}_qubits_NEW.pdf", dpi=500)
plt.show()