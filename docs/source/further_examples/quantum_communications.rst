*****************************
Quantum-communications scheme
*****************************

As with the classical-communications examples, the quantum-communications directives are already
explained in the documentation of :py:class:`~cunqa.circuit.core.CunqaCircuit`, so here we will
directly look at examples of their usage.

To begin with, we have already mentioned that, for now, there are two protocols implemented for
quantum communications: **telegate** and **teledata**. Let us see what each program looks like for
each protocol.

.. tab:: Teledata

    In this case, the operation is very similar to the example presented for classical
    communications, where :py:meth:`~cunqa.circuit.core.CunqaCircuit.send` and
    :py:meth:`~cunqa.circuit.core.CunqaCircuit.recv` are used. The difference here is that the
    object being sent is not a classical bit, but a qubit.

    It is worth noting that this protocol consumes one more qubit than *telegate*, since a qubit
    is required on the destination QPU to receive the state of the sent qubit.

    .. code-block:: python

        import os, sys
        # In order to import cunqa, we append to the search path the cunqa installation path.
        # In CESGA, we install by default on the $HOME path as $HOME/bin is in the PATH variable
        sys.path.append(os.getenv("HOME"))

        from cunqa.qpu import get_QPUs, qraise, qdrop, run
        from cunqa.circuit import CunqaCircuit
        from cunqa.qjob import gather

        family = qraise(2, "01:00:00", quantum_comm=True, co_located=True)

        c1 = CunqaCircuit(2, id="circuit1")
        c1.h(0)
        c1.cx(0,1)
        c1.qsend(1, "circuit2") # this qubit, which is sent, is put to |0>
        c1.measure_all()

        c2 = CunqaCircuit(1, id="circuit2")
        c2.qrecv(0, "circuit1")
        c2.measure_all()

        qjobs = run([c1, c2], qpus, shots=1024)
        results = gather(qjobs)

        for q in results:
            print("Result: ", q.counts)

        qdrop(family)

.. tab:: Telegate

    This protocol is the quantum analogue to the classical condition procedure given by
    :py:meth:`~cunqa.circuit.core.CunqaCircuit.cif`. Both use a Python ``ContextManager`` to
    include the gates subject to the operation: in the case of
    :py:meth:`~cunqa.circuit.core.CunqaCircuit.cif`, these are classically controlled gates, while
    in the case of this protocol—whose gate is :py:meth:`~cunqa.circuit.core.CunqaCircuit.expose`—
    they are gates controlled by a qubit from the remote QPU.

    .. code-block:: python

        import os, sys
        # In order to import cunqa, we append to the search path the cunqa installation path.
        # In CESGA, we install by default on the $HOME path as $HOME/bin is in the PATH variable
        sys.path.append(os.getenv("HOME"))

        from cunqa.qpu import get_QPUs, qraise, qdrop, run
        from cunqa.circuit import CunqaCircuit
        from cunqa.qjob import gather

        family = qraise(2, "01:00:00", quantum_comm=True, co_located=True)
        qpus  = get_QPUs(on_node=False, family=family)

        c1 = CunqaCircuit(1, id="circuit1")
        c1.h(0)

        c2 = CunqaCircuit(1, id="circuit2")

        with c1.expose(0, c2) as rcontrol:
            c2.cx(rcontrol,0)

        c1.measure_all()
        c2.measure_all()

        qjobs = run([c1, c2], qpus, shots=1024)
        results = gather(qjobs)

        for result in results:
            print(result)

        qdrop(family)

With these two protocols, we can now build quantum-distributed programs with greater relevance and
complexity. We have prepared the following examples to present these cases in depth.

**#TODO**