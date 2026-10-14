#!/usr/bin/env python3
"""
Communication noise model checker: runs teledata between two real vQPUs with quantum
communications and compares the outcomes with the analytic predictions of the model.

This is a SYSTEM test, not a unit test: it allocates real vQPUs through SLURM (``qraise``)
and goes through the whole stack (``run`` -> vQPUs -> quantum-communications executor). The
physics of the model is unit-tested in ``tests/cpp/test_qc_engine.cpp``; this script checks
that the model reaches the executor both from the backend JSON (deployment) and from the
``comm_model`` run argument, that per-link parameters can be given by circuit id, that the
simulated time follows the timing block of the backend, and that a wrong model is reported
without stopping the executor.

Simulators without Kraus channels (every one but Aer) run the Pauli-twirled model, whose
predictions for these checks are the same, so the script applies to all of them.

Usage
-----
    python check_comm_noise.py
    python check_comm_noise.py --shots 8000
    python check_comm_noise.py --simulator Maestro
"""

import argparse
import json
import math
import os
import sys
import tempfile
import traceback

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME"))  # HOME as install path is specific to CESGA

from cunqa.qpu import qraise, get_QPUs, run, qdrop
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.qc_protocols import qsend, qrecv

# Model deployed with the vQPUs: Werner pairs of fidelity 0.9 delivered instantly, 1-qubit gates
# and resets that take no time, CNOTs and SWAPs of 0.3 us, measurements of 1 us, decisions 0.1 us
# after the bits are known and 2 us of classical latency.
DEPLOYED_FIDELITY = 0.9
BACKEND = {
    "name": "CommNoiseBackend",
    "version": "0.0.1",
    "description": "Two data qubits and one communication qubit with noisy entanglement.",
    "num_qubits": [2, 1],
    "timing": {
        "one_qubit_gate": 0.0,
        "multi_qubit_gate": 3e-7,
        "measure": 1e-6,
        "reset": 0.0,
        "classical_latency": 2e-6,
        "feed_forward_latency": 1e-7,
    },
    "comm_model": {"epr": {"fidelity": DEPLOYED_FIDELITY}},
}
# CNOTs of the local work of the prefetching check: 6 us.
LOCAL_WORK = 20


def geometric_laplace(p, tau, s):
    """E[exp(-s N tau)] for N ~ Geom(p) attempts."""
    e = math.exp(-s * tau)
    return p * e / (1.0 - (1.0 - p) * e)


def teledata(prepare=(), undo=(), prefetch=False, local_work=False):
    """Teleports a prepared state from Sender to Receiver, which undoes the preparation and
    measures. P('0') is the fidelity of the teleported state."""
    sender = CunqaCircuit((2, 1), 2, id="Sender")
    receiver = CunqaCircuit((2, 1), 3, id="Receiver")
    data_s, comm_s = sender.get_qubits()
    data_r, comm_r = receiver.get_qubits()

    if prefetch:
        sender.gen_ent(comm_s[0], "Receiver", "td")
        receiver.gen_ent(comm_r[0], "Sender", "td")
    for _ in range(LOCAL_WORK if local_work else 0):
        sender.cx(data_s[0], data_s[1])
    for gate in prepare:
        getattr(sender, gate)(data_s[0])

    qsend(sender, data_s[0], comm_s[0], [0, 1], "Receiver", tag="td", prefetched=prefetch)
    qrecv(receiver, data_r[0], comm_r[0], [0, 1], "Sender", tag="td", prefetched=prefetch)
    for gate in undo:
        getattr(receiver, gate)(data_r[0])
    receiver.measure(data_r[0], 2)
    return [sender, receiver]


def fidelity(result):
    counts = result.counts
    return counts.get("0", 0) / sum(counts.values())


def run_teledata(qpus, shots, **teledata_args):
    comm_model = teledata_args.pop("comm_model", None)
    run_args = {"shots": shots, "seed": 7}
    if comm_model is not None:
        run_args["comm_model"] = comm_model
    return gather(run(teledata(**teledata_args), qpus, **run_args))


def tolerance(p, shots, sigmas=4.5):
    return sigmas * math.sqrt(max(p * (1 - p), 1e-4) / shots)


def check_deployed_model(qpus, shots):
    expected = (2 * DEPLOYED_FIDELITY + 1) / 3
    results = run_teledata(qpus, shots)
    measured = fidelity(results[1])
    assert abs(measured - expected) < tolerance(expected, shots), (measured, expected)
    assert results[1].simulated_time["min"] == results[1].simulated_time["max"]
    return f"fidelity {measured:.4f}, expected {expected:.4f}"


def check_run_override(qpus, shots):
    expected = (2 * 0.7 + 1) / 3
    measured = fidelity(run_teledata(qpus, shots, comm_model={"epr": {"fidelity": 0.7}})[1])
    assert abs(measured - expected) < tolerance(expected, shots), (measured, expected)
    return f"fidelity {measured:.4f}, expected {expected:.4f}"


