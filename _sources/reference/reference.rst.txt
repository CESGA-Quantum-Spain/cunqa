Reference
=========
This section will explain deeper the main components of CUNQA. As CUNQA manages the resources of a quantum infrastructure through a set of bash commands and provides an platform-user interaction through a Python API, this section is divided into two subsections: **resource management** and **Python API**.

.. _subs-resource-management:

Resource management
--------------------
Three commands cover the quantum infrastructures management: ``qraise``, ``qdrop``, and ``qinfo``.

- :doc:`qraise <commands/qraise>`. Responsible for deploying the :term:`vQPUs <vQPU>` to build 
  :term:`DQC` infrastructures. 
- :doc:`qdrop <commands/qdrop>`. Responsible for releasing the resources of :term:`vQPUs <vQPU>` when they are no 
  longer needed.
- :doc:`qinfo <commands/qinfo>`. Built to obtain information about the available 
  :term:`vQPUs <vQPU>`.

.. toctree::
    :maxdepth: 1
    :hidden:

        qraise <commands/qraise>
        qdrop <commands/qdrop>
        qinfo <commands/qinfo>


.. _subs-simulators:

Simulators
----------
The :term:`simulator` that each :term:`vQPU` uses to run quantum tasks is chosen at deployment time. The
:doc:`simulators` page lists the available simulators, the ``--simulator`` value to select each one,
and which communication schemes, noise and GPU execution they support.

.. toctree::
    :maxdepth: 1
    :hidden:

        Simulators <simulators>


.. _subs-configuration:

Configuration and files
-----------------------
CUNQA stores the state of the deployed :term:`vQPUs <vQPU>` on disk. The :doc:`configuration` page describes the
``$STORE/.cunqa`` directory, the ``qpus.json`` registry read by ``qinfo``/``qdrop``/``get_QPUs``,
and how to recover from stale state.

.. toctree::
    :maxdepth: 1
    :hidden:

        Configuration and files <configuration>


.. _subs-python-api:


Python API
----------
The Python API handles two basic things: the interaction user-:term:`vQPU` by sending and receiving quantum tasks and 
the actual design of quantum tasks.

- The module :py:mod:`cunqa.qpu` allows submitting quantum tasks and retrieving their result to one or several :term:`vQPUs <vQPU>` by leveraging the tools provided by the :py:mod:`cunqa.qjob` and :py:mod:`cunqa.result` modules.

- The design of circuits is handled by the module :py:mod:`~cunqa.circuit`. This module contains a class called :py:class:`~cunqa.circuit.core.CunqaCircuit` which contains the necessary directives to model a quantum task with and without communications. It also contains the submodule :py:mod:`~cunqa.circuit.transformations`, a series of special directives to perform cuts and unions of different circuits.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Module
     - Description
   * - :py:mod:`cunqa.qpu`
     - Contains the :py:class:`~cunqa.qpu.QPU` class and the main functions to interact with :term:`vQPUs <vQPU>`.
   * - :py:mod:`cunqa.qjob`
     - Contains the class that defines and manages quantum jobs.
   * - :py:mod:`cunqa.result`
     - Contains the :py:class:`~cunqa.result.Result`, which holds the output of the executions.
   * - :py:mod:`cunqa.tools.mappers`
     - Contains map-like callables to distribute circuits among :term:`vQPUs <vQPU>`.
   * - :py:mod:`cunqa.tools.probabilities`
     - Helpers to extract probability distributions (or frequency estimates) from results.
   * - :py:mod:`cunqa.circuit`
     - Quantum circuit abstraction for the :py:mod:`cunqa` API.
   * - :py:mod:`cunqa.qc_protocols`
     - :term:`Teledata` and :term:`telegate` quantum-communication protocols.

.. toctree::
    :hidden:
    :maxdepth: 1

    api/cunqa.qpu
    api/cunqa.qjob
    api/cunqa.result
    api/cunqa.mappers
    api/cunqa.probabilities
    api/cunqa.circuit
    api/cunqa.qc_protocols

In addition to the module API reference, three usage-oriented pages complement it:

- :doc:`api/circuit_instructions` --- a categorized overview of the
  :py:class:`~cunqa.circuit.core.CunqaCircuit` instruction set.
- :doc:`api/run_configuration` --- an explanation of the configuration parameters accepted 
  by :py:func:`~cunqa.qpu.run` and :py:meth:`~cunqa.qpu.QPU.execute`.
- :doc:`api/upgrade_parameters` --- a detailed example of the use of gate parameters and its updating.

.. toctree::
    :hidden:
    :maxdepth: 1

    api/circuit_instructions
    api/run_configuration
    api/upgrade_parameters
