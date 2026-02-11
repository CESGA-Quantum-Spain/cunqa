*****************
CUNQA basic usage
*****************

Circuit design
==============
As it is explained in :py:class:`~cunqa.circuit.core.CunqaCircuit`, this 
class does not aim to solve any existing problem in current circuit design APIs, but rather to 
extend them by adding communication directives. Here we will look at several simple examples that 
make this clear.

Circuit without communications
-------------------------------

In the following code snippet we see how to build a circuit that prepares a Bell pair. This allows us to understand 
how quantum instructions are applied, illustrating the simplicity of the process.

.. code-block:: python
    :caption: Simple Bell pair constructed.

    c = CunqaCircuit(2, 2)
    c.h(0)
    c.cx(0,1)
    c.measure(0, 0)
    c.measure(1, 1)

For the classically conditioned gates, we implemented the  
:py:meth:`~~cunqa.circuit.core.CunqaCircuit.cif` instruction which is used within a ``with`` statement as a context manager responsible for 
adding the conditioned gates. This is illustrated in the following example:

.. code-block:: python
    :caption: :py:meth:`~~cunqa.circuit.core.CunqaCircuit.cif` example.

    c = CunqaCircuit(2, 2)
    c.h(0)
    c.measure(0, 0)

    with c.cif(0) as cgates:
        cgates.x(1)

In the example above, the operations inside the block are applied only if the value stored in the
``0`` bit of the classical register is equal to ``1``.
Future extensions may introduce *else* support if needed.

Circuits with classical communications
--------------------------------------
Entering the realm of communication directives, it is convenient to start with the simplest ones:
classical communication directives. CUNQA supports send and receive
directives, two sides of the same coin, since they are responsible for exchanging classical bits between vQPUs. These directives are implemented through the :py:class:`~cunqa.circuit.core.CunqaCircuit` methods :py:meth:`~~cunqa.circuit.core.CunqaCircuit.send` and :py:meth:`~~cunqa.circuit.core.CunqaCircuit.recv` as the code below shows.

.. code-block:: python
    :caption: ``circuit1`` sending the index ``0`` bit.

    c1 = CunqaCircuit(1, 1, id="circuit1")
    c1.h(0)
    c1.measure(0,0)
    c1.send(0, recving_circuit = "circuit2")

.. code-block:: python
    :caption: ``circuit2`` receiving the coming bit into index ``0``.
    
    c2 = CunqaCircuit(1, 1, id="circuit2")
    c2.recv(0, sending_circuit = "circuit1")

    with c2.cif(0) as cgates:
        cgates.x(1)

Although this is similar to the MPI protocol, CUNQA does not support collective communications yet. They can be added if the need arises.

Circuits with quantum communications
------------------------------------
In this scheme, the teledata and telegate communication protocols were implemented.

Teledata
^^^^^^^^
This is the well-known quantum teleportation, which transfers the state of a qubit from one QPU to the 
qubit of another QPU.
Teledata is implemented through the :py:class:`~cunqa.circuit.core.CunqaCircuit` methods :py:meth:`~~cunqa.circuit.core.CunqaCircuit.qsend` and :py:meth:`~~cunqa.circuit.core.CunqaCircuit.qrecv` as shown in the code below.

.. code-block:: python
    :caption: ``circuit1`` sending the state of qubit ``0``.

    c1 = CunqaCircuit(1, 1, id="circuit1")
    c1.h(0)
    c1.qsend(0, recving_circuit = "circuit2")

.. code-block:: python
    :caption: ``circuit2`` receiving the state of qubit ``0`` from ``circuit1`` into qubit ``0``.
    
    c2 = CunqaCircuit(1, 1, id="circuit2")
    c2.qrecv(0, sending_circuit = "circuit1")

Telegate
^^^^^^^^
This protocol allows applying remote two-qubit gates between different QPUs. To implement this as a :py:class:`~cunqa.circuit.core.CunqaCircuit` method, a structure very similar to that of :py:meth:`~~cunqa.circuit.core.CunqaCircuit.cif` is used.
The example below shows how the implementation returns a subcircuit on which to apply 
the gates and the index representing the communication qubit that contains the state.

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
Given two circuits with *n* and *m* qubits, their union returns a circuit with *n + m* qubits, where the operations of the former are applied to the first *n* qubits and those of the latter
are applied to the last *m* qubits. If originally there were distributed instructions between the circuits, they would be replaced by local ones. In the following example we observe the union of two 
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
Addition of two circuits results in a new circuit with the size of the largest circuit and whose instructions are the concatenation of the original circuits instructions.

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
Given a circuit, this function divides the set of qubits into smaller subsets preserving each qubit's operations. Multi-qubit gates that involve different resulting subcircuits will be replaced by their remote counterpart. Therefore, horizontal split is the inverse of the union, so, let us take the
``union_circuit`` resulting from the union example to illustrate this operation:

.. code-block:: python
    :caption: Horizontal split of the ``union_circuit`` circuit.

    # As a list
    [c1, c2] = hsplit(union_circuit, [2,1])

    # As int, in this case is equivalent
    [c1, c2] = hsplit(union_circuit, 2)

In the :py:class:`~cunqa.circuit.core.CunqaCircuit` method implementation, one can specify the number of qubits required in each subcircuit as a list or the number of circuits. The list form allows the resulting circuits to have different amount of qubits.

Circuit execution
==================
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

    And now, we can obtain the result as a property of the :py:class:`~cunqa.qjob.QJob` class.
    Note that retrieving the result is a blocking call, that is, the program waits until the result 
    is returned.

    .. code-block:: python

        result = qjob.result # blocking call

        result.counts # obtaining the counts
        result.time_taken # obtaining the execution time

.. tab:: Several circuits

    .. code-block:: python

        qjobs = run([c1, c2], [qpu1, qpu2], shots = 1024)

    And now, thanks to the fact that the :py:mod:`~cunqa.qjob` module has the function 
    :py:class:`~cunqa.qjob.gather`, we can obtain the results. Note that retrieving the result is 
    a blocking call, that is, the program waits until the result is returned.

    .. code-block:: python

        results = gather(qjobs) # blocking call

        for result in results:
            result.counts # obtaining the counts
            result.time_taken # obtaining the execution time

Finally, it is good practice is to relinquish the resources used in order to make a 
responsible use of the HPC infraestructures that CUNQA runs on. For this we have two options, same 
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