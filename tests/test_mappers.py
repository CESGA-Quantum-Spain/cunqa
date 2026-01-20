# test_mappers.py
import json, os, sys
import numpy as np
from unittest.mock import Mock
import pytest

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

# If your classes live in a different module, adjust this import accordingly.
from cunqa.mappers import QJobMapper, QPUCircuitMapper
import cunqa.mappers as mappers_mod

# ------------------------
# QJobMapper tests
# ------------------------

def test_qjobmapper_init_sets_qjobs():
    qjobs = [object(), object(), object()]
    mapper = QJobMapper(qjobs)

    assert mapper.qjobs is qjobs


def test_qjobmapper_call_correctly(monkeypatch):
    def make_qjob_mock(name):
        q = Mock()
        q.name = name
        q.upgraded_with = None
        q.upgrade_parameters = Mock(side_effect=lambda params_list: setattr(q, "upgraded_with", params_list))
        return q

    q1, q2, q3 = make_qjob_mock("q1"), make_qjob_mock("q2"), make_qjob_mock("q3")
    mapper = QJobMapper([q1, q2, q3])

    gathered_with = {}
    def fake_gather(qjobs_list):
        gathered_with["qjobs"] = list(qjobs_list)
        return [f"result-{q.name}" for q in qjobs_list]
    monkeypatch.setattr(mappers_mod, "gather", fake_gather)

    population = [[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]]

    def cost_fn(result):
        return f"cost({result})"

    out = mapper(cost_fn, population)

    assert q1.upgraded_with == [1.0, 2.0]
    assert q2.upgraded_with == [3.0, 4.0]
    assert q3.upgraded_with == [5.0, 6.0]  # not zipped, so not upgraded

    assert gathered_with["qjobs"] == [q1, q2, q3]
    assert out == ["cost(result-q1)", "cost(result-q2)", "cost(result-q3)"]


@pytest.mark.parametrize("qjobs,population,", [
    ([object(), object()], [np.array([1.0, 2.0]), np.array([3.0, 4.0]), np.array([5.0, 6.0])]),
    ([object(), object(), object()], [np.array([1.0, 2.0]), np.array([3.0, 4.0])]),
])
def test_qjobmapper_call_without_population_and_qjob_sizes_matching(qjobs, population):
    
    with pytest.raises(ValueError) as _:
        mapper = QJobMapper(qjobs)
        _ = mapper(callable, population)


# ------------------------
# QPUCircuitMapper tests
# ------------------------

def test_qpucircuitmapper_init_accepts_quantumcircuit_and_stores_run_parameters():
    from qiskit import QuantumCircuit

    circuit = QuantumCircuit(1)
    qpus = [object(), object()]

    mapper = QPUCircuitMapper(qpus, circuit, shots=1000)

    assert mapper.qpus is qpus
    assert mapper.circuit is circuit
    assert mapper.run_parameters == {"shots": 1000}


def test_qpucircuitmapper_init_rejects_non_quantumcircuit():
    qpus = [object()]

    with pytest.raises(TypeError) as excinfo:
        QPUCircuitMapper(qpus, circuit={"not": "a qiskit circuit"})

def test_qpucircuitmapper_call_runs_circuits_round_robin_and_maps_results(monkeypatch):
    from qiskit import QuantumCircuit

    # Create a real QuantumCircuit (required by __init__).
    circuit = QuantumCircuit(1)

    assigned_params = []
    def fake_assign_parameters(params):
        assigned_params.append(params)
        return f"assembled({params})"
    monkeypatch.setattr(circuit, "assign_parameters", fake_assign_parameters)

    qpu_a, qpu_b = object(), object()
    qpus = [qpu_a, qpu_b]

    mapper = QPUCircuitMapper(qpus, circuit, shots=1000)

    run_calls = []
    def fake_run(circuit_assembled, qpu, **run_params):
        run_calls.append((circuit_assembled, qpu, dict(run_params)))
        return f"job({circuit_assembled}|{id(qpu)})"

    gathered_jobs = {}
    def fake_gather(jobs):
        jobs = list(jobs)
        gathered_jobs["jobs"] = jobs
        return [f"result({j})" for j in jobs]

    monkeypatch.setattr(mappers_mod, "run", fake_run)
    monkeypatch.setattr(mappers_mod, "gather", fake_gather)

    population = [
        [0.1, 0.2],
        [0.3, 0.4],
        [0.5, 0.6],
        [0.7, 0.8],
        [0.9, 1.0],
    ]

    def cost_fn(result):
        return f"cost={result}"

    out = mapper(cost_fn, population)

    # assign_parameters called once per population element
    assert assigned_params == population

    # run uses round-robin QPU selection: a, b, a, b, a
    assert len(run_calls) == 5
    assert run_calls[0][1] is qpu_a
    assert run_calls[1][1] is qpu_b
    assert run_calls[2][1] is qpu_a
    assert run_calls[3][1] is qpu_b
    assert run_calls[4][1] is qpu_a

    # run parameters passed through
    assert all(call[2] == {"shots": 1000} for call in run_calls)

    # Output maps cost_fn over gathered results in order
    assert out == [f"cost=result({j})" for j in gathered_jobs["jobs"]]


def test_qpucircuitmapper_call_wraps_qiskiterror_as_runtimeerror(monkeypatch):
    from qiskit import QuantumCircuit
    from qiskit.exceptions import QiskitError

    circuit = QuantumCircuit(1)

    def boom_assign_parameters(params):
        raise QiskitError("bad params")

    monkeypatch.setattr(circuit, "assign_parameters", boom_assign_parameters)

    mapper = QPUCircuitMapper([object()], circuit)

    with pytest.raises(RuntimeError) as excinfo:
        mapper(lambda r: r, population=[[1, 2, 3]])
