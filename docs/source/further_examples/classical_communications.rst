*******************************
Classical-communications scheme
*******************************

The classical-communications directives, together with simple examples, are explained in the :doc:`../cunqa_overview/classical_comm` section. 
This section will present a couple of examples are presented to highlight the use of classical communications.


In this first example we present a cyclic exchange of classic bits between 3 circuits:

.. literalinclude:: ../../../examples/python/cl_comm/02-cyclic.py
      :language: python

In this second example we present the distributed `iterative QPE <https://arxiv.org/abs/quant-ph/0610214>`_ in several vQPUs. Compare with the QPE presented in :doc:`no_communications` and :doc:`quantum_communications`.

.. literalinclude:: ../../../examples/python/cl_comm/03-distributed_QPE.py
      :language: python

