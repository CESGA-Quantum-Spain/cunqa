#!/usr/bin/env python3
"""
Gate support checker: runs every gate each simulator declares in its basis gates
against a real vQPU and compares the counts with an analytically derived result.

This is a SYSTEM test, not a unit test: it allocates real vQPUs through SLURM
(``qraise``) and executes circuits on them. It deliberately lives outside
``tests/`` -- that directory is for mocked pytest unit tests, and pytest would
otherwise collect this file.

Both execution paths are covered
--------------------------------
``NCExecutor::execute`` picks the path from ``config.is_dynamic``:

    is_dynamic == false  ->  simulator_->native_execute(circuit)   # per-simulator JSON
    is_dynamic == true   ->  custom_execute_()                     # apply_gate overloads

These are two independent implementations of the same gate set, and a gate listed
in a simulator's basis gates must work in both. A plain circuit has
``is_dynamic == False`` (only cif/recv/... set it), so a test that does not force
the flag silently exercises only ``native_execute``. Every job below therefore runs
once per path, passing ``is_dynamic`` as a run argument (``QJob`` merges run
arguments over its ``run_config``). Use ``--mode`` to restrict this.

How to read a result
--------------------
Every circuit is built so the ideal outcome is a single deterministic bitstring
(probability 1.0); the handful of genuinely random ones state their split
explicitly. A broken gate therefore either raises ("Gate X not supported by ...",
"Instruction not suported!") or spreads its counts, and both are unmistakable.

Bitstrings use the cunqa/Qiskit convention: qubit 0 is the RIGHTMOST character, so
'101' means q2=1, q1=0, q0=1. Simulators pad the string to different widths, so
only the low ``num_qubits`` characters are compared.

Angles are written as floats on purpose. An integer in ``params`` is stored by
nlohmann as an integer, and ``AERCircuit``'s ``get_ptr<double*>()`` then returns
nullptr ("Expected a floating-point JSON parameter").

A few gates are marked ``SMOKE``: their semantics are defined by a closed or
undocumented library (Maestro's ``k``, ``randomunitary``), so the check is only
that they execute and return counts. Everything else is checked numerically.

Usage
-----
    python check_gate_support.py                    # Aer, both paths
    python check_gate_support.py --simulator Quest
    python check_gate_support.py --simulator all    # every simulator, in turn
    python check_gate_support.py --mode native      # only native_execute
    python check_gate_support.py --gate cswap       # a single gate, any simulator
"""

import argparse
import math
import os
import sys
import traceback

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME"))  # HOME as install path is specific to CESGA

from cunqa.qpu import qraise, get_QPUs, run, qdrop
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit

PI = math.pi
SHOTS = 4096
TOLERANCE = 0.04  # absolute tolerance on each probability

ALL_SIMULATORS = ["Aer", "Munich", "Qulacs", "Maestro", "Qsim", "Quest", "Cunqa"]

SMOKE = None  # expected == SMOKE -> only assert the circuit ran

X_MAT = [[0 + 0j, 1 + 0j], [1 + 0j, 0 + 0j]]
Z_DIAG = [1 + 0j, -1 + 0j]


