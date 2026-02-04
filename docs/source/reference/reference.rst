CUNQA reference
===============
There are two aspects of the use of CUNQA: the **management** of the emulated infrastructure and the 
**interaction** with it. To achieve this, the platform consists of two pillars: a set of commands, 
responsible for managing the infrastructure, and a Python API, responsible for the platform-user 
interaction.

Terminal commands
-----------------
Three commands cover the infrastructure management: ``qraise``, ``qdrop``, and ``qinfo``.

- :doc:`qraise <commands/qraise>`. Responsible for deploying the vQPUs (simulated QPUs), which make up 
  the emulated quantum infrastructure. It allows deploying a specified number of vQPUs with the 
  desired characteristics defined through flags (see :doc:`the manual page <commands/qraise>` for more 
  information).
- :doc:`qdrop <commands/qdrop>`. Responsible for releasing the resources of vQPUs when they are no 
  longer needed. Its purpose and use are very similar to SLURM's ``scancel``.
- :doc:`qinfo <commands/qinfo>`. Built with the purpose of obtaining information about the available 
  vQPUs on the platform.

.. toctree::
    :maxdepth: 1
    :hidden:

        qraise <commands/qraise>
        qdrop <commands/qdrop>
        qinfo <commands/qinfo>

Python API
----------
The Python API can be divided into two blocks: sending and receiving quantum tasks, and, 
on the other hand, designing quantum tasks.

For the first block, the module :py:mod:`cunqa.qpu` allows submitting quantum tasks and retrieving 
their result by leveraging the tools provided by the :py:mod:`cunqa.qjob` and 
:py:mod:`cunqa.result` modules. Additionally, the :py:mod:`cunqa.mappers` allows the mapping of 
quantum tasks to a specific group of vQPUs, which is useful for performing optimizations.

For the second block, the design of circuits is handled by the module :py:mod:`~cunqa.circuit`. This module 
contains a class called :py:class:`~cunqa.circuit.core.CunqaCircuit` which contains the necessary 
directives to model a quantum task. It also contains the submodule :py:mod:`~cunqa.circuit.transformations`,
a series of special directives to perform cuts and unions of different circuits.

+--------------------------+---------------------------------------------------------------------+
| Module                   | Description                                                         |
+--------------------------+---------------------------------------------------------------------+
|  :py:mod:`cunqa.qpu`     |  Contains the :py:class:`~cunqa.qpu.QPU` class and the functions to |
|                          |  manage the virtual QPUs (vQPUs).                                   |
+--------------------------+---------------------------------------------------------------------+
|  :py:mod:`cunqa.qjob`    | Contains objects that define and manage quantum emulation jobs.     |
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