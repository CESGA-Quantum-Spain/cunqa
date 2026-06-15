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

Besides all quantum operations and communication directives, it is common for users to want to 
combine two circuits or, conversely, to create several circuits from a single one. 
This module exists to enable this capability, defining the following functions. See the following 
explanations of the :py:func:`~cunqa.circuit.transformations.add`, 
:py:func:`~cunqa.circuit.transformations.union` and :py:func:`~cunqa.circuit.transformations.hsplit` 
functions to understand how they empower the study of :term:`DQC` algorithms.


.. tab:: add 
        
    The function :py:func:`~cunqa.circuit.transformations.add` takes an iterable of circuits as 
    input and returns a circuit that has the instructions of each circuit of the iterable in order. 
    Its purpose is to build circuits modularly from simple parts.

    .. autofunction:: cunqa.circuit.transformations.add

    .. warning::
        
        As communications greatly depend on the order of execution, adding circuits does not mesh well with communications.
        In particular, if two circuits that communicate with eachother were *added*, execution would stall as the circuit would wait
        to communicate with the next subcircuit, which wouldn't respond until execution progressed, waiting indefinitely. 

        For this reason, if two circuits on the iterable contain comunications between them, an **exception** would be raised.
        
    The :py:func:`~cunqa.circuit.transformations.add` function can be used to avoid redundant code, for example when creating a circuit for the Grover algorithm:

    .. code-block::

        grover_circuit = add([oracle, diffusor] * repeat_times)

    Full example of the :py:func:`~cunqa.circuit.transformations.add` function:

    .. literalinclude:: ../../../../examples/circuit_transformations/03-add.py
        :language: python

.. tab:: union

    Given two circuits with *n* and *m* qubits, their union returns a circuit with *n + m* qubits, where the operations of the former are applied to the first *n* qubits and those of the latter
    are applied to the last *m* qubits. If originally there were distributed instructions between the circuits, they would be replaced by local ones. In the following example we observe the union of two 
    simple circuits. 

    .. autofunction:: cunqa.circuit.transformations.union

    Basic usage:

    .. code-block:: python

        c1 = CunqaCircuit(2, id="circuit1")
        c1.h(0)
        c1.cx(0,1)

        c2 = CunqaCircuit(1, id="circuit2")
        c2.x(0)

        union_circuit = union([c1, c2])


    Full example of the :py:func:`~cunqa.circuit.transformations.union` function:

    .. literalinclude:: ../../../../examples/circuit_transformations/02-union.py
        :language: python


.. tab:: hsplit

    The function `hsplit` divides the set of qubits of a circuit into subcircuits, preserving the 
    instructions and substituing local 2-qubit gates by distributed gates if they involve qubits 
    from different subcircuits. The name `hspit` stands for *horizontal split*, as in the 
    conventional way to visually represent a circuit one would have to draw a horizontal line 
    to separate the qubits of the circuit in two subsets. 

    To divide a circuit `circuit_to_divide`, one should provide an additional argument that 
    determines how the circuits should be divided. This argument can be a **list with the number 
    of qubits for each subcircuit** (the lenght of the list determines the number of subcircuits), 
    or an **int specifying the number of subcircuits**, which would get an equal number of qubits 
    except possibly the last one, which would get the remainder if the number of qubits is not 
    cleanly divided by the int provided.

    .. autofunction:: cunqa.circuit.transformations.hsplit

    Basic syntax:

    .. code-block:: python

        # List with the number of qubits per subcircuit
        [c1, c2] = hsplit(circuit_to_divide, [2, 7])

        # Int specifying the number of resulting subcircuits
        [c1, c2] = hsplit(circuit_to_divide, 2)

    In particular, it could be checked that `union` and `hsplit` are inverses of eachother:

    .. code-block:: python

        # New circuit equivalent to circ is returned
        union(hsplit(circ, 2)) 
        # New circuits equal to circ1 and circ2 are returned
        hsplit(union(circ1, circ2), [circ1.num_qubits, circ2.num_qubits])

    Full example of the :py:func:`~cunqa.circuit.transformations.hsplit` function:

    .. literalinclude:: ../../../../examples/circuit_transformations/01-hsplit.py
        :language: python