# ---------------------------------------------------------------------------
# What each simulator claims to support, mirrored from the *_BASIS_GATES arrays
# in src/sim/simulators/<Sim>/<sim>_simulator_adapter.hpp. Keep in sync: a gate
# listed here but not implemented is exactly the bug this script hunts for.
# ---------------------------------------------------------------------------
SUPPORTED = {
    "Aer": {
        "ccx", "ccz", "cp", "crx", "cry", "crz", "cswap", "csx", "cu", "cu1", "cu2",
        "cu3", "cx", "cy", "cz", "diagonal", "ecr", "gp", "h", "id", "mcp", "mcrx",
        "mcry", "mcrz", "mcswap", "mcsx", "mcu1", "mcu2", "mcu3", "mcx", "mcy",
        "mcz", "measure", "r", "rx", "rxx", "ry", "ryy", "rz", "rzx", "rzz", "s",
        "sdg", "swap", "sx", "sxdg", "t", "tdg", "u1", "u2", "u3", "unitary", "x",
        "y", "z"
    },
    "Munich": {
        "ch", "cp", "crx", "cry", "crz", "cs", "csdg", "cswap", "csx", "cu", "cu1",
        "cu3", "cx", "cy", "cz", "dcx", "ecr", "gp", "h", "id", "iswap", "mcp",
        "mcx", "measure", "p", "reset", "rx", "rxx", "ry", "ryy", "rz", "rzx",
        "rzz", "s", "sdg", "swap", "sx", "sxdg", "t", "tdg", "u", "u1", "u2", "u3",
        "x", "xxmyy", "xxpyy", "y", "z"
    },
    "Qulacs": {
        "amplitudedampingnoise", "bitflipnoise", "cp", "cx", "cz", "dephasingnoise",
        "depolarizingnoise", "diagonal", "ecr", "fusedswap", "h", "id",
        "independentxznoise", "measure", "multipauli", "multipaulirotation", "p0",
        "p1", "randomunitary", "rotinvx", "rotinvy", "rotinvz", "rotx", "roty",
        "rotz", "rx", "ry", "rz", "s", "sdg", "sparsematrix", "swap", "sx", "sxdg",
        "sy", "sydg", "t", "tdg", "twoqubitdepolarizingnoise", "u1", "u2", "u3",
        "unitary", "x", "y", "z"
    },
    # "reset" is intentionally absent: maestrolib's JSON circuit format has no reset
    # operation, so it only works on the dynamic path. See README "Known deviations".
    "Maestro": {
        "ccx", "ch", "cp", "crx", "cry", "crz", "cswap", "csx", "csxdg", "cu", "cx",
        "cy", "cz", "h", "k", "measure", "p", "rx", "ry", "rz", "s", "sdg",
        "swap", "sx", "sxdg", "t", "tdg", "u", "x", "y", "z"
    },
    "Qsim": {
        "cp", "cunitary", "cx", "cz", "fs", "gp", "h", "hz2", "id", "id2", "iswap",
        "measure", "rx", "rxy", "ry", "rz", "s", "swap", "sx", "sy", "t", "unitary",
        "x", "y", "z"
    },
    "Quest": {
        "ch", "cmx", "cp", "cpauligadget", "cpaulistr", "cphasegadget", "craxis",
        "crx", "cry", "crz", "cs", "csqrtswap", "cswap", "ct", "cx", "cy", "cz",
        "h", "mch", "mcmx", "mcp", "mcpauligadget", "mcpaulistr", "mcphasegadget",
        "mcraxis", "mcrx", "mcry", "mcrz", "mcs", "mcsqrtswap", "mcswap", "mct",
        "mcx", "mcy", "mcz", "measure", "mx", "nonunitarypauligadget", "p",
        "pauligadget", "paulistr", "phasegadget", "raxis", "rx", "ry", "rz", "s",
        "sqrtswap", "swap", "t", "x", "y", "z"
    },
    "Cunqa": {
        "crx", "cry", "crz", "cx", "cy", "cz", "h", "id", "measure", "rx", "ry",
        "rz", "swap", "sx", "x", "y", "z"
    },
}


