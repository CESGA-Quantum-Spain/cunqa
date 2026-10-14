
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


.. _sec_comm_model:

**Noise and time in the communications**
----------------------------------------

By default, entanglement is ideal and instantaneous: a :py:meth:`~cunqa.circuit.core.CunqaCircuit.gen_ent`
directive prepares a perfect Bell (or GHZ) state as soon as every participant reaches it, and the
local operations are noiseless. Two optional pieces make the emulation closer to real hardware:

- The ``timing`` block of the :term:`backend` gives every :term:`vQPU` a clock in
  :term:`simulated time` (see :doc:`../further_examples/json_examples/backend_json_example`). When
  every vQPU of the quantum-communications scheme has one, each shot follows its own timeline and
  the result reports it in :py:attr:`~cunqa.result.Result.simulated_time`.
- The **communication noise model** adds the cost of distributing entanglement, in fidelity and in
  time, on top of that timeline. It is built around the entanglement resource rather than around
  each protocol: an EPR generator delivers imperfect pairs after a random delay, the pair halves
  decohere in the :term:`communication qubits <comm qubit>` until they are consumed, and every qubit
  that waits for them decoheres too. The local operations of the protocols (CNOTs, measurements,
  Pauli corrections) get the gate and readout errors of the noise properties of each vQPU.

The noise model is enabled as soon as the backend of a vQPU has a ``comm_model`` or a
``noise_model`` section, or when a ``comm_model`` run argument is passed to
:py:func:`~cunqa.qpu.run`. It always works in simulated time: the vQPUs without a ``timing`` block
then use its default durations. Without any of them, the behaviour and the performance are the ones
described above.

Simulated time
^^^^^^^^^^^^^^

The clock of each vQPU follows the rules of the ``timing`` block:

- Every qubit keeps the time at which it is free again and every classical bit the time at which its
  value is known. An operation starts as soon as all its qubits are free, and operations on
  different qubits run in parallel. The clock of a vQPU is the latest of those times.
- A classically controlled block (:py:meth:`~cunqa.circuit.core.CunqaCircuit.cif`) waits for its
  bits plus the ``feed_forward_latency``; its qubits stay reserved until then, even when the block is
  skipped.
- A classical message is sent once its bits are known and arrives after the ``classical_latency``
  of the receiver plus a random ``classical_jitter``.

The noise model turns the time that a qubit spends idle into thermal relaxation (T1/T2), applied
when the qubit is next used. Relaxation composes, :math:`R_{t_1} \circ R_{t_2} = R_{t_1 + t_2}`, so
applying it lazily is exact. The schedule always takes the durations of the ``timing`` block: the
gate durations of the noise properties only enter the error of each gate.

Entanglement generation
^^^^^^^^^^^^^^^^^^^^^^^

Each link has an EPR generator. A vQPU requests a state when it reaches its
:py:meth:`~cunqa.circuit.core.CunqaCircuit.gen_ent` directive: *on demand*, once the vQPU has
finished the work issued before it (its data qubits) and the communication qubit is free. The
generation starts when every participant has requested it and takes the longest ``entanglement``
duration of their ``timing`` blocks (a fixed overhead, 0 by default) plus the attempts of the
generator: :math:`N \sim \mathrm{Geom}(p)` attempts of duration ``attempt_time`` (:math:`\tau`,
including the heralding round trip), so the pair is ready at
:math:`t_\mathrm{ready} = t_\mathrm{req} + t_\mathrm{entanglement} + N\tau`. At that time the
communication qubits are re-initialized and loaded with a Bell-diagonal pair (a Werner pair of the
given ``fidelity``, or explicit ``bell_coefficients``), which then decoheres with the T1/T2 of the
communication qubits until it is consumed. The operation consuming it starts at
:math:`\max(\text{local time}, t_\mathrm{ready})`.

