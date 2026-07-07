Glossary
========

.. glossary::

   vQPU
   virtual QPU
       A classical process running on an HPC environment with an allocated set of classical
       resources that simulates the behaviour of a real QPU. It is composed of a **server** (manages
       user–vQPU communication) and a **simulator** (performs the actual execution). See
       :ref:`sec_virtual_qpu`.

   QPU
       In this documentation, the :py:class:`~cunqa.qpu.QPU` Python object that represents a deployed
       vQPU and is used to submit quantum tasks to it. Also refers to real quantum hardware (e.g.
       CESGA's QMIO), which can be *deployed* alongside vQPUs.

   family
       A name that identifies a group of vQPUs deployed together by a single ``qraise`` call. It is
       set with the ``--family`` flag (Bash) or the ``family`` argument (Python) and is later used
       to select (:py:func:`~cunqa.qpu.get_QPUs`) or release (``qdrop``) that group. If not provided,
       it defaults to the SLURM job id of the deployment.

   co-located mode
       Deployment mode in which a vQPU can be accessed from any node, not only the one it was
       deployed on. Enabled with ``--co-located`` (Bash) / ``co_located=True`` (Python). The
       alternative is *hpc mode*.

   hpc mode
       Default deployment mode in which a vQPU can only be accessed from the node on which it was
       deployed. Contrast with *co-located mode*.

   DQC
   Distributed Quantum Computing
       Computing paradigm in which a quantum computation is distributed across several QPUs. CUNQA
       emulates DQC architectures and supports three :ref:`schemes <sec_dqc_schemes>`:
       embarrassingly parallel, classical communications and quantum communications.

   embarrassingly parallel
       DQC scheme with no inter-QPU communication: independent quantum tasks are mapped to the
       available vQPUs. Also referred to as the *no-communications* (NC) scheme.

   classical communications
       DQC scheme (CC) in which vQPUs exchange **classical bits** (measurement outcomes) during
       execution to classically condition operations on remote qubits.

   quantum communications
       DQC scheme (QC) in which vQPUs share quantum entanglement via the **teledata** and
       **telegate** protocols.

   data qubit
       A computation qubit of a circuit, used to carry out the quantum algorithm.

   communication qubit
   comm qubit
       A qubit reserved for inter-vQPU quantum communication (entanglement generation in teledata
       and telegate). Declared with the tuple form
       ``CunqaCircuit((num_data_qubits, num_comm_qubits), num_clbits)``.

   teledata
       Quantum-communication protocol that **teleports** the state of a data qubit from one circuit
       to another. Exposed through :py:func:`~cunqa.qc_protocols.qsend` /
       :py:func:`~cunqa.qc_protocols.qrecv`.

   telegate
       Quantum-communication protocol that **shares a control qubit** across vQPUs so that a remote
       controlled gate can be applied locally. Exposed through
       :py:func:`~cunqa.qc_protocols.cat_entangler` / :py:func:`~cunqa.qc_protocols.cat_disentangler`.

   backend
       The configuration describing the device a vQPU emulates (basis gates, coupling map, noise
       model, simulator, ...). Provided as a JSON file through ``--backend`` / ``backend`` and
       represented in Python by :py:class:`~cunqa.qpu.Backend`.

   simulator
       The software library that performs the quantum simulation inside a vQPU (e.g. Aer, Munich,
       Qulacs). Selected with ``--simulator``; see :doc:`reference/simulators`.
