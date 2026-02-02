CUNQA reference
===============
A user to use CUNQA requires two things: the management of the emulated infrastructure and the 
interaction with it. To achieve this, the platform consists of two pillars: a set of commands, 
responsible for managing the infrastructure, and a Python API, responsible for the platform-user 
interaction.

Commands
--------
The commands are simply three: ``qraise``, ``qdrop``, and ``qinfo``. Each of them is responsible for 
a different aspect of managing the infrastructure.

- :doc:`qraise <commands/qraise>`. Responsible for raising the vQPUs (simulated QPUs), which make up 
  the emulated quantum infrastructure. It allows raising a specified number of vQPUs with the 
  characteristics defined through its options (see :doc:`its manual <commands/qraise>` for more 
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
As for the Python API, we can divide it into two blocks: sending and receiving quantum tasks, and, 
on the other hand, designing quantum tasks.

For the first case, the module :py:mod:`cunqa.qpu` allows sending the quantum task and obtaining 
the result correctly by leveraging the tools provided by :py:mod:`cunqa.qjob` and 
:py:mod:`cunqa.result` modules. Additionally, there is the module :py:mod:`cunqa.mappers` to be 
able to map quantum tasks to a specific group of vQPUs in case of performing optimization.

For the second case, the design of circuits, the module :py:mod:`~cunqa.circuit`. This module 
contains a class called :py:class:`~cunqa.circuit.core.CunqaCircuit` which contains the necessary 
directives to model the quantum task in the required way. It also contains a submodule 
:py:mod:`~cunqa.circuit.transformations`, a series of special directives that allow to perform cuts 
and unions of different circuits can be found.

+--------------------------+---------------------------------------------------------------------+
| Module                   | Description                                                         |
+--------------------------+---------------------------------------------------------------------+
|  :py:mod:`cunqa.qpu`     |  Contains the :py:class:`~cunqa.qpu.QPU` class and the functions to |
|                          |  manage the virtual QPUs (vQPUs).                                   |
+--------------------------+---------------------------------------------------------------------+
| :py:mod:`cunqa.qjob`     | Contains objects that define and manage quantum emulation jobs.     |
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