Requesting and consuming a pair are separate events, so pairs can be **prefetched**: calling
:py:meth:`~cunqa.circuit.core.CunqaCircuit.gen_ent` earlier in the circuits overlaps the generation with
local work, and the protocols consume that pair when called with ``prefetched=True``. On-demand
requests, the default, give an upper bound on the idle decoherence of the data qubits; prefetching
trades it for decoherence of the pair in memory.

.. code-block:: python

    # Pair requested before the local work...
    circuit_1.gen_ent(comm_qubits_1[0], "circuit_2", "teledata")
    circuit_2.gen_ent(comm_qubits_2[0], "circuit_1", "teledata")

    circuit_1.cx(0, 1)  # ... so that it is generated while this runs

    qsend(circuit_1, data_qubits_1[0], comm_qubits_1[0], [0, 1], "circuit_2", tag="teledata", prefetched=True)
    qrecv(circuit_2, data_qubits_2[0], comm_qubits_2[0], [0, 1], "circuit_1", tag="teledata", prefetched=True)

States shared by more than two vQPUs (the GHZ states of :py:func:`~cunqa.qc_protocols.cat_entangler`)
are assembled from a star of links rooted at the first vQPU (by id): the state is ready when the
slowest link heralds, each leaf carries the Pauli noise of its own link, and the pairs that arrive
earlier decohere while waiting.

The parameters of the generator can also be **tabulated**: a list of samples, each one with the
generation ``delay`` and the Bell coefficients (or ``fidelity``) of the pair it delivers, with
optional ``weight``. This is how statistics precomputed with a network simulator (e.g. Q2NS or
NetSquid) are fed to CUNQA.

Random waits in each shot
^^^^^^^^^^^^^^^^^^^^^^^^^

The generation times (and the classical jitter) are random. ``strategy`` selects how the shots
account for them:

- ``"exact"`` (default): every shot samples its own delays, so each shot is one realization of the
  timeline and the output statistics include the whole latency distribution. ``time_bin`` rounds the
  sampled delays to a grid, as when shots are grouped by realization; keep it much smaller than T2.
- ``"averaged"``: the delays are not sampled. The timeline uses their means, and each random wait
  :math:`T` becomes the averaged relaxation channel :math:`\mathbb{E}[R_T]`, with parameters
  :math:`\mathbb{E}[e^{-T/T_1}]` and :math:`\mathbb{E}[e^{-T/T_2}]` (the Laplace transform of the
  waiting time). It is exact for a single qubit, but it misses the correlations of a wait shared by
  several qubits, since :math:`\mathbb{E}[R_T \otimes R_T] \neq \mathbb{E}[R_T] \otimes \mathbb{E}[R_T]`.

For instance, with a Bell state on two data qubits that both wait for the same pair, generated with
:math:`p = 0.02` and :math:`\tau = 0.2\,\mu s` (so :math:`\mathbb{E}[T] = T_2/2`), the probability of
an even XX parity is :math:`(1 + \mathbb{E}[e^{-2T/T_2}])/2 \approx 0.749` with the exact strategy and
:math:`(1 + \mathbb{E}[e^{-T/T_2}]^2)/2 \approx 0.721` with the averaged one: the averaged channel
underestimates the coherence of correlated states.

Noise modes
^^^^^^^^^^^

``noise_mode`` selects how the channels are applied:

- ``"exact"``: thermal relaxation as its exact channel (a mixture of identity, Z and reset when
  :math:`T_2 \leq T_1`, amplitude damping plus dephasing otherwise). It needs Kraus channels, which
  only ``Aer`` provides (not with its stabilizer methods).
- ``"pauli"``: every channel is replaced by its Pauli twirl (for relaxation,
  :math:`p_x = p_y = (1 - e^{-t/T_1})/4` and :math:`p_z = (1 + e^{-t/T_1} - 2e^{-t/T_2})/4`), so that
  Clifford circuits run on stabilizer simulators (``method="stabilizer"`` in ``Aer``). The
  Bell-diagonal pairs are already Pauli channels.
