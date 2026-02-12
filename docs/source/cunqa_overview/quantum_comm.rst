
Quantum Communication
======================

This is the one paradigm that does not have a classical counterpart, since it is purely **quantum distribution**.

In this scheme quantum communication protocols such as **teledata** and **telegate** can be use to **share quantum entanglement** among the
states in each QPU involved.

**How to deploy**
------------------
To lauch an infrastructure incorporating quantum communicacions among vQPUs, the flag
:py:attr:`--quantum-comm` must be added

.. code-block:: bash

    qraise -n <num qpus> -t <max time> --quantum-comm [OTHER]

The above command line launches vQPUs with all-to-all quantum communications connectivity. For additional options
checkout :doc:`../reference/commands/qraise`. For a friendlier use by the Python API :py:func:`~cunqa.qpu.qraise` function
passing the argument :py:attr:`quantum_comm = True`.

**Circuits design**
-------------------

CUNQA allows the implementation of **teledata** and **telegate** protocols among circuits.

    1. **Creating both circuits** and adding the desired operations on them:

    .. code-block:: python

        circuit_1 = CunqaCircuit(num_qubits = 2, num_clbits = 2, id = "circuit_1")

        circuit_1.h(0)

        circuit_2 = CunqaCircuit(num_qubits = 2, num_clbits = 2, id = "circuit_2")

        circuit_2.x(1)

    2. **Implementing the quantum communication**:

    .. tab:: Teledata
        
        This protocol is similar to the classical communication directive but instead of sending a bit, here we **teleport a
        quantum state**. Even though in CUNQA we implement it in a friendlier way, the behind of scene of the protocol looks like
        the following:

        .. image:: /_static/teledata.png
            :alt: Teledata protocol
            :width: 60%
            :align: center

        where the state *a* at the first QPU is **teleported** to the first qubit of the second QPU. Here we see that a requirement
        is for the QPUs to **share a Bell pair** [CITA DQC REVIEW].

        At circuit level whithin CUNQA, class methods :py:meth:`~cunqa.circuit.core.CunqaCircuit.qsend` and :py:meth:`~cunqa.circuit.core.CunqaCircuit.qrecv`
        are designed for this purpose:

        .. code-block:: python

            circuit_1.qsend(qubit = 0, sending_circuit = "circuit_2")

            circuit_2.qrecv(qubit = 0, recving_circuit = "circuit_1")

        Here the state of qubit ``0`` at ``circuit_1`` is teleported to qubit ``0`` at ``circuit_2``. The origin qubit is then set to
        state ``0``. Then, we can continue to add instructions to both circuits.


    .. tab:: Telegate
        
        This protocol consists in exposing a qubit at one QPU to another for performing **multiple-qubits gates across QPUs**.
        The protocol itself consists in the following:

        .. image:: /_static/telegate.png
            :alt: Telegate protocol
            :width: 63%
            :align: center

        Here the quantum state *a* at the first QPU is participating in local operations at the other QPU.  We notice that a requirement
        is for the QPUs to **share a Bell pair** [CITA DQC REVIEW].

        Its circuit implementation at CUNQA begins by *exposing* the control qubit whithin a control context provided by the
        :py:meth:`~cunqa.circuit.core.CunqaCircuit.expose` method that returns a :py:class:`~cunqa.circuit.core.QuantumControlContext`
        for representing the remote qubit and a subcircuit to which the controlled instructions are applied:

        .. code-block:: python

            with circuit_1.expose(qubit = 0, target_circuit = "circuit_2") as rqubit, subcircuit:
                subcircuit.cx(rqubit, 1)

        This way, qubit ``0`` at ``circuit_1`` is participating in the :py:meth:`~cunqa.circuit.core.CunqaCircuit.cx` two-qubit gate
        together with qubit ``1`` at ``circuit_2``.


**Execution**
--------------

We obtain the :py:class:`~cunqa.qpu.QPU` objects associated to the displayed vQPUs through :py:func:`~cunqa.qpu.get_QPUs`. It is
important that those allow quanrtum communications, otherwise an error will be raised. When quantum communications are available,
classical communications directives are also permited at circuits.

