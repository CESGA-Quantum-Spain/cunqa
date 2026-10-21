"""
Benchmark of the execution time of the Quantum Phase Estimation (QPE) under different distribution
schemes. It reproduces the following configurations:

    =================  ========  ==========  =========================================
    Configuration      # vQPUs   Shots/vQPU  Algorithm
    =================  ========  ==========  =========================================
    nocomm_1           1         10^8        Textbook QPE (QFT^dagger), single vQPU
    nocomm_128         128       781250      Textbook QPE, shots split among vQPUs
    clcomm             16        10^8        Iterative QPE with classical comms
    qucomm             2         10^4        QPE with telegates (quantum comms)
    =================  ========  ==========  =========================================

All of them estimate PHASE_TO_COMPUTE = 1/pi with N_BITS = 16 bits of precision. For each
configuration the circuits are executed ``--reps`` times on the same deployed vQPUs and the wall
time of ``run`` + ``gather`` is measured. Mean, standard deviation, median, min and max are reported,
and all raw times are dumped to a JSON file.

Usage examples:
    python distributed_QPE_timings.py --config nocomm_1 --reps 10
    python distributed_QPE_timings.py --config clcomm --reps 5 --time 1-00:00:00
    python distributed_QPE_timings.py --config all --reps 10
"""
import os, sys
import time
import json
import argparse
import statistics
import numpy as np

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.qc_protocols import cat_entangler, cat_disentangler
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

# Global variables
N_BITS = 16                 # Bits of precision of the estimated phase
PHASE_TO_COMPUTE = 1 / np.pi
SEED = 18                   # Base seed; repetition r uses SEED + r

# Default parameters of each configuration (all of them can be overridden from the command line)
CONFIGS = {
    "nocomm_1":   {"n_qpus": 1,   "shots": 10**8,  "time": "04:00:00"},
    "nocomm_128": {"n_qpus": 128, "shots": 781250, "time": "01:00:00",
                   "qpus_per_node": 32},  # 64-core nodes, 2 cores per vQPU
    "clcomm":     {"n_qpus": 16,  "shots": 10**8,  "time": "2-00:00:00"},
    "qucomm":     {"n_qpus": 2,   "shots": 10**4,  "time": "2-00:00:00",
                   "cores_per_qpu": 4, "mem_per_qpu": 60},
}