- ``"auto"`` (default): ``"exact"`` when the simulator supports it, ``"pauli"`` otherwise.

Configuration
^^^^^^^^^^^^^

The model is described by a ``comm_model`` object. In the :term:`backend` file of each vQPU, next to
its ``timing`` block, it sets the model deployed with the vQPUs, as in
``examples/qu_comm/09-communication_noise.json`` (see :doc:`../further_examples/quantum_communications`);
passed as a run argument, it changes it for that run only, so that parameters can be swept without
deploying the vQPUs again. Only the keys given are changed:

.. code-block:: python

    qjobs = run([circuit_1, circuit_2], qpus, shots=4000, comm_model={
        "epr": {"fidelity": 0.95, "success_probability": 0.01, "attempt_time": 1e-6},
        "comm_qubits": {"T1": 1e-3, "T2": 5e-4},
        "links": [{"circuits": ["circuit_1", "circuit_2"], "epr": {"fidelity": 0.9}}]
    })

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Key
     - Meaning
   * - ``epr``
     - Generator of every link. Parametric (default): ``fidelity`` (Werner pair) or
       ``bell_coefficients`` (``[Phi+, Psi+, Psi-, Phi-]``), ``success_probability`` (:math:`p`, default
       1) and ``attempt_time`` (:math:`\tau` in seconds, default 0). Tabulated: ``"source": "tabulated"``
       with ``samples`` (``delay``, ``fidelity`` or ``bell_coefficients``, ``weight``) or the ``path``
       of a JSON file with them.
   * - ``links``
     - Generators of specific links, overriding ``epr``: a list of objects with the two ``circuits``
       they connect (``qpus`` in a backend) and their ``epr``.
   * - ``data_qubits``, ``comm_qubits``
     - ``T1`` and ``T2`` (seconds) of the idle qubits. The noise properties of the vQPU take
       precedence for the data qubits; the communication qubits use ``comm_qubits``, then the noise
       properties, then ``data_qubits``.
   * - ``strategy``, ``time_bin``
     - ``"exact"`` or ``"averaged"`` treatment of the random waits; grid for the sampled delays.
   * - ``noise_mode``
     - ``"auto"``, ``"exact"`` or ``"pauli"``.

Durations and classical latencies are not part of it: they belong to the ``timing`` block of each
vQPU. The parameters of the network (``epr``, ``links``, ``strategy``, ``time_bin`` and
``noise_mode``) are shared by all the vQPUs, so the backends that set them must agree, while
``data_qubits`` and ``comm_qubits`` belong to each vQPU. A wrong model is reported when the vQPUs are
deployed, or as the error of the run.

Results
^^^^^^^

In simulated time, the :py:class:`~cunqa.result.Result` of each circuit reports in
:py:attr:`~cunqa.result.Result.simulated_time` the duration of its shots and the mean idle time of
each qubit. With the noise model, :py:attr:`~cunqa.result.Result.comm_stats` adds the entanglement
requests per shot and their mean generation time.

.. code-block:: python

    results = gather(qjobs)
    print(results[1].simulated_time)   # {'unit': 's', 'mean': 1.2e-05, 'min': ..., 'max': ..., 'idle_mean': [...]}
    print(results[1].comm_stats)       # {'entanglement_requests_per_shot': 1.0, 'mean_generation_time': ...}

Scope of the model
^^^^^^^^^^^^^^^^^^

Each pair of communication qubits runs its own generation: the number of communication qubits of a
vQPU bounds how many pairs it can generate and hold at once, and requests that share a communication
qubit are served in the order of the circuit, which is their simulated-time order. Multi-hop links,
entanglement swapping and distillation, memory cutoffs and contention between many nodes are not
modeled. The excited-state population of the qubits is neglected in the relaxation channels.
