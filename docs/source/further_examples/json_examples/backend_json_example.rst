Backend JSON 
=============

Here it is presented how to specify a :term:`backend` using JSON. Supported 
:term:`simulators <simulator>` must be indicated with the following aliases:

+--------------------------+----------------------------+
| **Siulator Name**        | **Alias for the backend**  |
+--------------------------+----------------------------+
|  AerSimulator            |  Aer                       |
+--------------------------+----------------------------+
|  MQT-DDSIM               | Munich                     |
+--------------------------+----------------------------+
|  Qulacs                  | Qulacs                     |
+--------------------------+----------------------------+
|  Maestro                 | Maestro                    |
+--------------------------+----------------------------+
|  CunqaSimulator          | Cunqa                      |
+--------------------------+----------------------------+

**Ideal backend**

This is the simplest backend example. The only thing to note is that the number of qubits is specified under the key _num_qubits_ as a list where the first element is the number of data qubits and the second element is the number of communication qubits.

.. code-block:: json

    {
        "backend":{
            "name": "IdealBackend", 
            "version": "0.0.1",
            "description": "Example of an ideal backend",
            "num_qubits": [16,0], 
            "basis_gates": [
                "id", "h", "x", "y", "z", "cx", "cy", "cz", "ecr"
            ], 
            "coupling_map": [],
        }
    }


**Noisy backend**

The simplest way to specify a noisy :term:`backend` is through a JSON file containing the desired 
noise properties in the format specified in :doc:`noise_properties_example`. Then, internally it 
will be convert to the specific noise model format supported for the corresponding :term:`simulator`.

.. code-block:: json

    {
        "backend": {
            "name": "NoisyBackend", 
            "version": "0.0.1",
            "description": "Example of a noisy backend",
            "num_qubits": [16, 0], 
            "basis_gates": [
                "id", "h", "x", "y", "z", "cx", "cy", "cz", "ecr"
            ], 
            "coupling_map": [],
            "noise_model": {
                "noise_properties_path": "/path/to/noise/properties/json",
                "thermal_relaxation": true,
                "readout_error": true,
                "gate_error": true
            }
        }
    }


**Quantum Communications backend**

The only unique feature of this type of backend is that it specifies the number of communication qubits as the second element in the list associated with the key _num_qubits_. In this example, the vQPU would have 16 data qubits and 3 communication qubits associated with it.

.. code-block:: json

    {
        "backend":{
            "name": "IdealBackend", 
            "version": "0.0.1",
            "description": "Example of an ideal backend",
            "num_qubits": [16,3], 
            "basis_gates": [
                "id", "h", "x", "y", "z", "cx", "cy", "cz", "ecr"
            ], 
            "coupling_map": [],
        }
    }

**Backend with simulated time**

By default every :term:`vQPU` runs on wall-clock time: ``time_taken`` only measures how long the
simulation took on the HPC. Adding a ``timing`` block gives the vQPU a logical clock, so that each
operation, classical message and entanglement generation has a simulated duration. All durations
are given in seconds and every field is optional:

.. code-block:: json

    {
        "backend": {
            "name": "TimedBackend",
            "version": "0.0.1",
            "description": "Example of a backend with simulated time",
            "num_qubits": [16, 0],
            "basis_gates": [
                "id", "h", "x", "y", "z", "cx", "cy", "cz", "ecr"
            ],
            "coupling_map": [],
            "timing": {
                "one_qubit_gate": 5e-8,
                "multi_qubit_gate": 3e-7,
                "measure": 1e-6,
                "reset": 1e-6,
                "classical_latency": 5e-7,
                "entanglement": 2e-6,
                "gate_durations": {"cx": 2.5e-7, "rz": 0}
            }
        }
    }

* ``one_qubit_gate`` (default 5e-8) and ``multi_qubit_gate`` (default 3e-7) are the durations of
  gates without an entry in ``gate_durations``. Noise channels and ``save_state`` take no time.
* ``measure`` and ``reset`` default to 1e-6.
* ``classical_latency`` (default 0) is added to the send time of every classical message the vQPU
  receives.
* ``entanglement`` (default 0) is the time a GHZ/EPR generation takes. The state becomes available
  on every vQPU taking part at the same simulated time.
* ``gate_durations`` overrides the duration of specific instructions, by name.

Operations are scheduled as soon as their qubits and classical bits are ready, so independent gates
run in parallel and gates inside a classically controlled block wait until the condition is known.
When the condition is not met the block is skipped, but its qubits still wait for the condition:
the controller cannot go on before knowing it.
The result then includes a ``simulated_time`` entry (see
:py:attr:`~cunqa.result.Result.simulated_time`) with the mean, minimum and maximum duration of a
shot and the mean idle time of each qubit. When :term:`vQPUs <vQPU>` communicate, every one of them
must define a ``timing`` block.