# ---------------------------------------------------------------------------
# Test table: (gate, num_qubits, build, expected, rationale)
#   build    : callable applying the gate sequence to a CunqaCircuit
#   expected : {bitstring: probability}, or SMOKE for "must merely execute"
#
# Which simulators run a given entry is derived from SUPPORTED, so adding a gate
# to a basis list automatically starts testing it everywhere it is claimed.
# ---------------------------------------------------------------------------
TESTS = [
    # ------------------------- one qubit, no params ------------------------
    ("x",    1, lambda qc: qc.x(0), {"1": 1.0}, "X flips |0>"),
    ("y",    1, lambda qc: qc.y(0), {"1": 1.0}, "Y|0> = i|1>"),
    ("z",    1, lambda qc: (qc.h(0), qc.z(0), qc.h(0)), {"1": 1.0}, "H-Z-H = X"),
    ("h",    1, lambda qc: qc.h(0), {"0": 0.5, "1": 0.5}, "H|0> is an equal superposition"),
    ("id",   1, lambda qc: (qc.x(0), qc.i(0)), {"1": 1.0}, "identity must not disturb the X"),
    ("s",    1, lambda qc: (qc.h(0), qc.s(0), qc.s(0), qc.h(0)), {"1": 1.0}, "S^2 = Z"),
    ("sdg",  1, lambda qc: (qc.h(0), qc.sdg(0), qc.sdg(0), qc.h(0)), {"1": 1.0}, "SDG^2 = Z"),
    ("t",    1, lambda qc: (qc.h(0), qc.t(0), qc.t(0), qc.t(0), qc.t(0), qc.h(0)),
     {"1": 1.0}, "T^4 = Z"),
    ("tdg",  1, lambda qc: (qc.h(0), qc.tdg(0), qc.tdg(0), qc.tdg(0), qc.tdg(0), qc.h(0)),
     {"1": 1.0}, "TDG^4 = Z"),
    ("sx",   1, lambda qc: (qc.sx(0), qc.sx(0)), {"1": 1.0}, "SX^2 = X"),
    ("sxdg", 1, lambda qc: (qc.sxdg(0), qc.sxdg(0)), {"1": 1.0}, "SXDG^2 = X"),
    ("sy",   1, lambda qc: (qc.sy(0), qc.sy(0)), {"1": 1.0}, "SY^2 = Y"),
    ("sydg", 1, lambda qc: (qc.sydg(0), qc.sydg(0)), {"1": 1.0}, "SYDG^2 = Y"),
    ("p0",   1, lambda qc: qc.p0(0), {"0": 1.0}, "P0 projector leaves |0> untouched"),
    ("p1",   1, lambda qc: (qc.x(0), qc.p1(0)), {"1": 1.0}, "P1 projector leaves |1> untouched"),
    ("hz2",  1, lambda qc: qc.hz2(0), {"0": 0.5, "1": 0.5},
     "qsim HZ2 is a pi/2 rotation about (X+Y)"),
    ("k",    1, lambda qc: qc.k(0), SMOKE, "Maestro-specific gate, semantics not public"),

    # ------------------------ one qubit, parameterised ---------------------
    ("rx",      1, lambda qc: qc.rx(PI, 0), {"1": 1.0}, "RX(pi) = -i*X"),
    ("ry",      1, lambda qc: qc.ry(PI, 0), {"1": 1.0}, "RY(pi) = -i*Y"),
    ("rz",      1, lambda qc: (qc.h(0), qc.rz(PI, 0), qc.h(0)), {"1": 1.0}, "RZ(pi) ~ Z"),
    ("p",       1, lambda qc: (qc.h(0), qc.p(PI, 0), qc.h(0)), {"1": 1.0}, "P(pi) = Z"),
    ("u1",      1, lambda qc: (qc.h(0), qc.u1(PI, 0), qc.h(0)), {"1": 1.0}, "U1(pi) = Z"),
    ("u2",      1, lambda qc: (qc.u2(0.0, PI, 0), qc.z(0), qc.u2(0.0, PI, 0)),
     {"1": 1.0}, "U2(0,pi) = H, so H-Z-H = X"),
    ("u3",      1, lambda qc: qc.u3(PI, 0.0, PI, 0), {"1": 1.0}, "U3(pi,0,pi) = X"),
    ("u",       1, lambda qc: qc.u(PI, 0.0, PI, 0), {"1": 1.0},
     "U(pi,0,pi,gamma=0) = X; exercises the 4th (global phase) parameter"),
    ("r",       1, lambda qc: qc.r(PI, 0.0, 0), {"1": 1.0}, "R(pi,0) = -i*X"),
    ("rxy",     1, lambda qc: qc.rxy(0.0, PI, 0), {"1": 1.0},
     "qsim RXY(theta=0, phi=pi) = -i*X; it is a ONE-qubit gate"),
    ("raxis",   1, lambda qc: qc.raxis(PI, [1.0, 0.0, 0.0], 0), {"1": 1.0},
     "pi rotation about the X axis"),
    ("rotx",    1, lambda qc: qc.rotx(PI, 0), {"1": 1.0}, "RotX(pi) = +-i*X"),
    ("roty",    1, lambda qc: qc.roty(PI, 0), {"1": 1.0}, "RotY(pi) = +-i*Y"),
    ("rotz",    1, lambda qc: (qc.h(0), qc.rotz(PI, 0), qc.h(0)), {"1": 1.0}, "RotZ(pi) ~ Z"),
    ("rotinvx", 1, lambda qc: qc.rotinvx(PI, 0), {"1": 1.0}, "inverse pi rotation still flips"),
    ("rotinvy", 1, lambda qc: qc.rotinvy(PI, 0), {"1": 1.0}, "inverse pi rotation still flips"),
    ("rotinvz", 1, lambda qc: (qc.h(0), qc.rotinvz(PI, 0), qc.h(0)), {"1": 1.0}, "~ Z"),
    ("gp",      1, lambda qc: (qc.h(0), qc.globalp(PI)), {"0": 0.5, "1": 0.5},
     "a global phase is unobservable; this asserts gp is accepted at all"),

    # -------------------------- two qubits, no params ----------------------
    ("cx",       2, lambda qc: (qc.x(0), qc.cx(0, 1)), {"11": 1.0}, "control set -> target flips"),
    ("cy",       2, lambda qc: (qc.x(0), qc.cy(0, 1)), {"11": 1.0}, "control set -> target flips"),
    ("cz",       2, lambda qc: (qc.x(0), qc.h(1), qc.cz(0, 1), qc.h(1)),
     {"11": 1.0}, "CZ conjugated by H on the target is CX"),
    ("ch",       2, lambda qc: (qc.x(0), qc.ch(0, 1), qc.cz(0, 1), qc.ch(0, 1)),
     {"11": 1.0}, "controlled H-Z-H = CX"),
    ("csx",      2, lambda qc: (qc.x(0), qc.csx(0, 1), qc.csx(0, 1)), {"11": 1.0}, "CSX^2 = CX"),
    ("csxdg",    2, lambda qc: (qc.x(0), qc.csxdg(0, 1), qc.csxdg(0, 1)),
     {"11": 1.0}, "CSXDG^2 = CX"),
    ("cs",       2, lambda qc: (qc.x(0), qc.h(1), qc.cs(0, 1), qc.cs(0, 1), qc.h(1)),
     {"11": 1.0}, "CS^2 = CZ"),
    ("csdg",     2, lambda qc: (qc.x(0), qc.h(1), qc.csdg(0, 1), qc.csdg(0, 1), qc.h(1)),
     {"11": 1.0}, "CSDG^2 = CZ"),
    ("ct",       2, lambda qc: (qc.x(0), qc.h(1), qc.ct(0, 1), qc.ct(0, 1), qc.ct(0, 1),
                                qc.ct(0, 1), qc.h(1)),
     {"11": 1.0}, "CT^4 = CZ"),
    ("swap",     2, lambda qc: (qc.x(0), qc.swap(0, 1)), {"10": 1.0}, "excitation moves q0 -> q1"),
    ("iswap",    2, lambda qc: (qc.x(0), qc.iswap(0, 1)), {"10": 1.0},
     "iSWAP moves the excitation (with a phase)"),
    ("sqrtswap", 2, lambda qc: (qc.x(0), qc.sqrtswap(0, 1), qc.sqrtswap(0, 1)),
     {"10": 1.0}, "SQRTSWAP^2 = SWAP"),
    ("dcx",      2, lambda qc: (qc.x(0), qc.dcx(0, 1)), {"10": 1.0}, "double-CX moves |01> -> |10>"),
    ("ecr",      2, lambda qc: qc.ecr(0, 1), {"01": 0.5, "11": 0.5},
     "ECR|00> = (|01> - i|11>)/sqrt(2); q0 always ends at 1"),
    ("id2",      2, lambda qc: (qc.x(0), qc.id2(0, 1)), {"01": 1.0}, "two-qubit identity"),

    # ------------------------- two qubits, parameterised -------------------
    ("cp",     2, lambda qc: (qc.x(0), qc.h(1), qc.cp(PI, 0, 1), qc.h(1)),
     {"11": 1.0},
     "CP(pi) = CZ. NOTE pi is the one angle where the sign convention cannot show: "
     "cunqa follows AER's exp(+i*phi) but qsim's GateCP is exp(-i*phi), and the two "
     "agree only at 0 and pi. See 'Known deviations' in the README"),
    ("cu1",    2, lambda qc: (qc.x(0), qc.h(1), qc.cu1(PI, 0, 1), qc.h(1)),
     {"11": 1.0}, "CU1(pi) = CZ"),
    ("crx",    2, lambda qc: (qc.x(0), qc.crx(PI, 0, 1)), {"11": 1.0}, "CRX(pi) flips the target"),
    ("cry",    2, lambda qc: (qc.x(0), qc.cry(PI, 0, 1)), {"11": 1.0}, "CRY(pi) flips the target"),
    ("crz",    2, lambda qc: (qc.x(0), qc.h(1), qc.crz(PI, 0, 1), qc.h(1)),
     {"11": 1.0}, "CRZ(pi) ~ CZ"),
    ("craxis", 2, lambda qc: (qc.x(0), qc.add_instructions({
        "name": "craxis", "qubits": [0, 1], "params": [PI], "axis": [1.0, 0.0, 0.0]})),
     {"11": 1.0}, "controlled pi rotation about X (no craxis() helper exists)"),
    ("rxx",    2, lambda qc: qc.rxx(PI, 0, 1), {"11": 1.0}, "RXX(pi) = -i X kron X"),
    ("ryy",    2, lambda qc: qc.ryy(PI, 0, 1), {"11": 1.0}, "RYY(pi) = -i Y kron Y"),
    ("rzz",    2, lambda qc: (qc.h(0), qc.h(1), qc.rzz(PI, 0, 1), qc.h(0), qc.h(1)),
     {"11": 1.0}, "RZZ(pi) ~ Z kron Z, conjugated by H kron H"),
    ("rzx",    2, lambda qc: qc.rzx(PI, 0, 1), {"10": 1.0}, "X acts on q1, Z on q0"),
    ("xxpyy",  2, lambda qc: (qc.x(0), qc.xxpyy(PI, 0.0, 0, 1)), {"10": 1.0},
     "XX+YY at pi swaps the single excitation"),
    ("xxmyy",  2, lambda qc: qc.xxmyy(PI, 0.0, 0, 1), {"11": 1.0},
     "XX-YY at pi drives |00> to |11>"),
    ("fs",     2, lambda qc: (qc.x(0), qc.fs(PI / 2, 0.0, 0, 1)), {"10": 1.0},
     "fSim(pi/2, 0) swaps the excitation"),
    ("cu2",    2, lambda qc: (qc.x(0), qc.cu2(0.0, PI, 0, 1), qc.z(1), qc.cu2(0.0, PI, 0, 1)),
     {"11": 1.0}, "CU2(0,pi) = controlled H"),
    ("cu3",    2, lambda qc: (qc.x(0), qc.cu3(PI, 0.0, PI, 0, 1)), {"11": 1.0}, "CU3(pi,0,pi) = CX"),
    ("cu",     2, lambda qc: (qc.x(0), qc.cu(PI, 0.0, PI, 0.0, 0, 1)), {"11": 1.0},
     "CU(pi,0,pi,0) = CX"),

    # ------------------------------ three qubits ---------------------------
    ("ccx",       3, lambda qc: (qc.x(0), qc.x(1), qc.ccx(0, 1, 2)),
     {"111": 1.0}, "Toffoli with both controls set"),
    ("ccz",       3, lambda qc: (qc.x(0), qc.x(1), qc.h(2), qc.ccz(0, 1, 2), qc.h(2)),
     {"111": 1.0}, "CCZ conjugated by H on the target"),
    ("cswap",     3, lambda qc: (qc.x(0), qc.x(1), qc.cswap(0, 1, 2)), {"101": 1.0},
     "Fredkin: control q0 swaps q1 and q2. '110' means the third qubit was dropped"),
    ("csqrtswap", 3, lambda qc: (qc.x(0), qc.x(1), qc.csqrtswap(0, 1, 2),
                                 qc.csqrtswap(0, 1, 2)),
     {"101": 1.0}, "CSQRTSWAP^2 = CSWAP"),

    # --------------------------- multi-controlled --------------------------
    ("mcx",  3, lambda qc: (qc.x(0), qc.x(1), qc.mcx(0, 1, 2)), {"111": 1.0}, "two controls set"),
    ("mcy",  3, lambda qc: (qc.x(0), qc.x(1), qc.mcy(0, 1, 2)), {"111": 1.0}, "two controls set"),
    ("mcz",  3, lambda qc: (qc.x(0), qc.x(1), qc.h(2), qc.mcz(0, 1, 2), qc.h(2)),
     {"111": 1.0}, "MCZ conjugated by H"),
    ("mch",  3, lambda qc: (qc.x(0), qc.x(1), qc.mch(0, 1, 2), qc.mcz(0, 1, 2), qc.mch(0, 1, 2)),
     {"111": 1.0}, "multi-controlled H-Z-H"),
    ("mcs",  3, lambda qc: (qc.x(0), qc.x(1), qc.h(2), qc.mcs(0, 1, 2), qc.mcs(0, 1, 2), qc.h(2)),
     {"111": 1.0}, "MCS^2 = MCZ"),
    ("mct",  3, lambda qc: (qc.x(0), qc.x(1), qc.h(2), qc.mct(0, 1, 2), qc.mct(0, 1, 2),
                            qc.mct(0, 1, 2), qc.mct(0, 1, 2), qc.h(2)),
     {"111": 1.0}, "MCT^4 = MCZ"),
    ("mcsx", 3, lambda qc: (qc.x(0), qc.x(1), qc.mcsx(0, 1, 2), qc.mcsx(0, 1, 2)),
     {"111": 1.0}, "MCSX^2 = MCX"),
    ("mcp",  3, lambda qc: (qc.x(0), qc.x(1), qc.h(2), qc.mcp(PI, 0, 1, 2), qc.h(2)),
     {"111": 1.0}, "MCP(pi) = MCZ"),
    ("mcu1", 3, lambda qc: (qc.x(0), qc.x(1), qc.h(2), qc.mcu1(PI, 0, 1, 2), qc.h(2)),
     {"111": 1.0}, "MCU1(pi) = MCZ"),
    ("mcu2", 3, lambda qc: (qc.x(0), qc.x(1), qc.mcu2(0.0, PI, 0, 1, 2), qc.z(2),
                            qc.mcu2(0.0, PI, 0, 1, 2)),
     {"111": 1.0}, "MCU2(0,pi) = multi-controlled H"),
    ("mcu3", 3, lambda qc: (qc.x(0), qc.x(1), qc.mcu3(PI, 0.0, PI, 0, 1, 2)),
     {"111": 1.0}, "MCU3(pi,0,pi) = MCX"),
    ("mcrx", 3, lambda qc: (qc.x(0), qc.x(1), qc.mcrx(PI, 0, 1, 2)), {"111": 1.0}, "MCRX(pi)"),
    ("mcry", 3, lambda qc: (qc.x(0), qc.x(1), qc.mcry(PI, 0, 1, 2)), {"111": 1.0}, "MCRY(pi)"),
    ("mcrz", 3, lambda qc: (qc.x(0), qc.x(1), qc.h(2), qc.mcrz(PI, 0, 1, 2), qc.h(2)),
     {"111": 1.0}, "MCRZ(pi) ~ MCZ"),
    ("mcraxis", 3, lambda qc: (qc.x(0), qc.x(1), qc.mcraxis(PI, [1.0, 0.0, 0.0], 0, 1, 2)),
     {"111": 1.0}, "exercises the angle+axis parameter packing"),
    ("mcswap", 3, lambda qc: (qc.x(0), qc.x(1), qc.mcswap(0, 1, 2)), {"101": 1.0},
     "one control, last TWO qubits are the swap targets"),
    ("mcsqrtswap", 3, lambda qc: (qc.x(0), qc.x(1), qc.mcsqrtswap(0, 1, 2),
                                  qc.mcsqrtswap(0, 1, 2)),
     {"101": 1.0}, "MCSQRTSWAP^2 = MCSWAP"),

    # ------------------------ Pauli strings and gadgets --------------------
    ("paulistr",   1, lambda qc: qc.paulistr("X", 0), {"1": 1.0}, "the Pauli string 'X' is X"),
    ("cpaulistr",  2, lambda qc: (qc.x(0), qc.cpaulistr("XI", 0)), {"11": 1.0},
     "qubits[0] is the control; the string carries the target, rightmost char = q0, "
     "so 'XI' is X on q1"),
    ("mcpaulistr", 3, lambda qc: (qc.x(0), qc.x(1), qc.mcpaulistr("XII", 0, 1)),
     {"111": 1.0}, "every listed qubit is a control; 'XII' puts the X on q2"),
    ("pauligadget", 1, lambda qc: qc.pauligadget(PI, "X", 0), {"1": 1.0},
     "exp(-i pi/2 X) = -i*X"),
    ("nonunitarypauligadget", 1, lambda qc: qc.nonunitarypauligadget(PI, "X", 0), SMOKE,
     "non-unitary variant: the state is not normalised, so only execution is checked"),
    ("cpauligadget", 2, lambda qc: (qc.x(0), qc.cpauligadget(PI, "XI", 0)),
     {"11": 1.0}, "qubits[0] is the control; 'XI' puts the gadget on q1"),
    ("mcpauligadget", 3, lambda qc: (qc.x(0), qc.x(1), qc.mcpauligadget(PI, "XII", 0, 1)),
     {"111": 1.0}, "every listed qubit is a control; 'XII' puts the gadget on q2"),
    ("phasegadget", 1, lambda qc: (qc.h(0), qc.phasegadget(PI, 0), qc.h(0)),
     {"1": 1.0}, "exp(-i pi/2 Z) ~ Z"),
    ("cphasegadget", 2, lambda qc: (qc.x(0), qc.h(1), qc.cphasegadget(PI, 0, 1), qc.h(1)),
     {"11": 1.0}, "controlled phase gadget ~ CZ"),
    ("mcphasegadget", 3, lambda qc: (qc.x(0), qc.x(1), qc.h(2),
                                     qc.mcphasegadget(PI, 2, 0, 1, 2), qc.h(2)),
     {"111": 1.0}, "num_controls=2 -> q0,q1 are controls and q2 is the target"),

    # --------------------------- multi-qubit NOT ---------------------------
    ("mx",   2, lambda qc: qc.mx(0, 1), {"11": 1.0}, "X on every listed qubit"),
    ("cmx",  3, lambda qc: (qc.x(0), qc.cmx(0, 1, 2)), {"111": 1.0},
     "control q0, X on the remaining qubits"),
    ("mcmx", 3, lambda qc: (qc.x(0), qc.x(1), qc.mcmx(2, 0, 1, 2)), {"111": 1.0},
     "num_controls=2 -> q0,q1 are controls and X lands on q2"),

    # ------------------------------- matrices ------------------------------
    ("unitary",   1, lambda qc: qc.unitary(X_MAT, 0), {"1": 1.0},
     "X supplied as a matrix. Note a 2x2 unitary always has |m01| = |m10|, so this "
     "cannot detect a transposed layout -- only that the gate runs and acts"),
    ("cunitary",  2, lambda qc: (qc.x(0), qc.cunitary(X_MAT, 0, 1)), {"11": 1.0},
     "controlled X from a 2x2 matrix; a '01' result means the control order is inverted"),
    ("sparsematrix", 1, lambda qc: qc.sparsematrix(X_MAT, 0), {"1": 1.0}, "X as a sparse matrix"),
    ("diagonal",  1, lambda qc: (qc.h(0), qc.diagonal(Z_DIAG, 0), qc.h(0)),
     {"1": 1.0}, "diag(1,-1) = Z, so H-Z-H = X"),
    ("fusedswap", 2, lambda qc: (qc.x(0), qc.fusedswap(1, 0, 1)), {"10": 1.0},
     "block size 1 makes fusedswap a plain SWAP"),
    ("randomunitary", 1, lambda qc: qc.randomunitary(0, seed=1234), SMOKE,
     "random by construction: only execution is checked"),
    ("multipauli", 1, lambda qc: qc.multipauli([1], 0), {"1": 1.0}, "pauli id 1 is X"),
    ("multipaulirotation", 1, lambda qc: qc.multipaulirotation(PI, [1], 0), {"1": 1.0},
     "exp(-i pi/2 X) = -i*X"),

    # -------------------------------- noise --------------------------------
    ("bitflipnoise", 1, lambda qc: qc.bitflipnoise(1.0, 0), {"1": 1.0},
     "probability 1 makes the bit flip deterministic"),
    ("dephasingnoise", 1, lambda qc: qc.dephasingnoise(0.0, 0), {"0": 1.0},
     "probability 0 is the identity channel"),
    ("depolarizingnoise", 1, lambda qc: qc.depolarizingnoise(0.0, 0), {"0": 1.0},
     "probability 0 is the identity channel"),
    ("independentxznoise", 1, lambda qc: qc.independentxznoise(0.0, 0), {"0": 1.0},
     "probability 0 is the identity channel"),
    ("amplitudedampingnoise", 1, lambda qc: (qc.x(0), qc.amplitudedampingnoise(1.0, 0)),
     {"0": 1.0}, "full damping drives |1> back to |0>"),
    ("twoqubitdepolarizingnoise", 2, lambda qc: qc.twoqubitdepolarizingnoise(0.0, 0, 1),
     {"00": 1.0}, "probability 0 is the identity channel"),

    # ------------------------------- special -------------------------------
    ("reset", 1, lambda qc: (qc.x(0), qc.reset(0)), {"0": 1.0}, "reset returns the qubit to |0>"),
]


