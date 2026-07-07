
Quantum Communication
======================

.. figure:: /_static/QuantumCommScheme.png
    :alt: Quantum Communications Scheme
    :width: 60%
    :align: center

    Quantum Communications Scheme
    
This is the one paradigm that does not have a classical counterpart, since it is purely **quantum distribution**.

In this scheme :term:`quantum communication <quantum communications>` protocols such as :term:`teledata` and :term:`telegate` can be use to **share quantum entanglement** among the
states in each :term:`QPU` involved.

**How to deploy**
------------------
To lauch a set of :term:`vQPUs <vQPU>` incorporating :term:`quantum communications` among them, you must use the flag
:py:attr:`--quantum_comm` when deploying.

.. tab:: Bash command

    .. code-block:: bash

        qraise -n <num qpus> -t <max time> --quantum_comm [OTHER]

.. tab:: Python function

    .. code-block:: python

        family = qraise(4, <max time>, quantum_comm=True, [OTHER])
    

The above command line launches :term:`vQPUs <vQPU>` with all-to-all :term:`quantum communications` connectivity. For 
additional options in the Bash command checkout :doc:`../reference/commands/qraise`, and check 
:py:func:`~cunqa.qpu.qraise` for the Python function. Again, it is recomended to use the 
``--co-located`` flag (and ``co_located`` attribute in Python), as it allows to access the :term:`vQPUs <vQPU>` 
from every node, not just the one the :term:`vQPUs <vQPU>` are being set up. In this documentation we are going 
to consider that this flag is set.

**Circuits design**
-------------------

CUNQA allows the implementation of **teledata** and **telegate** protocols among circuits. These
protocols are provided as functions in the :py:mod:`cunqa.qc_protocols` subpackage, and are built on
top of the low-level :py:meth:`~cunqa.circuit.core.CunqaCircuit.gen_ent` entanglement-generation
primitive. In order to use them, and in a similar manner than with the :term:`classical communications`
model, several steps must be followed.

    1. **Creating both circuits**, reserving the :term:`communication qubits <comm qubit>` they will need. The comm
       qubits are declared with the tuple form ``CunqaCircuit((num_data_qubits, num_comm_qubits),
       num_clbits, ...)``, and the :py:meth:`~cunqa.circuit.core.CunqaCircuit.get_qubits` method
       returns the data and :term:`comm qubit` indices:

    .. code-block:: python

        circuit_1 = CunqaCircuit((2, 1), num_clbits=2, id="circuit_1")
        circuit_1.h(0)

        circuit_2 = CunqaCircuit((2, 1), num_clbits=2, id="circuit_2")

    2. **Implementing the quantum communication**:

    .. tab:: Teledata

        This protocol is similar to the :term:`classical communication <classical communications>` directive but instead of sending a
        bit, here we **teleport a quantum state**. Even though in CUNQA we implement it in a
        friendlier way, the behind of scene of the protocol looks like the following:

        .. image:: /_static/teledata.png
            :alt: Teledata protocol
            :width: 60%
            :align: center

        where the state *a* at the first :term:`QPU` is **teleported** to the first qubit of the second :term:`QPU`.
        Here we see that a requirement is for the :term:`QPUs <QPU>` to **share a Bell pair**.

        At circuit level whithin CUNQA, the functions :py:func:`~cunqa.qc_protocols.qsend`
        and :py:func:`~cunqa.qc_protocols.qrecv` are designed for this purpose:

        .. code-block:: python

            from cunqa.qc_protocols import qsend, qrecv

            data_qubits_1, comm_qubits_1 = circuit_1.get_qubits()
            data_qubits_2, comm_qubits_2 = circuit_2.get_qubits()

            # qsend(circuit, data_qubit, comm_qubit, clbits, recving_circuit, tag)
            qsend(circuit_1, data_qubits_1[0], comm_qubits_1[0], [0, 1], recving_circuit="circuit_2", tag="teledata")

            # qrecv(circuit, data_qubit, comm_qubit, clbits, control_circuit, tag)
            qrecv(circuit_2, data_qubits_2[0], comm_qubits_2[0], [0, 1], control_circuit="circuit_1", tag="teledata")

        Here the state of data qubit ``0`` at ``circuit_1`` is teleported to data qubit ``0`` at ``circuit_2``,
        using the :term:`comm qubit` ``0`` of each circuit and the classical bits ``[0, 1]`` to carry the
        Bell-measurement corrections. Then, we can continue to add instructions to both circuits.

        The matching ``tag`` links both calls. Without tags, the operation would be the same, except 
        when multiple calls to qsend and qrecv are made by the same :term:`vQPUs <vQPU>`. In this case, 
        without tags, qubits would be sent and received sequentially, not based on tag matching.


    .. tab:: Telegate

        This protocol consists in sharing a control qubit from one :term:`QPU` to another (or several) for
        performing **multiple-qubit gates across QPUs**. The protocol itself consists in the
        following:

        .. image:: /_static/telegate.png
            :alt: Telegate protocol
            :width: 63%
            :align: center

        Here the quantum state *a* at the first :term:`QPU` is participating in local operations at the
        other :term:`QPU`. We notice that a requirement is for the :term:`QPUs <QPU>` to **share a Bell pair**.

        Its circuit implementation at CUNQA opens a :term:`telegate` block with
        :py:func:`~cunqa.qc_protocols.cat_entangler`, which shares the control qubit onto the comm
        qubits of the other circuits; the receiving circuit then applies the controlled gate locally
        on its :term:`comm qubit`, and the block is closed with
        :py:func:`~cunqa.qc_protocols.cat_disentangler`:

        .. code-block:: python

            from cunqa.qc_protocols import cat_entangler, cat_disentangler

            data_qubits_1, comm_qubits_1 = circuit_1.get_qubits()
            data_qubits_2, comm_qubits_2 = circuit_2.get_qubits()

            cat_entangler(
                [circuit_1, circuit_2],
                data_qubits_1[0],
                [comm_qubits_1[0], comm_qubits_2[0]],
                [0, 0],
                tag="telegate"
            )

            circuit_2.cx(comm_qubits_2[0], data_qubits_2[0])

            cat_disentangler(
                [circuit_1, circuit_2],
                data_qubits_1[0],
                [comm_qubits_2[0]],
                [0, 1],
                [0, 0]
            )

        This way, qubit ``0`` at ``circuit_1`` acts as the control of the
        :py:meth:`~cunqa.circuit.core.CunqaCircuit.cx` two-qubit gate applied together with the data
        qubit at ``circuit_2``.


