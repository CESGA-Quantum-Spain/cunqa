CunqaCircuit
============

This section introduces the ``CunqaCircuit`` class, the core abstraction used in CUNQA to describe
quantum circuits with explicit classical and quantum communication directives.

Motivation and Design
---------------------

``CunqaCircuit`` is intentionally designed to resemble
``qiskit.circuit.QuantumCircuit`` in order to reduce the learning curve for users familiar with
Qiskit. However, this similarity is purely superficial.

The main reason for introducing a custom circuit abstraction is that Qiskit does not natively
support communication primitives between distributed circuits. Such primitives are central to
CUNQA, as the framework targets distributed quantum computing scenarios in which circuits executed
on different virtual QPUs must exchange classical information or quantum states.

Classically Controlled Operations
---------------------------------

Although Qiskit provides mechanisms such as ``c_if`` and ``if_else`` for classical control, their
usage can quickly become verbose and difficult to read when describing non-trivial control flow.

CUNQA introduces the ``cif`` context manager to express classically controlled blocks in a more
natural and explicit way:

.. code-block:: python

    c = CunqaCircuit(2, 2)
    c.h(0)
    c.measure(0, 0)

    with c.cif(0) as cgates:
        cgates.x(1)

The operations inside the ``cif`` block are executed only if the value stored in classical bit
``0`` is equal to ``1``. Currently, ``cif`` does not support an explicit *else* branch. This design
choice reflects the requirements of the distributed algorithms currently supported by CUNQA.
Future extensions may introduce *else* support if needed.

Classical Communication Between Circuits
----------------------------------------

CUNQA allows circuits to exchange classical information explicitly. Measurement outcomes obtained
in one circuit can be transmitted to another circuit and used to condition local operations.

This enables the description of distributed protocols in which decision-making depends on remote
measurement results, while keeping the circuit-level description explicit and readable.

Quantum Communication and Teleportation-Based Protocols
-------------------------------------------------------

In addition to classical communication, CUNQA supports quantum communication primitives. When the
underlying virtual QPUs provide quantum connectivity, qubits can be transferred between circuits.

Internally, these operations rely on teleportation-based protocols such as *teledata* and
*telegate*. These protocols are handled transparently by the framework, allowing users to express
distributed quantum workflows without implementing the low-level details.

Teledata Protocol
~~~~~~~~~~~~~~~~~

The teledata protocol enables the reconstruction of an unknown quantum state at a remote location
without physically transmitting the system itself.

Within CUNQA, sending a qubit from one circuit to another automatically triggers the appropriate
teleportation procedure. After the transfer, the sent qubit is reset and can be reused.

Telegate Protocol
~~~~~~~~~~~~~~~~~

Quantum gate teleportation (telegate) reduces connectivity requirements by replacing non-local
two-qubit gates with local operations, classical communication, and shared entanglement.

In CUNQA, telegate is exposed through the ``expose`` method and managed by the
``QuantumControlContext`` class, allowing remote qubits to act as controls for local operations in
a structured and explicit manner.

References
----------

- `Review of Distributed Quantum Computing. From single QPU to High Performance Quantum Computing
  <https://arxiv.org/abs/2404.01265>`_
