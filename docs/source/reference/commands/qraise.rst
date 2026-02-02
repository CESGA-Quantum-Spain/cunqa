qraise
======

Raise one or more vQPUs with a well defined configuration.

The ``qraise`` command is used to deploy a set of virtual QPUs with a given configuration,
including computational resources, backend selection, and communication capabilities.
The raised vQPUs can subsequently be used to execute quantum circuits within the CUNQA
framework.

Synopsis
--------

.. code-block:: bash

   qraise [OPTIONS]

Options
-------

General deployment options
~~~~~~~~~~~~~~~~~~~~~~~~~~

``-n, --num_qpus <int>``
    Number of QPUs to be raised.
    Default: ``0``

``-t, --time <string>``
    Time during which the QPUs will remain active.
    Default: empty string.

``-c, --cores <int>``
    Number of CPU cores assigned to each QPU.
    Default: ``2``

``-p, --partition <string>``
    Partition requested for the QPUs.

``--mem-per-qpu <int>``
    Amount of memory (in GB) assigned to each QPU.

``-N, --n_nodes <int>``
    Number of compute nodes used to deploy the QPUs.
    Default: ``1``

``--node_list <string>``
    List of nodes where the QPUs will be deployed.
    This option can be specified multiple times.

``--qpus_per_node <int>``
    Number of QPUs deployed on each node.

Backend and simulation options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``-b, --backend <string>``
    Path to the backend configuration file.

``-sim, --simulator <string>``
    Simulator responsible for running the simulations.
    Default: ``Aer``

Grouping and communication options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``--family_name <string>``
    Name used to identify the group of QPUs that were raised together.
    Default: ``default``

``--co-located``
    Enable co-located mode.
    In this mode, the user can connect to any deployed QPU. If set, the vQPU can be accesed from 
    any node. If not set, the user can only access it from the node it is deployed on.

``--classical_comm``
    Enable classical communications between QPUs.

``--quantum_comm``
    Enable quantum communications between QPUs.

Infrastructure options
~~~~~~~~~~~~~~~~~~~~~~

``--infrastructure <string>``
    Path to an infrastructure description defining a set of QPUs.
    Optional.

Notes
-----

- Some options are backend- or simulator-specific and may not be supported in all execution
  environments.
- Invalid or incompatible combinations of options may result in an error at deployment time.
