Adding a simulator
==================

CUNQA is designed so that new :term:`simulators <simulator>` can be plugged in without touching the rest of the
platform. Each :term:`simulator` is wrapped in an **adapter** that implements a common C++ interface, and is
then registered with a factory and exposed to the ``qraise`` command. This page describes the steps
involved.

.. note::

   This is a developer-oriented guide. It assumes familiarity with the CUNQA build
   (:doc:`../installation/getting_started`) and with C++/CMake.

Overview
--------

The relevant pieces live under ``src/sim/`` and ``src/utils/``:

- ``src/sim/simulator.hpp`` --- the abstract ``Simulator`` interface every adapter derives
  from.
- ``src/sim/simulators/<NAME>/`` --- one directory per :term:`simulator` adapter (``AER``, ``Munich``,
  ``Qulacs``, ...).
- ``src/sim/simulators/simulator_factory.cpp`` --- maps the :term:`simulator` name string to the adapter.
- ``src/utils/constants.hpp.in`` --- declares which :term:`simulators <simulator>` are accepted by ``qraise`` for each
  communication scheme.

Steps
-----

1. **Create the adapter.**
   Add a new directory ``src/sim/simulators/<NAME>/`` with a class (e.g.
   ``<name>_simulator_adapter.hpp/.cpp``) deriving from ``cunqa::sim::Simulator``. Implement the pure
   virtual methods of the interface:

   - ``get_name()`` --- the :term:`simulator`'s CUNQA name.
   - ``get_basis_gates()`` --- the native gate set.
   - ``initialize()`` / ``clear()`` --- per-execution setup and teardown.
   - ``create_circuit(...)`` --- builds the :term:`simulator`'s circuit object from the instruction JSON.
   - ``native_execute(...)`` --- runs the circuit and returns the result JSON.
   - ``set_noise_model(...)`` --- apply a noise model (may be a no-op if noise is unsupported).

   Then override the ``apply_gate(...)`` overloads for every gate the :term:`backend` supports. Any overload
   you do **not** override falls back to the base-class implementation, which raises a clear
   "Gate ... not supported by <name> :term:`simulator`" error — so you only implement what the :term:`backend` can
   actually do.

2. **Register the adapter in the factory.**
   In ``src/sim/simulators/simulator_factory.cpp``, include the new header and add a branch to
   ``make_simulator()`` returning your adapter for the chosen name string.

3. **Declare it as supported.**
   In ``src/utils/constants.hpp.in``, add the name to the relevant arrays:

   - ``SUPPORTED_SIMPLE_SIMULATORS`` (no-communications scheme),
   - ``SUPPORTED_CC_SIMULATORS`` (:term:`classical communications`),
   - ``SUPPORTED_QC_SIMULATORS`` (:term:`quantum communications`),
   - ``SUPPORTED_NOISY_SIMULATORS`` if it supports noise models.

   Only add the name to the schemes the adapter actually supports — ``qraise`` uses these arrays to
   accept or reject a deployment, and the C++ ``apply_gate`` fallbacks guard the rest.

4. **Wire up the build.**
   Add the new directory to ``src/sim/simulators/CMakeLists.txt`` (an ``add_subdirectory(<NAME>)``
   entry and the adapter library in the ``simulator_factory`` link list), and provide a
   ``CMakeLists.txt`` inside the new directory that builds the adapter library and links the
   :term:`simulator`'s own dependencies.

5. **Document it.**
   Add a row to the table in :doc:`../reference/simulators` with the ``--simulator`` value, the
   library, and the supported features.

Once built and installed, the new :term:`simulator` can be selected like any other::

   qraise -n 2 -t 01:00:00 --simulator <NAME> --co-located
