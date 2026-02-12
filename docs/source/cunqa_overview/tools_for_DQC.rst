Tools for DQC algorithms: add and union
=======================================

[...]

.. warning::

   Unlike the :py:func:`~cunqa.circuit.transformations.union`, the :py:func:`~cunqa.circuit.transformations.add` function does not always mesh well with communications.

   Its purpose is to build circuits modularly from simple parts, however communications greatly depend on the order of execution, so that much flexibility is not necessarily desirable.
   In particular, if two circuits that communicate with eachother are *added*, execution will stall as the circuit waits to communicate with the next subcircuit, which won't respond until execution progresses, waiting indefinitely.

.. literalinclude:: ../../../examples/add_withqsendERROR.py
    :language: python