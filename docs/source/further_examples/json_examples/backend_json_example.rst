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

.. code-block:: json

    {
        "backend":{
            "name": "IdealBackend", 
            "version": "0.0.1",
            "description": "Example of an ideal backend",
            "n_qubits": 16, 
            "basis_gates": [
                "id", "h", "x", "y", "z", "cx", "cy", "cz", "ecr"
            ], 
            "custom_instructions": "",
            "gates": [],
            "coupling_map": [],
            "simulator":"Aer"
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
            "n_qubits": 16, 
            "basis_gates": [
                "id", "h", "x", "y", "z", "cx", "cy", "cz", "ecr"
            ], 
            "coupling_map": [],
            "simulator":"Aer",
            "noise_model": {
                "noise_properties_path": "/path/to/noise/properties/json",
                "thermal_relaxation": true,
                "readout_error": true,
                "gate_error": true
            }
        }
    }

