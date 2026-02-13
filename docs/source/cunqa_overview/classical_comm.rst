Classical Communications
=========================

.. figure:: /_static/ClassicalCommScheme.png
    :alt: Classical Communications scheme
    :width: 60%
    :align: center

    Classical Communications scheme

This is the **classical conterpart** of classical parallelization processes that **interchange information in execution time**,
such as MPI.

In the context of quantum circuits, this communication done by sending **classical bits** from measurement results.
This is interesting for using the received bits to **classically condition operations**. That is, if the outcome of the remote bit
is ``1``, quantum instructions will be applied to the specified local qubits, else, nothing is applied.

**How to deploy**
---------------------
To lauch an infrastructure incorporating classical communicacions among vQPUs, the flag
:py:attr:`--classical-comm` must be added

.. code-block:: bash

    qraise -n <num qpus> -t <max time> --classical-comm [OTHER]

The above command line launches vQPUs with all-to-all classical communications connectivity. For additional options
checkout :doc:`../reference/commands/qraise`. For a friendlier use by the Python API :py:func:`~cunqa.qpu.qraise` function
passing the argument :py:attr:`classical_comm = True`.


**Circuits design**
----------------------
In order to classically condition local operations, we must first implement the communication of such
classical bits. With this purpose, :py:class:`~cunqa.circuit.core.CunqaCircuit` class incorporates the
:py:meth:`~cunqa.circuit.core.CunqaCircuit.send` and :py:meth:`~cunqa.circuit.core.CunqaCircuit.recv` class methods.
The process follows as:

    1. **Creating** both **circuits** and adding the desired operations on them:

    .. code-block:: python

        circuit_1 = CunqaCircuit(num_qubits = 2, num_clbits = 2, id = "circuit_1")

        circuit_1.h(0)

        circuit_2 = CunqaCircuit(num_qubits = 2, num_clbits = 2, id = "circuit_2")

        circuit_2.x(1)


    If no *id* is not provided, it will be generated and accesed by the class attribute :py:attr:`~cunqa.circuit.core.CunqaCircuit.id`.

    2. **Measuring and seding a classical bit** from ``circuit_1`` and receiving it at ``circuit_2``:

    .. code-block:: python

        circuit_1.measure(qubit = 0, clbit = 0)

        circuit_1.send(clbits = 0, recving_circuit = "circuit_2")

        circuit_2.recv(clbits = 1, sending_circuit = "circuit_1")

    At ``circuit_2`` we receive the bit and store it at possition ``1`` of the local classical register.

    
    3. **Classically controlling a quantum operation** by a control context provided by the :py:meth:`~cunqa.circuit.core.CunqaCirucit.cif`
    method:

    .. code-block:: python

        with circuit_2.cif(clbits = 1) as cgates:
            cgates.x(0)

    We use the outcome stored at clbit ``1`` to classically control a :py:meth:`~cunqa.circuit.core.CunqaCircuit.x` gate
    at qubit ``0``.


**Execution**
-------------

We obtain the :py:class:`~cunqa.qpu.QPU` objects associated to the displayed vQPUs through :py:func:`~cunqa.qpu.get_QPUs`. It is
important that those allow classical communications, otherwise an error will be raised.

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

This is a blocking call, since here the function waits both executions to be done. Simulation times and output statistics can
be accessed by

.. code-block:: python

    times_list = [result.time_taken for result in results]

    counts_list = [result.counts for result in results]



**Basic example**
------------------

Here we show an example on a classical communication performed from one circuit to another to classically control a quantum
operation. Further examples and use cases are listed in :doc:`../further_examples/further_examples`.


.. code-block:: python

    import os, sys
    # In order to import cunqa, we append to the search path the cunqa installation path.
    # In CESGA, we install by default on the $HOME path as $HOME/bin is in the PATH variable
    sys.path.append(os.getenv("HOME"))

    from cunqa.qpu import get_QPUs, qraise, qdrop, run
    from cunqa.circuit import CunqaCircuit
    from cunqa.qjob import gather

    # 1. QPU deployment

    family_name = qraise(2, "01:00:00", classical_comm=True, family_name = "qpus_class_comms")
    qpus = get_QPUs(family = family_name)

    # 2. Circuit design with classical communications directives

    circuit_1 = CunqaCircuit(2, 2, id="circuit_1")
    circuit_2 = CunqaCircuit(2, 2, id="circuit_2")

    circuit_1.h(0)
    circuit_1.measure(0,0)
    circuit_1.send(0, recving_circuit = "circuit_2")
    circuit_1.measure(1,1)

    circuit_2.recv(0, sending_circuit = "circuit_1")

    with circuit_2.cif(0) as cgates:
        cgates.x(1)

    circuit_2.measure(0,0)
    circuit_2.measure(1,1)

    # 3. Execution

    distributed_qjobs = run([circuit_1, circuit_2], qpus, shots=1000)
    results = gather(distributed_qjobs)
    counts_list = [result.counts for result in results]

    for counts, qpu in zip(counts_list, qpus):

        print(f"Counts from vQPU {qpu.id}: {counts}")

    # 4. Release classical resources
    qdrop(family_name)
