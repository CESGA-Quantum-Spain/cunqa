Quick start
===========

Once :ref:`installed <sec_installation>`, the basic CUNQA workflow can be summarized as:

1. **Deploy vQPUs:** allocate classical resources for simulation. 
2. **Design quantum task:** create quantum circuits to be run on vQPUs. 
3. **Execution:** send quantum tasks to vQPUs to be simulated.
4. **Relinquish vQPUs:** free classical resources.

To deploy the vQPUs we mainly use the :doc:`../reference/commands/qraise`
bash command. The following example will deploy 4 vQPUs, for 1 hour and accessible from any HPC node:

.. code-block:: bash

    qraise -n 4 -t 01:00:00 --co-located


Once the vQPUs are deployed, we can design and execute quantum tasks:

.. code-block:: python 

    import os, sys

    # Adding path to access CUNQA module
    sys.path.append(os.getenv("</your/cunqa/installation/path>"))

    # Let's get the raised QPUs
    from cunqa.qpu import get_QPUs

    # List of deployed vQPUs
    qpus  = get_QPUs(co_located=True)

    # Let's create a circuit to run in our QPUs
    from cunqa.circuit import CunqaCircuit

    qc = CunqaCircuit(num_qubits = 2)
    qc.h(0)
    qc.cx(0,1)
    qc.measure_all()

    qcs = [qc] * 4

    # Submitting the same circuit to all vQPUs
    from cunqa.qpu import run

    qjobs = run(qcs , qpus, shots = 1000)

    # Gathering results
    from cunqa.qjob import gather

    results = gather(qjobs)

    # Getting the counts
    counts_list = [result.counts for result in results]

    # Printing the counts
    for counts in counts_list:
        print(f"Counts: {counts}" ) # Format: {'00':546, '11':454}


It is a good practice to relinquish resources when the work is done. This is achieved by the :doc:`../reference/commands/qdrop` command:

.. code-block:: bash

    qdrop --all