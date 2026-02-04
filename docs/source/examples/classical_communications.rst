************************
Classical communications
************************

The classical-communications directives are already explained in the documentation of
:py:class:`~cunqa.circuit.core.CunqaCircuit`, so here we will look at an example of how to use them.
Since we have already seen the workflow of a CUNQA program, we will present the full program and
comment on the code fragments whenever necessary.

.. code-block:: python

    import os, sys
    # In order to import cunqa, we append to the search path the cunqa installation path.
    # In CESGA, we install by default on the $HOME path as $HOME/bin is in the PATH variable
    sys.path.append(os.getenv("HOME"))

    from cunqa.qpu import get_QPUs, qraise, qdrop, run
    from cunqa.circuit import CunqaCircuit
    from cunqa.qjob import gather

    family = qraise(2, "01:00:00", classical_comm=True, co_located = True)
    qpus = get_QPUs(co_located=True, family = family)

    # Construct the circuits
    c1 = CunqaCircuit(10, 2, id="First")
    c1.h(0)
    c1.measure(0,0)
    c1.send(0, recving_circuit = "Second")
    c1.measure(1,1)

    c2 = CunqaCircuit(2, 2, id="Second")
    c2.recv(0, sending_circuit = "First")

    with c2.cif(0) as cgates:
        cgates.x(1)
    c2.measure(0,0)
    c2.measure(1,1)

    # Run and show the circuits
    circs = [c1, c2]
    distr_jobs = run(circs, qpus, shots=1000)
    result_list = gather(distr_jobs)

In this example we see a typical use case for classical communications: a classical bit is sent
from one QPU to another, where it is used as a control element (i.e., as the bit of a
:py:meth:`~cunqa.circuit.core.CunqaCircuit.cif`).

This :py:meth:`~cunqa.circuit.core.CunqaCircuit.send` and
:py:meth:`~cunqa.circuit.core.CunqaCircuit.recv` syntax for sending and receiving, respectively,
is well known in classical computing, since it is used in virtually all programs that include
communication directives: socket libraries, MPI, etc.

Examples can be made as complex as desired, just as the algorithms that use classical
communications can be made more complex. Below we provide several more complex examples that use
classical communications.

**#TODO**