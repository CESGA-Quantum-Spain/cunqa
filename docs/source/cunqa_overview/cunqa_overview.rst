CUNQA overview
===============

This section presents an overview of CUNQA, focusing on the supported three DQC schemes.

vQPUs
------

A virtual QPU (vQPU) is a classical process running on a HPC environment with an allocated set of classical resources responsible for simulating the behaviour of a real QPU.

It comprises two main components: 

- Server component: communication user-vQPU.
- Simulator component: performs the actual execution.  Currently, the following simulators are available [TODO: links]:
   - AerSimulator
   - MQT-DDSIM
   - Qulacs
   - Maestro
   - CunqaSimulator

The modular structure of CUNQA allows the implementation of other simulators.

Additionally, GPU.

The way of deploying vQPUs is through the bash command :doc:`../reference/commands/qraise` with additional arguments depending on the desired vQPU type. These arguments will be explored inside the description of any of the DQC schemes below. 

Quantum circuits
-----------------
:py:class:`~cunqa.circuit.core.CunqaCircuit` provides the necessary tools to model quantum circuits, including those with communications directives. These novel directives will be explored inside the description of any of the DQC schemes below.

.. note:: 
   Apart from :py:class:`~cunqa.circuit.core.CunqaCircuit`, Qiskit QuantumCircuit and raw json instructions [TODO: Link to json schema] are supported as circuit representations.


DQC schemes
------------

As a Distributed Quantum Computing emulator, CUNQA supports the three basic DQC schemes:

- :doc:`embarrassingly_parallel`: classical distribution of quantum tasks with no communications at all.
- :doc:`classical_comm`: interchange classical bits between vQPUs at execution time.
- :doc:`quantum_comm`: implementation of teledata and telegate protocols.

Each of the sections above shows how to deploy vQPU infrastructures with the corresponding scheme and how to build circuits with the appropiate communication directives. 

.. toctree::
   :maxdepth: 1
   :hidden:

      Embarrassingly parallel <embarrassingly_parallel.rst>
      Classical communications <classical_comm.rst>
      Quantum communications <quantum_comm.rst>


Tools for DQC algorithms
------------------------

Hybrid execution
-----------------
Real QPU
      