def build_circuit(num_qubits, build):
    qc = CunqaCircuit(num_qubits)
    build(qc)
    qc.measure_all()
    return qc


def check(counts, expected, num_qubits):
    """Compare measured counts against expected probabilities.

    Simulators pad the counts bitstring to different widths (AER trims to the
    circuit's num_clbits, Maestro/QuEST pad to the backend's), so only the low
    num_qubits characters -- the ones measure_all() wrote -- are compared.
    """
    total = sum(counts.values())
    if total == 0:
        return False, "no shots returned"

    # A result must account for every shot. A short total means the counts were
    # not produced by this circuit's execution -- most likely the result was
    # associated with the wrong job -- so flag it as such rather than reporting a
    # misleading distribution mismatch.
    shortfall = ""
    if total != SHOTS:
        shortfall = f"result holds {total} shots, expected {SHOTS}"

    if expected is SMOKE:
        return (not shortfall), shortfall

    narrowed = {}
    for bits, n in counts.items():
        key = bits[-num_qubits:].zfill(num_qubits)
        narrowed[key] = narrowed.get(key, 0) + n

    measured = {k: v / total for k, v in narrowed.items()}

    problems = []
    for bits, want in expected.items():
        got = measured.get(bits, 0.0)
        if abs(got - want) > TOLERANCE:
            problems.append(f"{bits}: expected {want:.3f}, got {got:.3f}")
    for bits, got in measured.items():
        if bits not in expected and got > TOLERANCE:
            problems.append(f"{bits}: unexpected outcome at {got:.3f}")

    if shortfall:
        problems.insert(0, shortfall)

    return (not problems), "; ".join(problems)


