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
   * - ``$STORE/.cunqa/qpus.json``
     - Registry of currently deployed :term:`vQPUs <vQPU>`. Read by ``qinfo``, ``qdrop`` and
       :py:func:`~cunqa.qpu.get_QPUs`.
   * - ``$STORE/.cunqa/communications.json``
     - Communication endpoints used between :term:`vQPUs <vQPU>`.

In addition, each ``qraise`` deployment writes a SLURM log file named ``qraise_<jobid>`` in the
directory from which the command was launched. These can be removed together with the job using
``qdrop ... --rm``.

The ``qpus.json`` registry
--------------------------

``qpus.json`` is a JSON object whose **keys are vQPU identifiers** (of the form
``<slurm_job_id>_<index>``) and whose values describe each :term:`vQPU`. The most relevant fields, as
consumed by the tooling, are:

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

Illustrative entry (fields trimmed for brevity):

.. code-block:: json

   "507805_1071427": {
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

If a deployment ends abnormally, ``qpus.json`` may keep entries for :term:`vQPUs <vQPU>` that are no longer running.
Running ``qdrop --all`` cancels any remaining jobs and resets ``qpus.json`` to an empty object
(``{}``). After that, ``qinfo`` will report that there are no deployed :term:`vQPUs <vQPU>` until you ``qraise``
again.
