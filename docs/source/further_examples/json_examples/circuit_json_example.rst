:orphan:

Raw Quantum Circuit JSON
-------------------------


**Simple example**

.. code-block:: json

  {
    "id":"simple_circuit", 
    "instructions": {
      {
        "name":"h",
        "qubits":[0]
      },
      {
        "name":"cx",
        "qubits":[0,1]
      },
      {
        "name":"measure",
        "qubits":[0]
      },
      {
        "name":"measure",
        "qubits":[1]
      },
    }, 
    "num_qubits": 2,
    "num_clbits": 2,
    "classical_registers": [0, 1],
    "quantum_registers": [0, 1],  
    "is_dynamic":false, 
    "sending_to":[]
  }

**Classical Communications example**

.. code-block:: json

  {
    "id":"sender_circuit", 
    "instructions": {
      {
        "name":"h",
        "qubits":[0]
      },
      {
        "name":"measure",
        "qubits":[0]
      },
      {
        "name":"send",
        "clbits":[0],
        "circuits":["receiver_circuit"]
      }
    }, 
    "num_qubits": 1,
    "num_clbits": 1,
    "classical_registers": [0],
    "quantum_registers": [0],  
    "is_dynamic":true, 
    "sending_to":["receiver_circuit"]
  }

.. code-block:: json

  {
    "id":"receiver_circuit", 
    "instructions": {
      {
        "name":"recv",
        "clbits":[0],
        "circuits":["sender_circuit"]
      }
    }, 
    "num_qubits": 1,
    "num_clbits": 1,
    "classical_registers": [0],
    "quantum_registers": [0],  
    "is_dynamic":true, 
    "sending_to":[]
  }

**Quantum Communications example**

.. code-block:: json

  {
    "id":"qsender_circuit", 
    "instructions": {
      {
        "name":"h",
        "qubits":[0]
      },
      {
        "name":"qsend",
        "qubits":[0],
        "circuits":["qreceiver_circuit"]
      },
      {"name":"measure",
        "qubits":[0]
      }
    }, 
    "num_qubits": 1,
    "num_clbits": 1,
    "classical_registers": [0],
    "quantum_registers": [0],  
    "is_dynamic":true, 
    "sending_to":[]
  }

.. code-block:: json

  {
    "id":"qreceiver_circuit", 
    "instructions": {
      {
        "name":"qrecv",
        "qubits":[0],
        "circuits":["qsender_circuit"]
      },
      {
        "name":"measure",
        "qubits":[0]
      }
    }, 
    "num_qubits": 1,
    "num_clbits": 1,
    "classical_registers": [0],
    "quantum_registers": [0],  
    "is_dynamic":true, 
    "sending_to":[]
  }