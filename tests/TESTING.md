# Testing Guidelines

We use **pytest** together with `unittest.mock`.

All tests must be unit tests: isolated, explicit, and deterministic.

## Core Principles

- Mock all external dependencies (subprocess, filesystem, network, SLURM, etc.).
- Patch where the symbol is used, not where it is defined.
- Verify internal calls (`assert_called_once_with`, `assert_not_called`).
- Test both the happy path and error branches.
- Check internal state changes when relevant.

---

## Typical Test Structure

```python
def test_upgrade_parameters_calls_assign_and_serializes(
    monkeypatch, qclient_mock, circuit_ir, default_device
):
    future_mock = Mock()
    qclient_mock.send_parameters.return_value = future_mock

    job = QJob(qclient_mock, default_device, circuit_ir)
    job._result = Mock()
    job._future = Mock()

    assign_mock = Mock()
    monkeypatch.setattr(job, "assign_parameters_", assign_mock)

    monkeypatch.setattr(
        "cunqa.qjob.json.dumps",
        lambda obj, default=None: '{"theta":1.0}'
    )

    job._params = {"theta": 1.0}

    job.upgrade_parameters({"theta": 1.0})

    assign_mock.assert_called_once_with({"theta": 1.0})
    qclient_mock.send_parameters.assert_called_once_with(
        '{"params":{"theta":1.0}}'
    )
    assert job._future is future_mock
    assert job._updated is False
```

---

## Naming Convention

Use descriptive names:

```
test_<function>_<condition>_<expected_behavior>
```

Example:

```
test_run_passes_param_values_to_execute
```

## Location and execution

Tests are stored on the `tests` folder on the root directory of the CUNQA project. 
To execute all tests, move to that directory and use the simple command
```
pytest
```
on an environment where pytest is installed.

## C++ tests

The C++ components of the simulators and executors (simulated time, noise
channels and the quantum-communications engine) are tested with GoogleTest in
`tests/cpp`. They are built only when configuring with `-DCUNQA_BUILD_TESTS=ON`,
which fetches GoogleTest, and run with `ctest`:

```
cmake -B build/ -DCUNQA_BUILD_TESTS=ON
cmake --build build/ --target cunqa_cpp_tests
ctest --test-dir build/
```

The tests that run circuits on the simulators compare their statistics with
analytic predictions, with fixed seeds and tolerances of a few standard
deviations.