def run_suite(simulator, dynamic, gate_filter, verbose):
    claimed = SUPPORTED[simulator]
    applicable = [t for t in TESTS
                  if t[0] in claimed and (gate_filter is None or t[0] == gate_filter)]

    path = "dynamic (apply_gate)" if dynamic else "native (native_execute)"
    tag = f"{simulator}/{'dynamic' if dynamic else 'native'}"
    untested = sorted(claimed - {t[0] for t in TESTS} - {"measure"})

    print(f"\n{'=' * 78}\n  {simulator} [{path}]: {len(applicable)} of "
          f"{len(claimed)} declared basis gates\n{'=' * 78}")
    if untested and gate_filter is None:
        print(f"  (no test defined for: {', '.join(untested)})")
    if not applicable:
        return 0, 0

    family = qraise(1, "00:20:00", simulator=simulator, co_located=True)
    passed = failed = 0
    try:
        [qpu] = get_QPUs(co_located=True, family=family)

        for gate, num_qubits, build, expected, rationale in applicable:
            if expected is SMOKE:
                label = "runs"
            else:
                label = ", ".join(f"{k}:{v:.2f}" for k, v in sorted(expected.items()))
            try:
                qc = build_circuit(num_qubits, build)
                [result] = gather([run(qc, qpu, shots=SHOTS, is_dynamic=dynamic)])
                counts = result.counts
                result_id = getattr(result, "id", None)
            except Exception as exc:
                print(f"  FAIL  {gate:<22} [{tag}] expected {{{label}}} -> raised {exc}")
                if verbose:
                    traceback.print_exc()
                failed += 1
                continue

            ok, message = check(counts, expected, num_qubits)
            if ok:
                print(f"  PASS  {gate:<22} expected {{{label}}}")
                passed += 1
            else:
                print(f"  FAIL  {gate:<22} [{tag}] expected {{{label}}}")
                print(f"        got {counts}")
                if result_id != qc.id:
                    print(f"        !! result id {result_id} does not match circuit id {qc.id}")
                print(f"        {message}")
                print(f"        rationale: {rationale}")
                failed += 1
    finally:
        qdrop(family)

    return passed, failed


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--simulator", default="Aer",
                        help=f"one of {', '.join(ALL_SIMULATORS)}, or 'all'")
    parser.add_argument("--mode", default="both", choices=["dynamic", "native", "both"],
                        help="execution path to exercise (default: both)")
    parser.add_argument("--gate", default=None, help="run a single gate only")
    parser.add_argument("--verbose", action="store_true", help="print full tracebacks")
    args = parser.parse_args()

    if args.simulator == "all":
        simulators = ALL_SIMULATORS
    elif args.simulator in ALL_SIMULATORS:
        simulators = [args.simulator]
    else:
        parser.error(f"unknown simulator {args.simulator!r}; "
                     f"choose from {', '.join(ALL_SIMULATORS)} or 'all'")

    if args.gate is not None and args.gate not in {t[0] for t in TESTS}:
        parser.error(f"no test defined for gate {args.gate!r}")

    modes = {"dynamic": [True], "native": [False], "both": [True, False]}[args.mode]

    total_passed = total_failed = 0
    for simulator in simulators:
        for dynamic in modes:
            passed, failed = run_suite(simulator, dynamic, args.gate, args.verbose)
            total_passed += passed
            total_failed += failed

    print(f"\n{'=' * 78}\n  TOTAL: {total_passed} passed, {total_failed} failed\n{'=' * 78}")
    return 1 if total_failed else 0


if __name__ == "__main__":
    sys.exit(main())
