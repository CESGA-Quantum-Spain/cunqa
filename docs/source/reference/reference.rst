CUNQA reference
===============
This section aims to explain in more depth the main components of CUNQA based on the following dichotomy: the **management** of the emulated infrastructure and the **interaction** with it. CUNQA manages the resources of a quantum infrastructure through a set of bash commands and provides an platform-user interaction through a Python API.

Resource management
--------------------
Three commands cover the quantum infrastructures management: ``qraise``, ``qdrop``, and ``qinfo``.

- :doc:`qraise <commands/qraise>`. Responsible for deploying the vQPUs to build 
  DQC infrastructures. 
- :doc:`qdrop <commands/qdrop>`. Responsible for releasing the resources of vQPUs when they are no 
  longer needed.
- :doc:`qinfo <commands/qinfo>`. Built to obtain information about the available 
  vQPUs.

.. toctree::
    :maxdepth: 1
    :hidden:

        qraise <commands/qraise>
        qdrop <commands/qdrop>
        qinfo <commands/qinfo>

Python API
----------
The Python API handles two basic things: the interaction user-vQPU by sending and receiving quantum tasks and 
the actual design of quantum tasks.

- The module :py:mod:`cunqa.qpu` allows submitting quantum tasks and retrieving their result to one or several vQPUs by leveraging the tools provided by the :py:mod:`cunqa.qjob` and :py:mod:`cunqa.result` modules.

- The design of circuits is handled by the module :py:mod:`~cunqa.circuit`. This module contains a class called :py:class:`~cunqa.circuit.core.CunqaCircuit` which contains the necessary directives to model a quantum task with and without communications. It also contains the submodule :py:mod:`~cunqa.circuit.transformations`, a series of special directives to perform cuts and unions of different circuits.

+--------------------------+---------------------------------------------------------------------+
| Module                   | Description                                                         |
+--------------------------+---------------------------------------------------------------------+
|  :py:mod:`cunqa.qpu`     |  Contains the :py:class:`~cunqa.qpu.QPU` class and the main         |
|                          |  functions interact with vQPUs.                                     | 
+--------------------------+---------------------------------------------------------------------+
|  :py:mod:`cunqa.qjob`    | Contains the class that defines and manages quantum jobs.           |
+--------------------------+---------------------------------------------------------------------+
|  :py:mod:`cunqa.result`  |  Contains the :py:class:`~cunqa.result.Result`, which contains the  |
|                          |  output of the executions.                                          |
+--------------------------+---------------------------------------------------------------------+
| :py:mod:`cunqa.mappers`  | Contains map-like callables to distribute circuits among vQPUs.     |
+--------------------------+---------------------------------------------------------------------+
| :py:mod:`cunqa.circuit`  | Quantum circuit abstraction for the :py:mod:`cunqa` API.            |
+--------------------------+---------------------------------------------------------------------+

.. toctree::
    :hidden:
    :maxdepth: 1

    api/cunqa.qpu
    api/cunqa.qjob
    api/cunqa.result
    api/cunqa.mappers
    api/cunqa.circuit