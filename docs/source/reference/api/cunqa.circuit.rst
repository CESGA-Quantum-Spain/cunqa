#############
cunqa.circuit
#############

.. automodule:: cunqa.circuit
   :no-members:

This module defines :class:`~cunqa.circuit.core.CunqaCircuit`. Many circuit design models already 
exist, the best-known being Qiskit's :class:`QuantumCircuit`. The motivation behind the design of 
CunqaCircuit does not stem from identifying problems in existing models, but from the lack of 
communication directives, which are so vital in CUNQA as a DQC platform. In this way, 
:py:class:`~cunqa.circuit.core.CunqaCircuit` class allows users to describe quantum circuits that 
include not only local quantum operations, but also classical and quantum communication directives 
between distributed circuits. A :py:class:`~cunqa.circuit.core.CunqaCircuit` can, therefore, 
represent both computation and communication in DQC scenarios.

.. autoclass:: cunqa.circuit.core.CunqaCircuit
   :no-members:
   
Transformations
===============

.. automodule:: cunqa.circuit.transformations
   :no-members:

Besides all quantum operations and communication directives, it is common for users to want to 
combine two circuits or, conversely, to create several circuits from a single one. 
This module exists to enable this capability, defining the following functions.

.. autofunction:: cunqa.circuit.transformations.vsplit
.. autofunction:: cunqa.circuit.transformations.hsplit
.. autofunction:: cunqa.circuit.transformations.union
.. autofunction:: cunqa.circuit.transformations.add