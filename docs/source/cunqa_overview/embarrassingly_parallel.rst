Embarrassingly Parallel
========================

.. figure:: /_static/NoCommScheme.png
    :alt: Embarrasingly parallel scheme
    :width: 60%
    :align: center

    Embarrasingly parallel scheme

As it was introduced, this scheme consists in the **classical offloading of quantum tasks** by mapping 
circuits to the available QPUs. For this, there is no need of inter-QPU communications of any type.

This distribution is convenient for **exploiting resources** and for **accelerating execution times** by reducing
workload for each quantum node.

**How to deploy**
-----------------
Within CUNQA, an insfrastructure of vQPUs which do not incorporate communications among them is launched as

.. code-block:: bash

    qraise -n <num qpus> -t <max time> [OTHER]

The number of vQPUs and their maximum deployment time are the only mandatory flags, for other additional options
checkout :doc:`../reference/commands/qraise`. For a friendlier use, the Python API also incorporates the
:py:func:`~cunqa.qpu.qraise` function.

A useful feature is the :py:attr:`--family` flag. since it allows us to tag the infrastructure. If none is entered, family name
will be the *SLURM job id* of the process in which the vQPUs live, shown right after they are lauched in the terminal or returned
as a ``str`` by the Python function.

**Circuits design**
----------------------
For quantum tasks ran in these scheme the limitation is that communications directives are
not allowed since their are not available for the vQPUs.

For instanciating :py:class:`~cunqa.circuit.core.CunqaCircuit` class we must provide the number of qubits
and, if single-qubit measurements are intended to be done, number of classical bits

.. code-block:: python

    cunqacircuit = CunqaCirucit(num_qubits = 2, num_clbits = 2)

Supported instructions are further explained at :py:class:`~cunqa.circuit.core.CunqaCircuit`.


**Execution**
--------------
Once we obtain the :py:class:`~cunqa.qpu.QPU` objects through :py:func:`~cunqa.qpu.get_QPUs`,
for classically distributing circuits, function :py:func:`~cunqa.qpu.run` is used. By providing
the list of circuits and the list of :py:class:`~cunqa.qpu.QPU` objects we allow their mapping to the corresponding
vQPUs:

.. code-block:: python 

    qpus_list = get_QPUs()

    qjobs = run(circuits_list, qpus_list, shots = 100)

The output is a list of :py:class:`~cunqa.qjob.QJob` objects, each one associated to each quantum task. One can also
input a single circuit instead of a list and it will be mapped to all vQPUs provided. Executions results can be obtained
all together by the :py:func:`~cunqa.qjob.gather` function

.. code-block:: python

    results = gather(qjobs)

This call for the results is a blocking call, since both simulations runnng in parallel need to be done for the
:py:class:`~cunqa.result.Result` objects to be returned. To access information such as time of simulation or output
statistics there are class attributes:

.. code-block:: python

    times_list  = [result.time_taken for result in results]
    counts_list = [result.counts for result in results]


**Basic example**
------------------
Next we show an example that contains all the steps for a embarrasingly parallel distribution. Further examples and use cases
are listed in :doc:`../further_examples/further_examples`.


.. code-block:: python

    import os, sys
    # In order to import cunqa, we append to the search path the cunqa installation path.
    # In CESGA, we install by default on the $HOME path as $HOME/bin is in the PATH variable
    sys.path.append(os.getenv("HOME"))

    from cunqa.qpu import get_QPUs, qraise, qdrop, run
    from cunqa.circuit import CunqaCircuit
    from cunqa.qjob import gather

    # 1. QPU deployment

    family_name = qraise(2, "01:00:00", family_name = "qpu_no_comms")
    qpus = get_QPUs(family = family_name)


    # 2. Circuit design

    cunqacircuit = CunqaCircuit(num_qubits = 2, num_clbits = 2)

    cunqacircuit.h(0)
    cunqacircuit.cx(0,1)

    cunqacircuit.measure_all()

    # 3. Execution

    qjobs = run(cunqacircuit, qpus, shots = 100)
    results = gather(qjobs)
    counts_list = [result.counts for result in results]

    for counts, qpu in zip(counts_list, qpus):

        print(f"Counts from vQPU {qpu.id}: {counts}")

    # 4. Release classical resources
    qdrop(family_name)








