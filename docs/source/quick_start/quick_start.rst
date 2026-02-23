Quick start
===========

Once :ref:`installed <sec_installation>`, the basic CUNQA workflow can be summarized as:

1. **Deploy vQPUs:** allocate classical resources for simulation. 
2. **Design quantum task:** create quantum circuits to be run on vQPUs. 
3. **Execution:** send quantum tasks to vQPUs to be simulated.
4. **Relinquish vQPUs:** free classical resources.

The following example will deploy 4 vQPUs, for 1 hour and accessible from any HPC node. To do this,
there are two posible worflows: raising the vQPUs with a Python function or raising the vQPUs with 
a bash command and get them in the Python program. 

.. tab:: Bash command

    To deploy the vQPUs we use the :doc:`../reference/commands/qraise` Bash command.

    .. code-block:: bash

        qraise -n 4 -t 01:00:00 --co-located

    Once the vQPUs are deployed, we can design and execute quantum tasks:

    .. code-block:: python 

        import os, sys

        # Adding path to access CUNQA module
        sys.path.append(os.getenv("</your/cunqa/installation/path>")) # os.getenv("HOME") at CESGA by default

        # Gettting the raised QPUs
        from cunqa.qpu import get_QPUs

        qpus  = get_QPUs(co_located=True)

        # Creating a circuit to run in our QPUs
        from cunqa.circuit import CunqaCircuit

        qc = CunqaCircuit(num_qubits = 2)
        qc.h(0)
        qc.cx(0,1)
        qc.measure_all()

        # Submitting the same circuit to all vQPUs
        from cunqa.qpu import run

        qcs = [qc] * 4
        qjobs = run(qcs , qpus, shots = 1000)

        # Gathering results
        from cunqa.qjob import gather

        results = gather(qjobs)

        # Getting and printing the counts
        counts_list = [result.counts for result in results]

        for counts in counts_list:
            print(f"Counts: {counts}" ) # Format: {'00':546, '11':454}
    
    It is a good practice to relinquish resources when the work is done. This is achieved by the :doc:`../reference/commands/qdrop` command:

    .. code-block:: bash

        qdrop --all

.. tab:: Python function
    
    In this case, the deployment and the design and execution of quantum tasks comes altogether.

    .. code-block:: python 

        import os, sys

        # Adding path to access CUNQA module
        sys.path.append(os.getenv("</your/cunqa/installation/path>"))

        # Raising the QPUs
        from cunqa.qpu import qraise

        family = qraise(2, "00:10:00", simulator="Aer", co_located=True)

        # Gettting the raised QPUs
        from cunqa.qpu import get_QPUs

        qpus  = get_QPUs(co_located=True)

        # Creating a circuit to run in our QPUs
        from cunqa.circuit import CunqaCircuit

        qc = CunqaCircuit(num_qubits = 2)
        qc.h(0)
        qc.cx(0,1)
        qc.measure_all()

        # Submitting the same circuit to all vQPUs
        from cunqa.qpu import run

        qcs = [qc] * 4
        qjobs = run(qcs , qpus, shots = 1000)

        # Gathering results
        from cunqa.qjob import gather

        results = gather(qjobs)

        # Getting and printing the counts
        counts_list = [result.counts for result in results]

        for counts in counts_list:
            print(f"Counts: {counts}" ) # Format: {'00':546, '11':454}
        
        # Relinquishing the resources
        from cunqa.qpu import qdrop
        
        qdrop(family)