def check_link_by_circuit_ids(qpus, shots):
    model = {"links": [{"circuits": ["Receiver", "Sender"], "epr": {"fidelity": 1.0}}]}
    measured = fidelity(run_teledata(qpus, shots, comm_model=model)[1])
    assert measured == 1.0, measured
    return "perfect pairs on the Sender-Receiver link"


def check_fidelity_decreases_with_generation_time(qpus, shots):
    t2, tau = 50e-6, 1e-6
    previous, summary = 1.0, []
    for p in (1.0, 0.2, 0.05):
        model = {
            "epr": {"fidelity": 1.0, "success_probability": p, "attempt_time": tau},
            "data_qubits": {"T2": t2},
            "comm_qubits": {"T1": 1e3, "T2": 1e3},
        }
        expected = 0.5 * (1 + geometric_laplace(p, tau, 1 / t2))
        measured = fidelity(run_teledata(qpus, shots, prepare=("h",), undo=("h",), comm_model=model)[1])
        assert abs(measured - expected) < tolerance(expected, shots), (p, measured, expected)
        assert measured < previous, (p, measured, previous)
        previous = measured
        summary.append(f"p={p}: {measured:.4f} ({expected:.4f})")
    return ", ".join(summary)


def check_simulated_time(qpus, shots):
    # Pair ready at 1 us. Sender: CX [1, 1.3], measurements [1.3, 2.3], corrections of its
    # qubits decided at 2.4. Receiver: corrections decided at 2.3 + 2 + 0.1 = 4.4, SWAP
    # [4.4, 4.7], measurement [4.7, 5.7], in every shot.
    results = run_teledata(qpus, min(shots, 200), comm_model={"epr": {"attempt_time": 1e-6}})
    sender, receiver = (r.simulated_time for r in results)
    assert sender["unit"] == "s", sender
    assert abs(sender["max"] - 2.4e-6) < 1e-12, sender
    assert abs(receiver["min"] - 5.7e-6) < 1e-12 and abs(receiver["max"] - 5.7e-6) < 1e-12, receiver
    assert abs(results[1].comm_stats["mean_generation_time"] - 1e-6) < 1e-12
    return f"sender {sender['max'] * 1e6:.2f} us, receiver {receiver['max'] * 1e6:.2f} us"


def check_prefetch(qpus, shots):
    # 6 us of local work and pairs generated in 5 us: requesting the pair first overlaps both.
    model = {"epr": {"attempt_time": 5e-6}}
    on_demand = run_teledata(qpus, min(shots, 200), local_work=True, comm_model=model)
    prefetched = run_teledata(qpus, min(shots, 200), local_work=True, prefetch=True, comm_model=model)
    saved = on_demand[1].simulated_time["mean"] - prefetched[1].simulated_time["mean"]
    assert abs(saved - 5e-6) < 1e-12, saved
    return f"prefetching saves {saved * 1e6:.2f} us"


def check_invalid_model_is_reported(qpus, shots):
    qjobs = run(teledata(), qpus, shots=10, comm_model={"epr": {"success_probability": 2.0}})
    # Every job of the round gets the error. Each result is collected on its own: gather()
    # would stop at the first error and leave the other one waiting in its vQPU buffer.
    messages = []
    for qjob in qjobs:
        try:
            qjob.result
        except RuntimeError as error:
            messages.append(str(error))
    assert len(messages) == len(qjobs), messages
    message = messages[0]
    # The executor keeps serving runs.
    measured = fidelity(run_teledata(qpus, shots, comm_model={"links": [
        {"circuits": ["Sender", "Receiver"], "epr": {"fidelity": 1.0}}]})[1])
    assert measured == 1.0, measured
    return "reported: " + message.split(":")[-1].strip()


CHECKS = [
    check_deployed_model,
    check_run_override,
    check_link_by_circuit_ids,
    check_fidelity_decreases_with_generation_time,
    check_simulated_time,
    check_prefetch,
    check_invalid_model_is_reported,
]


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--shots", type=int, default=4000)
    parser.add_argument("--simulator", default="Aer")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(dir=os.getcwd()) as directory:
        backend_path = os.path.join(directory, "comm_noise_backend.json")
        with open(backend_path, "w") as backend_file:
            json.dump(BACKEND, backend_file)

        family = qraise(2, "00:30:00", simulator=args.simulator, quantum_comm=True,
                        backend=backend_path, co_located=True)
        failures = 0
        try:
            qpus = get_QPUs(co_located=True, family=family)
            for check in CHECKS:
                try:
                    detail = check(qpus, args.shots)
                    print(f"PASS  {check.__name__}: {detail}")
                except Exception:
                    failures += 1
                    print(f"FAIL  {check.__name__}")
                    traceback.print_exc()
        finally:
            qdrop(family)

    print(f"\n{len(CHECKS) - failures}/{len(CHECKS)} checks passed")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
