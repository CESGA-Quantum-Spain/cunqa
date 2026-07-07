Simulators
==========

A :term:`vQPU` executes quantum tasks through a **simulator**. The :term:`simulator` is selected at deployment
time with the ``-sim``/``--simulator`` flag of :doc:`commands/qraise` (Bash) or the ``simulator``
argument of :py:func:`~cunqa.qpu.qraise` (Python). If none is given, ``Aer`` is used by default.

.. important::

   The value passed to ``--simulator`` is the **CUNQA name** in the first column of the table
   below, **not** the name of the underlying library. For example, MQT-DDSIM is selected with
   ``--simulator Munich`` and CunqaSimulator with ``--simulator Cunqa``.

Supported simulators
--------------------

The following table summarizes the :term:`simulators <simulator>` currently accepted by ``qraise`` and the features
each one supports. *NC*, *CC* and *QC* stand for the three :ref:`DQC schemes <sec_dqc_schemes>`
(no-communications, classical-communications and quantum-communications).

.. list-table::
   :header-rows: 1
   :widths: 20 26 8 8 8 10 10

   * - ``--simulator``
     - Library
     - NC
     - CC
     - QC
     - Noise
     - GPU
   * - ``Aer``
     - `Qiskit Aer <https://github.com/Qiskit/qiskit-aer>`_
     - ✅
     - ✅
     - ✅
     - ✅
     - ✅
   * - ``Munich``
     - `MQT-DDSIM <https://github.com/munich-quantum-toolkit/ddsim>`_
     - ✅
     - ✅
     - ✅
     - ❌
     - ❌
   * - ``Cunqa``
     - `CunqaSimulator <https://github.com/CESGA-Quantum-Spain/cunqasimulator>`_
     - ✅
     - ✅
     - ✅
     - ❌
     - ❌
   * - ``Qulacs``
     - `Qulacs <https://github.com/qulacs/qulacs>`_
     - ✅
     - ✅
     - ✅
     - ❌
     - ❌
   * - ``Maestro``
     - `Maestro <https://github.com/QoroQuantum/maestro>`_
     - ✅
     - ✅
     - ✅
     - ❌
     - ❌
   * - ``Qsim``
     - `qsim <https://github.com/quantumlib/qsim>`_
     - ✅
     - ✅
     - ✅
     - ❌
     - ❌
   * - ``Quest``
     - `QuEST <https://github.com/QuEST-Kit/QuEST>`_
     - ✅
     - ✅
     - ✅
     - ❌
     - ❌

.. note::

   This matrix reflects what the ``qraise`` command currently *accepts*, derived from the
   ``SUPPORTED_*_SIMULATORS`` constants. Individual :term:`backends <backend>` may still differ in the set of native
   gates and run options they implement; deploying with an unsupported combination raises an error
   at deployment time.

Notes on specific features
--------------------------

- **GPU.** GPU execution is only available with ``Aer`` (and requires building with
  ``-DUSE_GPU=ON``, see :doc:`../installation/getting_started`). Requesting ``--gpu`` with any
  other :term:`simulator` raises an error.
- **Noise.** Noisy simulations are supported by ``Aer`` and ``Munich``, and only within the
  no-communications scheme. The noise model is provided through a :term:`backend` configuration file
  (``--backend`` / ``backend``); see
  :doc:`../further_examples/json_examples/backend_json_example` and
  :doc:`../further_examples/json_examples/noise_properties_example`.

Selecting a simulator
---------------------

.. tab:: Bash command

   .. code-block:: bash

      qraise -n 2 -t 01:00:00 --simulator Munich --co-located

.. tab:: Python function

   .. code-block:: python

      family = qraise(2, "01:00:00", simulator="Munich", co_located=True)
