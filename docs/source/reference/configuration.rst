Configuration and files
=======================

CUNQA keeps a small amount of state on disk to track deployed :term:`vQPUs <vQPU>` and their
communication setup. This page describes where that state lives and what it contains.

The ``.cunqa`` directory
------------------------

All runtime state is stored under ``$STORE/.cunqa/``. The location is derived at build time from the
``STORE`` environment variable (see :doc:`../installation/getting_started`):

.. list-table::
   :header-rows: 1
   :widths: 34 66

   * - Path
     - Contents
   * - ``$STORE/.cunqa/``
     - Root directory for CUNQA runtime state.
   * - ``$STORE/.cunqa/qpus/<family>/<vqpu_id>.json``
     - Registry of currently deployed :term:`vQPUs <vQPU>`, one file per :term:`vQPU` grouped by
       :term:`family`. Read by ``qinfo``, ``qdrop`` and :py:func:`~cunqa.qpu.get_QPUs`.
   * - ``$STORE/.cunqa/communications/<slurm_job_id>/<id>.json``
     - Communication endpoints used between :term:`vQPUs <vQPU>`, one file per endpoint grouped by
       SLURM job.

In addition, each ``qraise`` deployment writes a SLURM log file named ``qraise_<jobid>`` in the
directory from which the command was launched. These can be removed together with the job using
``qdrop ... --rm``.

The vQPU registry
-----------------

Each :term:`vQPU` registers itself in its own file, ``$STORE/.cunqa/qpus/<family>/<vqpu_id>.json``,
where the identifier is of the form ``<slurm_job_id>_<index>``. Every file is written only once, and
atomically, by the :term:`vQPU` it describes, so no file locks are needed. This keeps the registry
consistent on shared filesystems such as NFS even when :term:`vQPUs <vQPU>` on several nodes start
at the same time. The files are removed when the SLURM job ends, and so is the directory of the
:term:`family` once it is empty.

The content of each file describes the :term:`vQPU`. The most relevant fields, as consumed by the
tooling, are:

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - Field
     - Meaning
   * - ``family``
     - :term:`family` name the :term:`vQPU` belongs to.
   * - ``slurm_job_id``
     - SLURM job that deployed the :term:`vQPU`.
   * - ``backend``
     - :term:`Backend` description: ``name``, ``description``, ``simulator``, ``basis_gates``,
       ``coupling_map``, ``num_qubits``, ...
   * - ``net.nodename``
     - Compute node the :term:`vQPU` runs on.
   * - ``net.mode``
     - Access mode: ``co_located`` or ``hpc``.
   * - ``net.endpoint``
     - Address the Python client connects to.
   * - ``net.device``
     - Device info; ``device.device_name`` is ``QPU`` for real hardware (e.g. QMIO) and otherwise
       identifies the simulated device.

Illustrative content of ``qpus/507805/507805_1071427.json`` (fields trimmed for brevity):

.. code-block:: json

   {
        "backend": {
            "basis_gates": [],
            "coupling_map": [],
            "description": "Backend with quantum communications.",
            "name": "QCBackend",
            "num_qubits": [
                2,
                1
            ],
            "simulator": "Aer",
            "version": "0.0.1"
        },
        "family": "507805",
        "name": "507805_1071427",
        "net": {
            "device": {
                "device_name": "CPU",
                "target_devices": []
            },
            "endpoint": "tcp://10.5.7.3:45653",
            "mode": "co_located",
            "nodename": "c7-3"
        },
        "slurm_job_id": "507805"
    }

Recovering from stale state
---------------------------

If a deployment ends abnormally, the registry may keep files for :term:`vQPUs <vQPU>` that are no
longer running. Running ``qdrop --all`` cancels any remaining jobs and removes the
``$STORE/.cunqa/qpus`` and ``$STORE/.cunqa/communications`` directories. After that, ``qinfo`` will report that there are no deployed :term:`vQPUs <vQPU>` until you ``qraise``
again.