# ------------------------------------------------------------------------------------------------
# Circuit design
# ------------------------------------------------------------------------------------------------
def qft_dagger_and_measure(circuit, qubits, n_bits):
    """Swaps + inverse QFT + measurement over ``qubits``, same convention as qu_comm example 04."""
    for i in range(n_bits // 2):
        circuit.swap(qubits[i], qubits[n_bits - 1 - i])

    for i in range(n_bits):
        for j in range(i):
            angle = (-np.pi) / (2**(i - j))
            circuit.crz(angle, qubits[n_bits - 1 - j], qubits[n_bits - 1 - i])
        circuit.h(qubits[n_bits - 1 - i])

    for i in range(n_bits):
        circuit.measure(qubits[i], i)


def nocomm_circuit(n_bits, phase, circuit_id="qpe"):
    """Textbook QPE in a single circuit: n_bits ancilla qubits + 1 register qubit (the last one)."""
    circuit = CunqaCircuit(n_bits + 1, n_bits, id=circuit_id)
    ancillas = list(range(n_bits))
    register = n_bits

    circuit.x(register) # Rz eigenvector
    for i in range(n_bits):
        circuit.h(ancillas[i])

    for i in range(n_bits):
        param = (2**i) * 2 * 2 * np.pi * phase
        circuit.crz(param, ancillas[n_bits - 1 - i], register)

    qft_dagger_and_measure(circuit, ancillas, n_bits)
    return circuit


def clcomm_circuits(n_bits, phase):
    """Iterative QPE distributed with classical communications (cl_comm example 03)."""
    circuits = []
    for i in range(n_bits):
        # U = Rz(2*theta) with theta = 2*pi*phase. Subcircuit i applies the controlled
        # U^(2^(n_bits-1-i)), i.e. crz(2*theta*2^(n_bits-1-i))
        theta = 2 * np.pi * phase
        angle = 2**(n_bits - 1 - i) * 2 * theta

        circuits.append(CunqaCircuit(2, 2, id=f"cc_{i}"))
        circuits[i].h(0)
        circuits[i].x(1)
        circuits[i].crz(angle, 0, 1)

        for j in range(i):
            param = -np.pi * 2**(-j - 1)
            recv_id = i - j - 1

            circuits[i].recv(0, sending_circuit=f"cc_{recv_id}")

            # Gate conditioned by the received bit
            circuits[i].cif(clbits=0)
            circuits[i].rz(param, 0)
            circuits[i].endcif()

        circuits[i].h(0)

        circuits[i].measure(0, 0)
        for j in range(n_bits - i - 1):
            circuits[i].send(0, recving_circuit=f"cc_{i + j + 1}")

        circuits[i].measure(1, 1)
    return circuits


def qucomm_circuits(n_bits, phase):
    """QPE distributed with telegates (qu_comm example 04)."""
    ancilla_circuit  = CunqaCircuit((n_bits, 1), n_bits, id="ancilla_circuit")
    register_circuit = CunqaCircuit((1, 1), 1, id="register_circuit")

    data_qubits_anc, comm_qubits_anc = ancilla_circuit.get_qubits()
    data_qubits_reg, comm_qubits_reg = register_circuit.get_qubits()

    register_circuit.x(data_qubits_reg[0]) # Rz eigenvector

    for i in range(n_bits):
        ancilla_circuit.h(data_qubits_anc[i])

    for i in range(n_bits):
        cat_entangler(
            target_circuits = [ancilla_circuit, register_circuit],
            data_qubit      = data_qubits_anc[n_bits - 1 - i],
            comm_qubits     = [comm_qubits_anc[0], comm_qubits_reg[0]],
            clbits          = [0, 0],
            tag             = f"telegate_{i}"
        )

        param = (2**i) * 2 * 2 * np.pi * phase
        register_circuit.crz(param, comm_qubits_reg[0], data_qubits_reg[0])

        cat_disentangler(
            target_circuits = [ancilla_circuit, register_circuit],
            data_qubit      = data_qubits_anc[n_bits - 1 - i],
            comm_qubits     = [comm_qubits_reg[0]],
            recv_clbits     = [0],
            send_clbits     = [0]
        )

    qft_dagger_and_measure(ancilla_circuit, data_qubits_anc, n_bits)
    return [ancilla_circuit, register_circuit]


# ------------------------------------------------------------------------------------------------
# Post-processing
# ------------------------------------------------------------------------------------------------
def phase_from_bitstring(bitstring):
    """Bitstring as returned by the QFT-based circuits: leftmost char is the 2^-1 bit."""
    n = len(bitstring)
    return sum(1 / 2**(n - i) for i, digit in enumerate(bitstring) if digit == '1')


def estimate_nocomm(results):
    # Merge the counts of all vQPUs (shots were split among them)
    total = {}
    for result in results:
        for key, value in result.counts.items():
            total[key] = total.get(key, 0) + value
    return phase_from_bitstring(max(total, key=total.get))


def estimate_clcomm(results):
    binary_string = ""
    for result in results:
        counts = result.counts
        most_frequent_output = max(counts, key=counts.get)
        binary_string += most_frequent_output[1]

    estimated_theta = 0.0
    for i, digit in enumerate(reversed(binary_string)):
        if digit == '1':
            estimated_theta += 1 / (2**(i + 1))   # last subcircuit gives the 2^-1 bit
    return estimated_theta


def estimate_qucomm(results):
    counts = results[0].counts
    return phase_from_bitstring(max(counts, key=counts.get))


# ------------------------------------------------------------------------------------------------
# Benchmark
# ------------------------------------------------------------------------------------------------
def write_backend(n_data, n_comm):
    """The default backend has too few qubits for the 16-bit QPE, so a custom one is written."""
    name = f"qpe_backend_{n_data}_{n_comm}"
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"{name}.json")
    with open(path, "w") as f:
        json.dump({
            "name": name,
            "description": f"Backend with {n_data} data qubits and {n_comm} communication qubits.",
            "num_qubits": [n_data, n_comm]
        }, f, indent=4)
    return path


