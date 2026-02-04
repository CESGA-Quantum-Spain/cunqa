*****************
CUNQA basic usage
*****************

Circuit design
==============
As already explained when discussing the API of :py:class:`~cunqa.circuit.core.CunqaCircuit`, this 
class does not aim to solve any existing problem in current circuit design APIs, but rather to 
extend them by adding communication directives. Here we will look at several simple examples that 
make this clear.

Circuit without communications
-------------------------------
To visualize what we are saying, it is first key to see that, indeed,
:py:class:`~cunqa.circuit.core.CunqaCircuit` is very similar to the rest of the APIs and does not 
imply a steep learning curve for those who are already used to programming quantum circuits.

In the following code snippet we see how to construct a Bell pair, which allows us to understand 
how gates are applied; effectively confirming that the CUNQA API does not explore any new method.

.. code-block:: python
    :caption: Simple Bell pair constructed.

    c = CunqaCircuit(2, 2)
    c.h(0)
    c.cx(0,1)
    c.measure(0, 0)
    c.measure(1, 1)

The only novelty in this type of operation without communications is found in the case of
:py:meth:`~~cunqa.circuit.core.CunqaCircuit.cif`. This defines a ``ContextManager`` responsible for 
adding conditional gates. In the following example we see that this is done by applying the gates 
to a subcircuit that we name ``cgates``.

.. code-block:: python
    :caption: :py:meth:`~~cunqa.circuit.core.CunqaCircuit.cif` example.

    c = CunqaCircuit(2, 2)
    c.h(0)
    c.measure(0, 0)

    with c.cif(0) as cgates:
        cgates.x(1)

The operations inside the ``cif`` block are executed only if the value stored in classical bit
``0`` is equal to ``1``. Currently, ``cif`` does not support an explicit *else* branch. This design
choice reflects the requirements of the distributed algorithms currently supported by CUNQA.
Future extensions may introduce *else* support if needed.

Circuits with classical communications
--------------------------------------
Entering the realm of communication directives, it is convenient to start with the simplest ones:
classical communication directives. In CUNQA, for the moment there are only two,
:py:meth:`~~cunqa.circuit.core.CunqaCircuit.send` and :py:meth:`~~cunqa.circuit.core.CunqaCircuit.recv`; 
both are two sides of the same coin, since :py:meth:`~~cunqa.circuit.core.CunqaCircuit.send` is 
responsible for sending a bit of classical information and 
:py:meth:`~~cunqa.circuit.core.CunqaCircuit.recv` for receiving it. We can observe how they work in 
the following example.

.. code-block:: python
    :caption: ``circuit1`` sending the bit from classical bit ``0``.

    c1 = CunqaCircuit(1, 1, id="circuit1")
    c1.h(0)
    c1.measure(0,0)
    c1.send(0, recving_circuit = "circuit2")

.. code-block:: python
    :caption: ``circuit2`` receiving the bit on classical bit ``0`` and using it in a ``cif``.
    
    c2 = CunqaCircuit(1, 1, id="circuit2")
    c2.recv(0, sending_circuit = "circuit1")

    with c2.cif(0) as cgates:
        cgates.x(1)

As is well known in, for example, MPI, classical communications include a series of collective
operations that allow the massive communication of information among participants. In CUNQA we have
moved away from this type of use case because we have not found them used in any algorithm. In any
case, if they were needed by some algorithm or user, they would be added.

Circuits with quantum communications
------------------------------------
In this case we find that there are two protocols: teledata and telegate. The first corresponds to
the well-known quantum teleportation, which transfers the state of a qubit from one QPU to the 
qubit of another (which in the case of CUNQA are virtual QPUs). The second is a protocol that 
allows controlling qubits of one QPU with the state of a qubit from another QPU, that is, 
controlling gates remotely (hence the name telegate). Let us see how they work.

Teledata
^^^^^^^^
Teledata directives are analogous to those of :py:meth:`~~cunqa.circuit.core.CunqaCircuit.send` and
:py:meth:`~~cunqa.circuit.core.CunqaCircuit.recv`, but instead of sending and receiving the state 
of a bit, they send and receive the state of a qubit.

.. code-block:: python
    :caption: ``circuit1`` sending the state of qubit ``0``.

    c1 = CunqaCircuit(1, 1, id="circuit1")
    c1.h(0)
    c1.qsend(0, recving_circuit = "circuit2")

.. code-block:: python
    :caption: ``circuit2`` receiving the state of qubit ``0`` of ``circuit1`` into its qubit ``0``.
    
    c2 = CunqaCircuit(1, 1, id="circuit2")
    c2.qrecv(0, sending_circuit = "circuit1")

Telegate
^^^^^^^^
This protocol, as we already anticipated, allows gates to be controlled remotely. To do so, we will
use a structure very similar to that of :py:meth:`~~cunqa.circuit.core.CunqaCircuit.cif`, that is, 
we will have a ``ContextManager`` that, in this case, will return a subcircuit on which to apply 
the gates and, additionally, an index representing the communication qubit that contains the state 
of the remote qubit. We observe this below.

