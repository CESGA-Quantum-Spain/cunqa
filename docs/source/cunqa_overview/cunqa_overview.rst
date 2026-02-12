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
Several circuit manipulation techniques have been implemented in CUNQA to empower the study of DQC algorithms. 
In particular, the following functions are available for building circuits from smaller pieces and for dividing circuits
into subcircuits:

- :py:func:`~cunqa.circuit.transformations.union`: combine circuits to produce another circuit with a larger set of qubits. For instance, given two circuits with `n` and `m` qubits, a circuit with `n+m` qubits with the corresponding instructions on each register would be obtained.
- :py:func:`~cunqa.circuit.transformations.add`: sum two circuits to obtain a deeper circuit which executes the instructions of the first summand and then those of the second summand.
- :py:func:`~cunqa.circuit.transformations.hsplit`: divide the set of qubits of a circuit into subcircuits. 
  For instance, given a `n+m` qubit circuit, two circuits with `n` and `m` qubits preserving the instructions would be obtained.

.. note::
   The function :py:func:`~cunqa.circuit.transformations.union` replaces distributed instructions between the circuits for local ones, 
   while :py:func:`~cunqa.circuit.transformations.hsplit` replaces local 2-qubit operations that involve different subcircuits into distributed instructions.
   Indeed, :py:func:`~cunqa.circuit.transformations.union` is the **inverse** of :py:func:`~cunqa.circuit.transformations.hsplit`.


These functions facilitate the inquiry into DQC algorithms as they can transform a set of communicated circuits into a single monolithic circuit (``union``),
and conversely, split a circuit implementing a monolithic algorithm into several circuits to run in communicated QPUs (``hsplit``).

Check :doc:`tools_for_DQC` for detailed examples of these functions.

.. toctree::
   :maxdepth: 1
   :hidden:

      Add and Union <tools_for_DQC.rst>
   

Hybrid execution
-----------------
Real QPU
      