def write_qucomm_infrastructure(n_bits):
    """
    The quantum-comm executor simulates the sum of the qubits of all the vQPU backends, so giving
    both vQPUs the [n_bits, 1] backend would mean 2*(n_bits + 1) qubits. Instead, each vQPU gets a
    backend sized for its own circuit.
    """
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"qpe_infrastructure_{n_bits}.json")
    with open(path, "w") as f:
        json.dump({"backends": [write_backend(n_bits, 1), write_backend(1, 1)]}, f, indent=4)
    return path


def stats(values):
    return {
        "mean":   statistics.mean(values),
        "std":    statistics.stdev(values) if len(values) > 1 else 0.0,
        "median": statistics.median(values),
        "min":    min(values),
        "max":    max(values),
    }


def benchmark(config, args):
    cfg = dict(CONFIGS[config])
    for key in ("n_qpus", "shots", "time", "cores_per_qpu", "mem_per_qpu", "n_nodes",
                "qpus_per_node"):
        if getattr(args, key) is not None:
            cfg[key] = getattr(args, key)
    n_qpus, shots = cfg["n_qpus"], cfg["shots"]

    if config.startswith("nocomm"):
        # One copy per vQPU with its own id, since results are matched by circuit id
        circuits = [nocomm_circuit(N_BITS, PHASE_TO_COMPUTE, f"qpe_{k}") for k in range(n_qpus)]
        estimate = estimate_nocomm
        raise_kwargs = {"backend": write_backend(N_BITS + 1, 0)}
    elif config == "clcomm":
        circuits, estimate = clcomm_circuits(N_BITS, PHASE_TO_COMPUTE), estimate_clcomm
        raise_kwargs = {"classical_comm": True}
    else:
        circuits, estimate = qucomm_circuits(N_BITS, PHASE_TO_COMPUTE), estimate_qucomm
        raise_kwargs = {"quantum_comm": True, "infrastructure": write_qucomm_infrastructure(N_BITS)}

    if len(circuits) != n_qpus:
        raise ValueError(f"Configuration {config} needs {len(circuits)} vQPUs, got {n_qpus}.")

    print(f"\n=== {config}: {n_qpus} vQPUs, {shots} shots/vQPU, {args.reps} repetitions ===")

    # 1. QPU deployment. Without communications the vQPUs are independent, so they are raised as one
    # single-node family per group of qpus_per_node, one after another: vQPUs of several nodes
    # registering at the same time in qpus.json (on NFS) lose entries and qraise never returns.
    if config.startswith("nocomm") and cfg.get("qpus_per_node"):
        group_sizes = [min(cfg["qpus_per_node"], n_qpus - i)
                       for i in range(0, n_qpus, cfg["qpus_per_node"])]
    else:
        group_sizes = [n_qpus]

    families, wall_times, sim_times, phases = [], [], [], []
    try:
        for g, group_size in enumerate(group_sizes):
            families.append(qraise(group_size, cfg["time"],
                                   simulator     = args.simulator,
                                   co_located    = True,
                                   cores_per_qpu = cfg.get("cores_per_qpu"),
                                   mem_per_qpu   = cfg.get("mem_per_qpu"),
                                   n_nodes       = cfg.get("n_nodes"),
                                   partition     = args.partition,
                                   family        = f"qpe_bench_{config}_{g}",
                                   **raise_kwargs))

        qpus = [qpu for family in families for qpu in get_QPUs(co_located=True, family=family)]
        if config == "qucomm":
            # The ancilla circuit must go to the vQPU with the biggest backend
            qpus.sort(key=lambda qpu: qpu.backend["num_qubits"][0], reverse=True)

        def execute(seed):
            if config.startswith("nocomm"):
                # Independent vQPUs: each one needs its own seed, otherwise all of them would
                # sample exactly the same shots
                jobs = [run(circuit, qpu, shots=shots, seed=seed * n_qpus + k)
                        for k, (circuit, qpu) in enumerate(zip(circuits, qpus))]
            else:
                jobs = run(circuits, qpus, shots=shots, seed=seed)
            return gather(jobs)

        # 2. Warm-up executions (not measured)
        for w in range(args.warmup):
            execute(SEED - 1 - w)

        # 3. Measured executions
        for rep in range(args.reps):
            start = time.perf_counter()
            results = execute(SEED + rep)
            wall = time.perf_counter() - start

            # Simulator-reported time: the slowest vQPU determines the makespan
            sim = max(float(result.time_taken) for result in results)
            phase = estimate(results)

            wall_times.append(wall)
            sim_times.append(sim)
            phases.append(phase)
            print(f"  rep {rep + 1}/{args.reps}: wall = {wall:.3f} s, "
                  f"simulator = {sim:.3f} s, phase = {phase}")
    finally:
        # 4. Release resources
        if families:
            qdrop(*families)

    return {
        "config": config,
        "n_qpus": n_qpus,
        "shots_per_qpu": shots,
        "reps": args.reps,
        "estimated_phases": phases,
        "wall_times": wall_times,
        "simulator_times": sim_times,
        "wall_stats": stats(wall_times),
        "simulator_stats": stats(sim_times),
    }


