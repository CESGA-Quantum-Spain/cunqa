*****************************
Quantum-communications scheme
*****************************

The quantum-communications directives, together with simple examples, are explained in the :doc:`../overview/quantum_comm` section. 
This section will present a couple of examples are presented to highlight the use of :term:`quantum communications`.


In this first example we present an example of distributed QPE, so that the results obtained can be compared with the ones in :doc:`no_communications` and :doc:`classical_communications`.

.. literalinclude:: ../../../examples/qu_comm/04-qdistributed_QPE.py
      :language: python

The next example uses the :ref:`communication noise model <sec_comm_model>`: the vQPUs are deployed
with the timing and the model of its backend file, and the ``comm_model`` run argument sweeps the
success probability of the entanglement generator. Each run reports the fidelity of the teleported
state, the simulated time at which the receiver finishes and the mean generation time of its pair;
the last run prefetches the pair to overlap its generation with the local work.

.. literalinclude:: ../../../examples/qu_comm/09-communication_noise.json
      :language: json

.. literalinclude:: ../../../examples/qu_comm/09-communication_noise.py
      :language: python
