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