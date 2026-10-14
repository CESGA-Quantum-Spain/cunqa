#!/usr/bin/env python3
"""
Simulated-time checker: deploys vQPUs with a "timing" block in the three communication schemes
and compares the simulated time of their shots with the expected timeline.

This is a SYSTEM test, not a unit test: it allocates real vQPUs through SLURM (``qraise``) and
goes through the whole stack (``run`` -> vQPUs -> executors). The clock itself is unit-tested in
``tests/cpp/test_timing.cpp``. Every check runs a circuit with a classically controlled block whose
condition holds in some shots and not in others: the shots that skip the block must still wait
for its condition, so no shot finishes before its bits are known.

Usage
-----
    python check_simulated_time.py
    python check_simulated_time.py --simulator Munich
"""

import argparse
import json
import os
import sys
import tempfile
import traceback

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME"))  # HOME as install path is specific to CESGA

from cunqa.qpu import qraise, get_QPUs, run, qdrop
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit

US = 1e-6
EPS = 1e-12

# 1-qubit gates 0.05 us, 2-qubit gates 0.3 us, measurements and resets 1 us.
TIMING = {
    "one_qubit_gate": 5e-8,
    "multi_qubit_gate": 3e-7,
    "measure": 1e-6,
    "reset": 1e-6,
}


def backend(name, num_qubits, **timing):
    return {
        "name": name,
        "version": "0.0.1",
        "description": "Backend with simulated time (durations in s).",
        "num_qubits": num_qubits,
        "timing": {**TIMING, **timing},
    }


def deploy(directory, simulator, n, backend_json, **qraise_args):
    path = os.path.join(directory, backend_json["name"] + ".json")
    with open(path, "w") as backend_file:
        json.dump(backend_json, backend_file)
    return qraise(n, "00:10:00", simulator=simulator, co_located=True, backend=path, **qraise_args)


def near(value, expected):
    return abs(value - expected) < EPS


def both_branches(counts, *outcomes):
    """Every outcome appears, so the block was applied in some shots and skipped in others."""
    assert all(counts.get(outcome, 0) > 0 for outcome in outcomes), counts


def check_no_comm(directory, simulator, shots):
    # q0: H [0, 0.05), M [0.05, 1.05); the decision comes 0.1 us later, at 1.15.
    # q1: X [1.15, 1.2) and M [1.2, 2.2) when c0 = 1; when the X is skipped, q1 still waits for
    #     the decision and M takes [1.15, 2.15).
    family = deploy(directory, simulator, 1, backend("NoCommTimed", [2, 0], feed_forward_latency=1e-7))
    try:
        [qpu] = get_QPUs(co_located=True, family=family)
        qc = CunqaCircuit(2, 2)
        qc.h(0)
        qc.measure(0, 0)
        qc.cif(0)
        qc.x(1)
        qc.endcif()
        qc.measure(1, 1)
        result = run(qc, qpu, shots=shots).result
    finally:
        qdrop(family)

    both_branches(result.counts, "00", "11")
    timing = result.simulated_time
    assert near(timing["min"], 2.15 * US) and near(timing["max"], 2.2 * US), timing
    assert near(timing["idle_mean"][1], 1.15 * US), timing
    return f"shots last {timing['min'] / US:.2f}-{timing['max'] / US:.2f} us"


