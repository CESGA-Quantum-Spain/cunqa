# System Tests

These are **system tests**, not unit tests. They allocate real vQPUs through SLURM
(`qraise`) and execute circuits on them, so they need a working cluster
environment and an installed CUNQA.

They deliberately live outside `tests/`. That directory holds mocked pytest unit
tests (see `tests/TESTING.md`), and `pytest` would otherwise collect anything named
`test_*.py` and try to run it as part of the unit test suite.

Run them directly with the interpreter, never through `pytest`.

## `check_gate_support.py`

Executes every gate each simulator declares in its `*_BASIS_GATES` array and
compares the resulting counts against an analytically derived distribution.

```bash
python check_gate_support.py                    # Aer, both execution paths
python check_gate_support.py --simulator Quest
python check_gate_support.py --simulator all    # every simulator, in turn
python check_gate_support.py --mode native      # only native_execute
python check_gate_support.py --gate cswap       # one gate, for a quick check
```

### What it covers

Both execution paths, because they are two independent implementations of the same
gate set. `NCExecutor::execute` chooses between them:

| `config.is_dynamic` | path | code under test |
| --- | --- | --- |
| `false` | `simulator_->native_execute(circuit)` | the per-simulator JSON conversion |
| `true` | `custom_execute_()` | the `apply_gate` overloads |

A plain circuit has `is_dynamic == False` (only `cif`, `recv`, ... set it), so a
test that does not force the flag silently exercises only `native_execute`. The
script passes `is_dynamic` explicitly as a run argument for each path.

### Reading the output

Every circuit is built so the ideal outcome is a **single deterministic bitstring**
at probability 1.0; the few genuinely random ones (`h`, `hz2`, `ecr`, `gp`) state
their split explicitly. A broken gate therefore either raises (`Gate X not
supported by ...`, `Instruction not suported!`) or spreads its counts.

Bitstrings follow the cunqa/Qiskit convention: **qubit 0 is the rightmost
character**, so `101` means q2=1, q1=0, q0=1. Simulators pad to different widths,
so only the low `num_qubits` characters are compared.

A few gates are marked `runs` instead of a distribution: their semantics come from
a closed or intentionally random source (Maestro's `k`, `randomunitary`, the
non-unitary Pauli gadget), so the check is only that they execute and return
counts. Everything else is checked numerically.

### Keeping it in sync

`SUPPORTED` mirrors the `*_BASIS_GATES` arrays in
`src/sim/simulators/<Sim>/<sim>_simulator_adapter.hpp`. When a gate is added to a
basis list, add it there too — the suite then starts testing it on every simulator
that claims it. Gates claimed but without a test are listed at the top of each run
so the gap is visible.

## Known deviations

Differences that are real but that this suite cannot catch, recorded so they are not
mistaken for passing behaviour.

### Qsim `cp` uses the opposite sign

cunqa follows the **AER/Qiskit** convention, `CP(phi) = diag(1, 1, 1, exp(+i*phi))`.
qsim's `GateCP` is built as `diag(1, 1, 1, cos(phi) - i*sin(phi))`, i.e.
`exp(-i*phi)`, so Qsim disagrees in sign with every other simulator for any angle
other than `0` and `pi`.

The `cp` test uses `pi`, where `exp(+i*pi) == exp(-i*pi) == -1`, so it passes on
every simulator and the mismatch stays invisible. `cp(pi/2)` produces a different
state on Qsim than on Aer. The call sites are marked `KNOWN DEVIATION` in
`src/sim/simulators/Qsim/qsim_simulator_adapter.cpp`; negating the angle there
brings Qsim in line with the rest.

More generally, a test that pins a parametric gate only at `pi` cannot distinguish
a sign convention. Adding a second, non-symmetric angle to the parametric tests
would close that blind spot for the whole gate set.

### Maestro `reset` only works on the dynamic path

`MaestroSimulatorAdapter::apply_gate` implements reset through maestrolib's
`ApplyReset`, so it works when `is_dynamic` is true. maestrolib's **JSON** circuit
format has no reset operation at all: `Json.h` parses `"measure"` plus a fixed
`gatesMap` of 32 gate names, and anything else raises `Gate type not supported.`

Because `native_execute` is the default path, `reset` was removed from
`MAESTRO_BASIS_GATES` rather than advertising something a plain circuit cannot do.
Nothing was lost: `basis_gates` is only used by `Backend::load_common_fields` to
validate a user-declared list, so dynamic-path reset still works.

maestrolib's `SimpleExecute` picks its parser from the first character — JSON for
`{`/`[`, otherwise QASM 2.0 — and its QASM parser *does* support reset. Routing
Maestro's native execution through QASM would let reset be advertised again. That
needs `src/utils/helpers/json_to_qasm2.hpp` repaired first: it is currently
included but never called, reads `qubits` as an array when one-qubit gates emit a
scalar, has no reset case, and returns its error message as the circuit string.
