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