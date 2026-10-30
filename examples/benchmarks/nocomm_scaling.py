"""
Strong scaling of the QPE without communications: a fixed total number of shots is split among an
increasing number of independent vQPUs, and the execution time and the speedup with respect to a
single vQPU are measured.

The vQPUs are deployed once (``--max-qpus`` of them, ``--qpus-per-node`` per node) and each point of
the curve uses the first ``n`` of them, filling one node before moving to the next. For each ``n``
the circuits are executed ``--reps`` times and the wall time of ``run`` + ``gather`` is measured.
Raw times and statistics are dumped to a JSON file and the figure is saved as PNG.

Usage examples:
    python nocomm_scaling.py --reps 10
    python nocomm_scaling.py --plot nocomm_scaling.json   # only redraw the figure
"""
import os, sys
import time
import json
import argparse
import statistics

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run, read_qpus
from cunqa.qjob import gather

from distributed_QPE_timings import (N_BITS, PHASE_TO_COMPUTE, SEED, nocomm_circuit, estimate_nocomm,
                                     write_backend, stats)


def split_shots(total_shots, n):
    """Splits ``total_shots`` among ``n`` vQPUs, giving the remainder to the first ones."""
    base, remainder = divmod(total_shots, n)
    return [base + (1 if k < remainder else 0) for k in range(n)]


def benchmark(args):
    n_values = [1] + list(range(args.step, args.max_qpus + 1, args.step))
    circuits = [nocomm_circuit(N_BITS, PHASE_TO_COMPUTE, f"qpe_{k}") for k in range(args.max_qpus)]

    family = qraise(args.max_qpus, args.time,
                    simulator     = args.simulator,
                    co_located    = True,
                    n_nodes       = -(-args.max_qpus // args.qpus_per_node),
                    qpus_per_node = args.qpus_per_node,
                    partition     = args.partition,
                    family        = "qpe_nocomm_scaling",
                    backend       = write_backend(N_BITS + 1, 0))
    results = []
    try:
        # Sorted by node, so that the first n vQPUs fill one node before using the next
        nodes = {id: info["net"]["nodename"] for id, info in read_qpus(family).items()}
        qpus = sorted(get_QPUs(co_located=True, family=family),
                      key=lambda qpu: (nodes[qpu.id], qpu.id))

        for n in n_values:
            shots = split_shots(args.shots, n)
            print(f"\n=== {n} vQPUs, {args.shots} total shots, {args.reps} repetitions ===")

            def execute(seed):
                # Independent vQPUs: each one needs its own seed, otherwise all of them would
                # sample exactly the same shots
                jobs = [run(circuits[k], qpus[k], shots=shots[k], seed=seed * args.max_qpus + k)
                        for k in range(n)]
                return gather(jobs)

            for w in range(args.warmup):
                execute(SEED - 1 - w)

            wall_times, phases = [], []
            for rep in range(args.reps):
                start = time.perf_counter()
                res = execute(SEED + rep)
                wall = time.perf_counter() - start

                wall_times.append(wall)
                phases.append(estimate_nocomm(res))
                print(f"  rep {rep + 1}/{args.reps}: wall = {wall:.3f} s, phase = {phases[-1]}")

            results.append({"n_qpus": n, "total_shots": args.shots, "wall_times": wall_times,
                            "estimated_phases": phases, "wall_stats": stats(wall_times)})

            # Save after each point so that partial results are not lost
            with open(args.output, "w") as f:
                json.dump(results, f, indent=4)
    finally:
        qdrop(family)

    return results


def plot(results, path):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    n = [r["n_qpus"] for r in results]
    mean = [r["wall_stats"]["mean"] for r in results]
    std = [r["wall_stats"]["std"] for r in results]

    # Speedup with respect to 1 vQPU, with the error propagated from both times
    t1, s1 = mean[0], std[0]
    speedup = [t1 / t for t in mean]
    speedup_err = [sp * ((s1 / t1)**2 + (s / t)**2)**0.5 for sp, t, s in zip(speedup, mean, std)]

    fig, ax_time = plt.subplots(figsize=(10, 6))
    ax_speedup = ax_time.twinx()

    time_line = ax_time.errorbar(n, mean, yerr=std, marker="o", color="tab:blue", capsize=3,
                                 label="Execution time")
    speedup_line = ax_speedup.errorbar(n, speedup, yerr=speedup_err, marker="s", color="tab:red",
                                       capsize=3, label="Measured speedup")
    ideal_line, = ax_speedup.plot(n, n, linestyle="--", color="gray", label="Ideal speedup")

    ax_time.set_yscale("log")
    # Plain 1-2-5 labels instead of powers of ten
    ax_time.yaxis.set_major_locator(matplotlib.ticker.LogLocator(subs=(1, 2, 5)))
    ax_time.yaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(lambda y, _: f"{y:g}"))
    ax_time.yaxis.set_minor_formatter(matplotlib.ticker.NullFormatter())
    ax_time.set_ylim(bottom=1)
    ax_time.set_xlabel("Number of vQPUs")
    ax_time.set_ylabel("Execution time (s)", color="tab:blue")
    ax_time.tick_params(axis="y", labelcolor="tab:blue")
    ax_speedup.set_ylabel("Speedup", color="tab:red")
    ax_speedup.tick_params(axis="y", labelcolor="tab:red")
    ax_time.set_xticks(n)
    ax_time.grid(True, which="both", linestyle=":", alpha=0.7)

    ax_time.legend(handles=[time_line, speedup_line, ideal_line], loc="upper center")

    fig.tight_layout()
    fig.savefig(path, dpi=200)
    print(f"Figure saved to {path}")


def main():
    parser = argparse.ArgumentParser(description="Strong scaling of the QPE without communications.")
    parser.add_argument("--max-qpus", dest="max_qpus", type=int, default=128)
    parser.add_argument("--step", type=int, default=16, help="Points at 1, step, 2*step, ...")
    parser.add_argument("--qpus-per-node", dest="qpus_per_node", type=int, default=32)
    parser.add_argument("--shots", type=int, default=10**8, help="Total shots, split among vQPUs.")
    parser.add_argument("--reps", type=int, default=10, help="Measured repetitions per point.")
    parser.add_argument("--warmup", type=int, default=0, help="Non-measured initial executions.")
    parser.add_argument("--time", default="03:00:00", help="qraise time limit, 'D-HH:MM:SS'.")
    parser.add_argument("--partition", default=None)
    parser.add_argument("--simulator", default="Aer")
    parser.add_argument("--output", default="nocomm_scaling.json", help="JSON file with raw results.")
    parser.add_argument("--figure", default="nocomm_scaling.png")
    parser.add_argument("--plot", default=None, help="Only draw the figure from this JSON file.")
    args = parser.parse_args()

    if args.plot is not None:
        with open(args.plot) as f:
            results = json.load(f)
    else:
        results = benchmark(args)

    print(f"\nReal phase: {PHASE_TO_COMPUTE}")
    print(f"{'vQPUs':>6}{'Mean (s)':>12}{'Std (s)':>11}{'Speedup':>10}")
    for r in results:
        w = r["wall_stats"]
        print(f"{r['n_qpus']:>6}{w['mean']:>12.3f}{w['std']:>11.3f}"
              f"{results[0]['wall_stats']['mean'] / w['mean']:>10.2f}")

    plot(results, args.figure)


if __name__ == "__main__":
    main()