**Execution**
--------------

We obtain the :py:class:`~cunqa.qpu.QPU` objects associated to the displayed :term:`vQPUs <vQPU>` through 
:py:func:`~cunqa.qpu.get_QPUs`, same as with the other methods. It is important that those allow 
:term:`quantum communications`, otherwise an error will be raised. When :term:`quantum communications` are 
available, classical-communication directives are also permited at circuits.

For the distribution, function :py:func:`~cunqa.qpu.run` is used. By providing
the list of circuits and the list of :py:class:`~cunqa.qpu.QPU` objects we allow their mapping to 
the corresponding :term:`vQPUs <vQPU>`:

.. code-block:: python 

    qpus_list = get_QPUs(family=family, co_located=True)

    distributed_qjobs = run([circuit_1, circuit_2], qpus_list, shots = 1024)

We can call for the results by the :py:func:`~cunqa.qjob.gather` function, passing the list of 
:py:class:`~cunqa.qjob.QJob` objects:

.. code-block:: python

    results = gather(distributed_qjobs)


For :term:`quantum communications` simulation, the :term:`vQPUs <vQPU>` deployed actually share a conjunt :term:`simulator`, 
therefore the call for results is a blocking call that waits for the whole simulation to be over. 
Simulation time and output statistics can be accessed by

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
            from cunqa.qc_protocols import qsend, qrecv

            # 1. QPU deployment

            family = qraise(2, "01:00:00", quantum_comm=True, family="qpus_quantum_comms")
            qpus = get_QPUs(family=family)

            # 2. Circuit design (each circuit reserves 2 data + 1 comm qubits)

            circuit_1 = CunqaCircuit((2, 1), 2, id="circuit_1")
            circuit_2 = CunqaCircuit((2, 1), 2, id="circuit_2")

            data_qubits_1, comm_qubits_1 = circuit_1.get_qubits()
            data_qubits_2, comm_qubits_2 = circuit_2.get_qubits()

            circuit_1.h(0)
            circuit_1.cx(0, 1)

            # -------------- Teledata! ------------
            qsend(circuit_1, data_qubits_1[0], comm_qubits_1[0], [0, 1], recving_circuit="circuit_2", tag="teledata")
            qrecv(circuit_2, data_qubits_2[0], comm_qubits_2[0], [0, 1], control_circuit="circuit_1", tag="teledata")
            # -------------------------------------

            circuit_2.cx(0, 1)

            circuit_1.measure_all()
            circuit_2.measure_all()

            # 3. Execution

            qjobs = run([circuit_1, circuit_2], qpus, shots=1000)
            results = gather(qjobs)
            counts_list = [result.counts for result in results]

            for counts, qpu in zip(counts_list, qpus):

                print(f"Counts from vQPU {qpu.id}: {counts}")

            qdrop(family)

    .. tab:: Telegate

        .. code-block:: python

            import os, sys
            # In order to import cunqa, we append to the search path the cunqa installation path.
            # In CESGA, we install by default on the $HOME path as $HOME/bin is in the PATH variable
            sys.path.append(os.getenv("HOME"))

            from cunqa.qpu import get_QPUs, qraise, qdrop, run
            from cunqa.circuit import CunqaCircuit
            from cunqa.qjob import gather
            from cunqa.qc_protocols import cat_entangler, cat_disentangler

            # 1. QPU deployment

            family = qraise(2, "01:00:00", quantum_comm=True, family="qpus_quantum_comms")
            qpus = get_QPUs(family=family)

            # 2. Circuit design (each circuit reserves 1 data + 1 comm qubit)

            circuit_1 = CunqaCircuit((1, 1), 2, id="circuit_1")
            circuit_2 = CunqaCircuit((1, 1), 2, id="circuit_2")

            data_qubits_1, comm_qubits_1 = circuit_1.get_qubits()
            data_qubits_2, comm_qubits_2 = circuit_2.get_qubits()

            circuit_1.h(data_qubits_1[0])

            # -------------------------- Telegate! -------------------------
            cat_entangler(
                [circuit_1, circuit_2],
                data_qubits_1[0],
                [comm_qubits_1[0], comm_qubits_2[0]],
                [0, 0],
                tag="telegate"
            )

            circuit_2.cx(comm_qubits_2[0], data_qubits_2[0])

            cat_disentangler(
                [circuit_1, circuit_2],
                data_qubits_1[0],
                [comm_qubits_2[0]],
                [0, 1],
                [0, 0]
            )
            # --------------------------------------------------------------

            circuit_1.measure(data_qubits_1[0], 0)
            circuit_2.measure(data_qubits_2[0], 0)

            # 3. Execution

            qjobs = run([circuit_1, circuit_2], qpus, shots=1000)
            results = gather(qjobs)
            counts_list = [result.counts for result in results]

            for counts, qpu in zip(counts_list, qpus):

                print(f"Counts from vQPU {qpu.id}: {counts}")

            qdrop(family)
