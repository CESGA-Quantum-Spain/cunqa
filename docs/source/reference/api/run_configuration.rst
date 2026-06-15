Run configuration
=================

When a quantum task is submitted with :py:func:`~cunqa.qpu.run` (or
:py:meth:`~cunqa.qpu.QPU.execute`), any extra keyword arguments are forwarded as **run parameters**
that configure the simulation:

.. code-block:: python

   qjobs = run(circuits, qpus, shots=2000, method="statevector", seed=42)

These are assembled into the run configuration carried to the C++ side, whose canonical definition is
the ``RunConfig`` struct (``src/sim/run_config.hpp``). Parameters fall into two groups: the
**common** ones recognized by every :term:`simulator`, and **simulator-specific** ones, which are passed
through untouched to the selected :term:`backend`.

Common parameters
-----------------

.. list-table::
   :header-rows: 1
   :widths: 22 14 16 48

   * - Parameter
     - Type
     - Default
     - Description
   * - ``shots``
     - ``int``
     - ``1024``
     - Number of measurement repetitions.
   * - ``method``
     - ``str``
     - ``"automatic"``
     - Simulation method. Valid values are :term:`simulator`-specific (e.g. ``statevector``,
       ``density_matrix`` for :term:`Aer <simulator>`); ``automatic`` lets the :term:`backend` choose.
   * - ``seed``
     - ``int``
     - *(unset)*
     - Seed for the random number generator. If omitted, a non-deterministic seed is used.
   * - ``avoid_parallelization``
     - ``bool``
     - ``False``
     - Disable internal parallelization of the simulation.

.. note::

   Some entries of ``RunConfig`` (``qpu_id``, ``num_clbits``, ``is_dynamic``, ``sending_to``,
   ``device``) are **set automatically** by CUNQA from the circuit and deployment, and are not meant
   to be supplied as run parameters.

Simulator-specific parameters
-----------------------------

Any keyword argument that is not one of the common parameters above is collected into
``simulator_specifics`` and handed to the chosen :term:`simulator` as-is. The accepted keys therefore depend
on the :term:`backend` selected with ``--simulator``; refer to :doc:`../simulators` and to the upstream
documentation of each :term:`simulator` for the options it understands.

Quantum-communications constraint
---------------------------------

In the :term:`quantum communications` scheme the participating :term:`vQPUs <vQPU>` share a single simulation, so
their run configurations must be **mutually consistent**. CUNQA enforces that all circuits in a QC
execution use the same ``method``, the same ``shots``, the same ``avoid_parallelization`` setting,
and the same device type; otherwise an error is raised. If their ``seed`` values differ, the seed is
left unset for the combined run.
