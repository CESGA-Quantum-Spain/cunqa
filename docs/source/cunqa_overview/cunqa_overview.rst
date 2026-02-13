CUNQA overview
===============

This section presents an overview of CUNQA, an emulator of Distributed Quantum Computing (DQC) architectures on HPC environments. Each of the architectures (or schemes) is built upon virtual QPUs, their basic building blocks. To interact with a infrastructure, a custom circuit creation interface was developed.

.. _sec_virtual_qpu:

Virtual QPU
------------

A virtual QPU (vQPU) is a classical process running on a HPC environment with an allocated set of classical resources responsible for simulating the behaviour of a real QPU.

.. figure:: /_static/VirtualQPU.png
    :alt: Virtual QPU
    :width: 60%
    :align: center

    Virtual QPU


.. rubric:: Made of two components

- Server component: manages communication user-vQPU.
- Simulator component: performs the actual execution.  Currently, the following simulators are available:

   - `AerSimulator <https://github.com/Qiskit/qiskit-aer/>`_
   - `MQT-DDSIM <https://github.com/munich-quantum-toolkit/ddsim>`_
   - `Qulacs <https://github.com/qulacs/qulacs>`_
   - `Maestro <https://github.com/QoroQuantum/maestro>`_
   - `CunqaSimulator <https://github.com/CESGA-Quantum-Spain/cunqasimulator>`_

The modular structure of CUNQA allows the implementation of other simulators on demand.

.. rubric:: How to deploy a vQPU?

The deployment of vQPUs is made through the bash command :doc:`../reference/commands/qraise`. Depending on the desired vQPU type, different argumets must be provided to the command. These arguments will be explored inside the description of each of the :ref:`sec_dqc_schemes` below. 

.. rubric:: GPU support

Additionally, we support the **GPU** execution provided by AerSimulator. This must be enabled at compile time as discussed in the :doc:`../installation/getting_started` section.

.. _sec_quantum_circs:

Quantum circuits
-----------------
As far as our knowledge extends, none of the most commonly used quantum circuit creation interfaces support the vQPU intercommunication instructions that we need to interact with CUNQA. Therefore, :py:class:`~cunqa.circuit.core.CunqaCircuit` was implemented as the basic tool to define distributed circuits. Its communication instructions will be explored in detail in their corresponding :ref:`sec_dqc_schemes` section below.

.. note:: 
   Apart from :py:class:`~cunqa.circuit.core.CunqaCircuit`, Qiskit QuantumCircuit and raw json instructions (see :doc:`../further_examples/json_examples/circuit_json_example`) are supported as circuit representations.

.. _sec_dqc_schemes:

DQC schemes
------------

As a Distributed Quantum Computing emulator, CUNQA supports the three basic DQC schemes:

- :doc:`embarrassingly_parallel`: classical distribution of quantum tasks with no communications at all.
- :doc:`classical_comm`: interchange classical bits between vQPUs at execution time.
- :doc:`quantum_comm`: implementation of teledata and telegate protocols.

Each of the previous sections will show:

1. How to deploy an infrastructure with the corresponding schema.
2. How to create and design circuits that fit that schema.
3. How to execute the circuits in the infrastructure.
4. A simple example.

.. toctree::
   :maxdepth: 1
   :hidden:

      Embarrassingly parallel <embarrassingly_parallel.rst>
      Classical communications <classical_comm.rst>
      Quantum communications <quantum_comm.rst>

.. _sec_tools_for_dqc_algorithms:

Tools for DQC algorithms
------------------------

.. _sec_hybrid_execution:

Hybrid execution
-----------------
CUNQA also allows working with real quantum hardware. In particular, the `CESGA's QMIO quantum computer <https://www.cesga.es/infraestructuras/cuantica/>`_ can be *deployed* alongside vQPUs to execute quantum tasks in a truly hybrid DQC infrastructure. Check :doc:`../reference/commands/qraise` to see how to deploy the real QPU.
      