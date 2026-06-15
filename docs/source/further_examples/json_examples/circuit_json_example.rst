:orphan:

Raw Quantum Circuit JSON
-------------------------


**Simple example**

.. code-block:: json

  {
    "id":"simple_circuit", 
    "instructions": [
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
        "qubits":[0],
        "clbits":[0]
      },
      {
        "name":"measure",
        "qubits":[1],
        "clbits":[1]
      }
    ], 
    "config":
    {
      "shots": 1024,
      "num_qubits":2,
      "num_clbits":2,
      "device":
      {
        "device_name":"CPU",
        "target_devices":[]
      }
    },
    "is_dynamic":false, 
    "sending_to":[]
  }

**Classical Communications example**

.. code-block:: json

  {
    "id":"sender_circuit", 
    "instructions": [
      {
        "name":"h",
        "qubits":[0]
      },
      {
        "name":"measure",
        "qubits":[0],
        "clbits":[0]
      },
      {
        "name":"send",
        "clbits":[0],
        "circuits":["receiver_circuit"]
      }
    ], 
    "config":
    {
      "shots": 1024,
      "num_qubits":1,
      "num_clbits":1,
      "device":
      {
        "device_name":"CPU",
        "target_devices":[]
      }
    },
    "is_dynamic":true, 
    "sending_to":["receiver_circuit"]
  }

.. code-block:: json

  {
    "id":"receiver_circuit", 
    "instructions": [
      {
        "name":"recv",
        "clbits":[0],
        "circuits":["sender_circuit"]
      }
    ], 
    "config":
    {
      "shots": 1024,
      "num_qubits":1,
      "num_clbits":1,
      "device":
      {
        "device_name":"CPU",
        "target_devices":[]
      }
    },
    "is_dynamic":true, 
    "sending_to":[]
  }

**Quantum Communications example**

:term:`Quantum communication <quantum communications>` is expressed in the IR through the ``gen_ent`` directive, which requests a
shared entangled state on a :term:`communication qubit` between the participating circuits (identified by a
common ``tag``). The high-level :term:`teledata` and :term:`telegate` protocols (see :py:mod:`cunqa.qc_protocols`)
expand into a ``gen_ent`` directive followed by local gates, measurements, ``cif``/``endcif``
corrections and the classical ``send``/``recv`` directives shown above. The snippet below shows the
core ``gen_ent`` directive shared between two circuits (the :term:`comm qubit` is index ``1``, after the
single :term:`data qubit` ``0``):

.. code-block:: json

  {
    "id":"qsender_circuit",
    "instructions": [
      {
        "name":"h",
        "qubits":[0]
      },
      {
        "name":"gen_ent",
        "comm_qubit":1,
        "circuits":["qreceiver_circuit", "qsender_circuit"],
        "tag":"teledata"
      }
    ],
    "config":
    {
      "shots": 1024,
      "num_qubits":2,
      "num_clbits":2,
      "device":
      {
        "device_name":"CPU",
        "target_devices":[]
      }
    },
    "is_dynamic":true,
    "sending_to":["qreceiver_circuit"]
  }

.. code-block:: json

  {
    "id":"qreceiver_circuit",
    "instructions": [
      {
        "name":"gen_ent",
        "comm_qubit":1,
        "circuits":["qsender_circuit", "qreceiver_circuit"],
        "tag":"teledata"
      }
    ],
    "config":
    {
      "shots": 1024,
      "num_qubits":2,
      "num_clbits":2,
      "device":
      {
        "device_name":"CPU",
        "target_devices":[]
      }
    },
    "is_dynamic":true,
    "sending_to":[]
  }