For the distribution, function :py:func:`~cunqa.qpu.run` is used. By providing
the list of circuits and the list of :py:class:`~cunqa.qpu.QPU` objects we allow their mapping to the corresponding
vQPUs:

.. code-block:: python 

    qpus_list = get_QPUs()

    distributed_qjobs = run([circuit_1, circuit_2], qpus_list, shots = 100)

We can call for the results by the :py:func:`~cunqa.qjob.gather` function, passing the list of :py:class:`~cunqa.qjob.QJob`
objects:

.. code-block:: python

    results = gather(distributed_qjobs)


For quantum communications simulation, the vQPUs deployed actually share a conjunt simulator, therefore the call for results
is a blocking call that waits for the whole simulation to be over. Simulation time and output statistics can
be accessed by

.. code-block:: python

    times_list = [result.time_taken for result in results]

    counts_list = [result.counts for result in results]


**Basic example**
-----------------

Here we show two examples of implementing the remote construction of a three-qubit entangled state by **teledata** and **telegate**.
Further examples and use cases are listed in :doc:`../further_examples/further_examples`.

    .. tab:: Teledata

        .. code-block:: python

            import os, sys
            # In order to import cunqa, we append to the search path the cunqa installation path.
            # In CESGA, we install by default on the $HOME path as $HOME/bin is in the PATH variable
            sys.path.append(os.getenv("HOME"))

            from cunqa.qpu import get_QPUs, qraise, qdrop, run
            from cunqa.circuit import CunqaCircuit
            from cunqa.qjob import gather

            # 1. QPU deployment

            family_name = qraise(2, "01:00:00", quantum_comm=True, family_name = "qpus_quantum_comms")
            qpus = get_QPUs(family = family_name)

            # 2. Circuit design

            circuit_1 = CunqaCircuit(2, id="circuit_1")
            circuit_2 = CunqaCircuit(2, id="circuit_2")

            circuit_1.h(0)
            circuit_1.cx(0,1)

            # -------------- Teledata! ------------
            circuit_1.qsend(1, "circuit_2")
            circuit_2.qrecv(0, "circuit_1")
            # -------------------------------------

            circuit_2.cx(0,1)

            circuit_1.measure_all()
            circuit_2.measure_all()

            # 3. Execution

            qjobs = run([circuit_1, circuit_2], qpus, shots=1000)
            results = gather(qjobs)
            counts_list = [result.counts for result in results]

            for counts, qpu in zip(counts_list, qpus):

                print(f"Counts from vQPU {qpu.id}: {counts}")

            qdrop(family_name)

    .. tab:: Telegate

        .. code-block:: python

            import os, sys
            # In order to import cunqa, we append to the search path the cunqa installation path.
            # In CESGA, we install by default on the $HOME path as $HOME/bin is in the PATH variable
            sys.path.append(os.getenv("HOME"))

            from cunqa.qpu import get_QPUs, qraise, qdrop, run
            from cunqa.circuit import CunqaCircuit
            from cunqa.qjob import gather

            # 1. QPU deployment

            family_name = qraise(2, "01:00:00", quantum_comm=True, family_name = "qpus_quantum_comms")
            qpus = get_QPUs(family = family_name)

            # 2. Circuit design

            circuit_1 = CunqaCircuit(2, id="circuit_1")
            circuit_2 = CunqaCircuit(1, id="circuit_2")

            circuit_1.h(0)
            circuit_1.cx(0,1)


            # -------------------------- Telegate! -------------------------
            with circuit_1.expose(1, "circuit_2") as (rqubit, subcircuit):
                subcircuit.cx(rqubit, 0)
            # --------------------------------------------------------------

            circuit_1.measure_all()
            circuit_2.measure_all()

            # 3. Execution

            qjobs = run([circuit_1, circuit_2], qpus, shots=1000)
            results = gather(qjobs)
            counts_list = [result.counts for result in results]

            for counts, qpu in zip(counts_list, qpus):

                print(f"Counts from vQPU {qpu.id}: {counts}")

            qdrop(family_name)