def check_classical_comm(directory, simulator, shots):
    # Sender: H [0, 0.05), M [0.05, 1.05), sent at 1.05.
    # Receiver: the bit arrives after 0.5 us of latency and a jitter J uniform in [0, 0.2) us,
    #           and is acted on 0.1 us later, at 1.65 + J. The X takes 0.05 us when c0 = 1, and
    #           the measurement 1 us, so its shots last between 2.65 and 2.9 us.
    timed = backend("ClassicalCommTimed", [1, 0], classical_latency=5e-7, feed_forward_latency=1e-7,
                    classical_jitter={"distribution": "uniform", "width": 2e-7})
    family = deploy(directory, simulator, 2, timed, classical_comm=True)
    try:
        qpus = get_QPUs(co_located=True, family=family)
        sender = CunqaCircuit(1, 1, id="sender")
        receiver = CunqaCircuit(1, 1, id="receiver")
        sender.h(0)
        sender.measure(0, 0)
        sender.send(0, recving_circuit="receiver")
        receiver.recv(0, sending_circuit="sender")
        receiver.cif(0)
        receiver.x(0)
        receiver.endcif()
        receiver.measure(0, 0)
        results = gather(run([sender, receiver], qpus, shots=shots))
    finally:
        qdrop(family)

    both_branches(results[1].counts, "0", "1")
    assert near(results[0].simulated_time["max"], 1.05 * US), results[0].simulated_time
    timing = results[1].simulated_time
    assert timing["min"] >= 2.65 * US - EPS and timing["max"] <= 2.9 * US + EPS, timing
    # The jitter is drawn in every shot: the spread exceeds the 0.05 us of the X.
    assert timing["max"] - timing["min"] > 0.1 * US, timing
    return f"receiver shots last {timing['min'] / US:.3f}-{timing['max'] / US:.3f} us"


def check_quantum_comm(directory, simulator, shots):
    # examples/qu_comm/08-simulated_time: Bob's comm qubit is reset when Alice asks for the pair,
    # so the 2 us generation starts at 1 us. Alice measures her half in [3, 4) and sends the bit,
    # which reaches Bob at 4.5 us; his data qubit waits for it in every shot, and the X [4.5, 4.55)
    # is only applied when it is 1, so his measurement ends at 5.55 or 5.5 us.
    timed = backend("QuantumCommTimed", [1, 1], classical_latency=5e-7, entanglement=2e-6)
    family = deploy(directory, simulator, 2, timed, quantum_comm=True)
    try:
        qpus = get_QPUs(co_located=True, family=family)
        alice = CunqaCircuit((1, 1), 2, id="Alice")
        a_data, a_comm = alice.get_qubits()
        bob = CunqaCircuit((1, 1), 3, id="Bob")
        b_data, b_comm = bob.get_qubits()

        alice.h(a_data[0])
        alice.gen_ent(a_comm[0], "Bob", tag="epr")
        alice.measure(a_comm[0], 0)
        alice.send(0, recving_circuit="Bob")
        alice.measure(a_data[0], 1)

        bob.reset(b_comm[0])
        bob.gen_ent(b_comm[0], "Alice", tag="epr")
        bob.measure(b_comm[0], 0)
        bob.recv(1, sending_circuit="Alice")
        bob.cif(1)
        bob.x(b_data[0])
        bob.endcif()
        bob.measure(b_data[0], 2)
        results = gather(run([alice, bob], qpus, shots=shots))
    finally:
        qdrop(family)

    both_branches(results[1].counts, "00", "11")
    assert set(results[1].counts) == {"00", "11"}, results[1].counts
    a, b = results[0].simulated_time, results[1].simulated_time
    assert near(a["min"], 4 * US) and near(a["max"], 4 * US), a
    assert near(a["idle_mean"][1], 1 * US), a
    assert near(b["min"], 5.5 * US) and near(b["max"], 5.55 * US), b
    assert near(b["idle_mean"][0], 4.5 * US) and near(b["idle_mean"][1], 0.0), b
    return f"Alice {a['max'] / US:.2f} us, Bob {b['min'] / US:.2f}-{b['max'] / US:.2f} us"


CHECKS = [
    check_no_comm,
    check_classical_comm,
    check_quantum_comm,
]


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--shots", type=int, default=1000)
    parser.add_argument("--simulator", default="Aer")
    args = parser.parse_args()

    failures = 0
    with tempfile.TemporaryDirectory(dir=os.getcwd()) as directory:
        for check in CHECKS:
            try:
                detail = check(directory, args.simulator, args.shots)
                print(f"PASS  {check.__name__}: {detail}")
            except Exception:
                failures += 1
                print(f"FAIL  {check.__name__}")
                traceback.print_exc()

    print(f"\n{len(CHECKS) - failures}/{len(CHECKS)} checks passed")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
