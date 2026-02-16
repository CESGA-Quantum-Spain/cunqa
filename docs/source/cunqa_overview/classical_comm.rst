Classical Communications
=========================

.. figure:: /_static/ClassicalCommScheme.png
    :alt: Classical Communications scheme
    :width: 60%
    :align: center

    Classical Communications scheme

This model extends the embarrassingly parallel approach by allowing processes to exchange 
**classical information during execution**, similarly to how MPI programs operate in classical 
HPC environments.

In the context of quantum circuits, this communication is achieved by transmitting classical bits 
obtained from measurement outcomes. This enables the use of received bits to perform classically 
conditioned operations. In other words, if the value of a remote bit is ``1``, specific quantum 
operations are applied to the designated local qubits.

**How to deploy**
---------------------
To lauch a set of vQPUs incorporating classical communications among them, you must use the flag
:py:attr:`--classical-comm` when deploying.

.. tab:: Bash command

    .. code-block:: bash

        qraise -n <num qpus> -t <max time> --classical-comm [OTHER]

.. tab:: Python function

    .. code-block:: python

        family = qraise(4, <max time>, classical_comm=True, [OTHER])
    

The above command line launches vQPUs with all-to-all classical communications connectivity. For 
additional options in the Bash command checkout :doc:`../reference/commands/qraise`, and check 
:py:func:`~cunqa.qpu.qraise` for the Python function. Again, it is recomended to use the 
``--co-located`` flag (and ``co_located`` attribute in Python), as it allows to access the vQPUs 
from every node, not just the one the vQPUs are being set up. In this documentation we are going 
to consider that this flag is set.


**Circuits design**
----------------------
In order to classically condition local operations with remote bits, we must first implement the 
communication of such bits. With this purpose, :py:class:`~cunqa.circuit.core.CunqaCircuit` class 
incorporates the :py:meth:`~cunqa.circuit.core.CunqaCircuit.send` and 
:py:meth:`~cunqa.circuit.core.CunqaCircuit.recv` class methods; and these steps must be followed:

    1. **Creating** both **circuits** and adding the desired operations on them:

    .. code-block:: python

        circuit_1 = CunqaCircuit(num_qubits=2, num_clbits=2, id="circuit_1")

        circuit_1.h(0)

        circuit_2 = CunqaCircuit(num_qubits=2, num_clbits=2, id="circuit_2")

        circuit_2.x(1)


    If no *id* is not provided, it will be generated and accesed by the class attribute 
    :py:attr:`~cunqa.circuit.core.CunqaCircuit.id`.

    2. Measuring and **sending** a classical bit from ``circuit_1`` and **receiving** it at 
       ``circuit_2``:

    .. code-block:: python

        circuit_1.measure(qubit=0, clbit=0)

        circuit_1.send(clbits=0, recving_circuit="circuit_2")

        circuit_2.recv(clbits=1, sending_circuit="circuit_1")

    At ``circuit_2`` we receive the bit and store it at possition ``1`` of the local classical register.
    
    3. **Classically controlling a quantum operation** employing the :py:meth:`~cunqa.circuit.core.CunqaCirucit.cif`
    method:

    .. code-block:: python

        with circuit_2.cif(clbits=1) as subcircuit:
            subcircuit.x(0)

    We use the outcome stored at clbit ``1`` to classically control a :py:meth:`~cunqa.circuit.core.CunqaCircuit.x` 
    gate at qubit ``0``.


**Execution**
-------------

We obtain the :py:class:`~cunqa.qpu.QPU` objects associated to the displayed vQPUs with 
:py:func:`~cunqa.qpu.get_QPUs`, as a common step between the three models. It is important that 
those allow classical communications---i.e., that the aforementioned ``classical-comm`` flag was 
set---, otherwise an error will be raised.

For the distribution, function :py:func:`~cunqa.qpu.run` is used. By providing
the list of circuits and the list of :py:class:`~cunqa.qpu.QPU` objects we allow their mapping to the corresponding
vQPUs:

.. code-block:: python 

    qpus_list = get_QPUs(family=family, co_located=True)

    distributed_qjobs = run([circuit_1, circuit_2], qpus_list, shots = 1024)

We can call for the results by the :py:func:`~cunqa.qjob.gather` function, passing the list of 
:py:class:`~cunqa.qjob.QJob` objects:

.. code-block:: python

    results = gather(distributed_qjobs)

This is a blocking call, since here the function waits both executions to be done. Simulation times 
and output statistics can be accessed by

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

    # If GPU execution is desired, just add "gpu = True" as another qraise argument
    family_name = qraise(2, "01:00:00", classical_comm=True, co_located=True, family = "qpus_class_comms")
    qpus = get_QPUs(family = family_name, co_located=True)

    # 2. Circuit design with classical communications directives

    circuit_1 = CunqaCircuit(2, 2, id="circuit_1")
    circuit_2 = CunqaCircuit(1, 1, id="circuit_2")

    circuit_1.h(0)
    circuit_1.measure(0,0)

    circuit_1.send(0, recving_circuit = "circuit_2")
    circuit_1.measure(1,1)

    circuit_2.recv(0, sending_circuit = "circuit_1")

    with circuit_2.cif(0) as subcircuit:
        subcircuit.x(0)

    circuit_2.measure(0,0)

    # 3. Execution

    distributed_qjobs = run([circuit_1, circuit_2], qpus, shots=1000)
    results = gather(distributed_qjobs)
    counts_list = [result.counts for result in results]

    for counts, qpu in zip(counts_list, qpus):

    print(f"Counts from vQPU {qpu.id}: {counts}")

    # 4. Release classical resources
    qdrop(family_name)