.. code-block:: python
    :caption: ``circuit1`` exposing qubit ``0`` and ``circuit2`` using it as the control of a ``cx`` gate.

    c1 = CunqaCircuit(1, id="circuit1")
    c2 = CunqaCircuit(1, id="circuit2")

    c1.h(0)
    with c1.expose(0, c2) as cgates, rcontrol:
        cgates.cx(rcontrol,0)

Circuit transformations
-----------------------
The circuit transformation functions are explained in the API documentation. Here we will see how
to use them with a series of simple examples. In the #TODO notebook, more complex cases of all these
transformations along with their execution can be observed.

Union
^^^^^
Union involves creating a larger circuit by joining the qubits and their operations from the
circuits subject to this transformation. In the following example we observe the union of two 
simple circuits.

.. code-block:: python
    :caption: Union of two quantum circuits.

    c1 = CunqaCircuit(2, id = "circuit1")
    c1.h(0)
    c1.cx(0,1)

    c2 = CunqaCircuit(1, id = "circuit2")
    c2.x(0)

    union_circuit = union([c1, c2])

Which corresponds to the following union.

.. image:: /_static/telegate.png
    :alt: Union of circuits diagram
    :width: 60%
    :align: center

Addition
^^^^^^^^
Addition, unlike union, results in a circuit with the size of the largest circuit. What this
operation does is concatenate the operations of the circuits that are part of the operation.

.. code-block:: python
    :caption: Addition of two quantum circuits.

    c1 = CunqaCircuit(1, id = "circuit1") # adding ancilla
    c1.h(0)

    c2 = CunqaCircuit(2, id = "circuit2")
    c2.cx(0,1)

    add_circuit = add([c1, c2])

Which corresponds to the following addition.

.. image:: /_static/telegate.png
    :alt: Union of circuits diagram
    :width: 60%
    :align: center

Horizontal split
^^^^^^^^^^^^^^^^
Horizontal split, as the inverse of union, involves dividing circuits into smaller ones, with the
operations associated with each qubit ending up in the subcircuit that contains it. Let us take the
``union_circuit`` resulting from the union example. The original circuits are obtained simply by
operating as shown below.

.. code-block:: python
    :caption: Horizontal split of the ``union_circuit`` circuit.

    # As a list
    [c1, c2] = hsplit(union, [2,1])

    # As int, in this case is equivalent
    [c1, c2] = hsplit(union, 2)

We see that one can specify the number of qubits required in each subcircuit—first
:py:func:`hsplit`—or the number of circuits—second. In this case they are the same, but this is not
usually common, since the first option offers greater modularity than the second, which always
divides the number of qubits evenly among the circuits.

Circuit execution
=================
We will now see how to execute the circuits that we have just built. We will go to the simplest
possible example, ignoring, for the moment, the communication paradigm used.

First, we must raise the vQPUs. For this we have two options: the :doc:`../reference/commands/qraise`
bash command and the :py:func:`~cunqa.qpu.qraise` Python function.

.. tab:: Bash command

    .. code-block:: bash

        qraise -n 4 -t 01:00:00 --co-located

.. tab:: Python function

    .. code-block:: python

        family = qraise(4, "01:00:00", co_located = True)

These two options have an equivalent result, the main difference being that when using the Python
function the family name is already available to obtain the QPUs (in case filtering is needed).

.. code-block:: python

    qpus  = get_QPUs(co_located = True, family = family)

After this, we are ready to execute the circuits. To do so we will use the function
:py:func:`~cunqa.qpu.run`. There are two cases: one circuit only or several circuits.

.. tab:: One circuit

    .. code-block:: python

        qjob = run(c, qpu, shots = 1024)

    And now, thanks to the fact that the :py:class:`~cunqa.qjob.QJob` class has the necessary methods, 
    we can obtain the result. It must be taken into account that obtaining the result is a blocking 
    call, that is, the program waits until the result is returned.

    .. code-block:: python

        result = qjob.result # blocking call

        result.counts # obtaining the counts
        result.time_taken # obtaining the execution time

.. tab:: Several circuits

    .. code-block:: python

        qjobs = run([c1, c2], [qpu1, qpu2], shots = 1024)

    And now, thanks to the fact that the :py:mod:`~cunqa.qjob` module has the function 
    :py:class:`~cunqa.qjob.gather`, we can obtain the results. It must be taken into account that 
    obtaining the result is a blocking call, that is, the program waits until the result is 
    returned.

    .. code-block:: python

        results = gather(qjobs) # blocking call

        for result in results:
            result.counts # obtaining the counts
            result.time_taken # obtaining the execution time

Finally, a good practice is to perform a relinquishing of the resources in order to make a 
responsible use of the HPC infraestructures CUNQA is working on. For this we have two options, same 
as with the deployment of the vQPUs: bash command (:doc:`../reference/commands/qdrop`) or Python 
function (:py:func:`~cunqa.qpu.qdrop`).

.. tab:: Bash command

    .. code-block:: bash

        qdrop --fam family
        qdrop --all # If removing all is desired

.. tab:: Python function

    .. code-block:: python

        qdrop(family)
        qdrop() # If removing all is desired

Thus, the workflow of a program that uses CUNQA has been exposed. All the examples presented in 
this documentation will follow a similar workflow, when not exactly the same.