def main():
    parser = argparse.ArgumentParser(description="Execution time of the distributed QPE.")
    parser.add_argument("--config", choices=list(CONFIGS) + ["all"], default="all")
    parser.add_argument("--reps", type=int, default=10, help="Measured repetitions per config.")
    parser.add_argument("--warmup", type=int, default=0, help="Non-measured initial executions.")
    parser.add_argument("--n-qpus", dest="n_qpus", type=int, default=None)
    parser.add_argument("--shots", type=int, default=None, help="Shots per vQPU.")
    parser.add_argument("--time", default=None, help="qraise time limit, 'D-HH:MM:SS'.")
    parser.add_argument("--cores-per-qpu", dest="cores_per_qpu", type=int, default=None)
    parser.add_argument("--mem-per-qpu", dest="mem_per_qpu", type=int, default=None, help="GB.")
    parser.add_argument("--n-nodes", dest="n_nodes", type=int, default=None)
    parser.add_argument("--qpus-per-node", dest="qpus_per_node", type=int, default=None)
    parser.add_argument("--partition", default=None)
    parser.add_argument("--simulator", default="Aer")
    parser.add_argument("--output", default=None, help="JSON file with the raw results.")
    args = parser.parse_args()

    configs = list(CONFIGS) if args.config == "all" else [args.config]
    summaries = []
    for config in configs:
        summaries.append(benchmark(config, args))
        # Save after each configuration so that partial results are not lost
        output = args.output or f"qpe_timings_{args.config}.json"
        with open(output, "w") as f:
            json.dump(summaries, f, indent=4)

    print(f"\nReal phase: {PHASE_TO_COMPUTE}")
    print(f"{'Config':<12}{'vQPUs':>7}{'Shots/vQPU':>12}{'Phase':>22}"
          f"{'Mean (s)':>12}{'Std (s)':>11}{'Median (s)':>12}{'Min (s)':>11}{'Max (s)':>11}")
    for s in summaries:
        w = s["wall_stats"]
        phase = statistics.mode(s["estimated_phases"])
        print(f"{s['config']:<12}{s['n_qpus']:>7}{s['shots_per_qpu']:>12}{phase:>22}"
              f"{w['mean']:>12.3f}{w['std']:>11.3f}{w['median']:>12.3f}"
              f"{w['min']:>11.3f}{w['max']:>11.3f}")
    print(f"\nRaw results saved to {output}")


if __name__ == "__main__":
    main()
