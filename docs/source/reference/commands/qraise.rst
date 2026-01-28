qraise
======

Raise one or more virtual Quantum Processing Units (QPUs).

The ``qraise`` command is used to deploy a set of virtual QPUs with a given configuration,
including computational resources, backend selection, and communication capabilities.
The raised QPUs can subsequently be used to execute quantum circuits within the CUNQA
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
    Optional.

``--mem-per-qpu <int>``
    Amount of memory (in GB) assigned to each QPU.
    Optional.

``-N, --n_nodes <int>``
    Number of compute nodes used to deploy the QPUs.
    Default: ``1``

``--node_list <string>``
    List of nodes where the QPUs will be deployed.
    This option can be specified multiple times.
    Optional.

``--qpus_per_node <int>``
    Number of QPUs deployed on each node.
    Optional.

Backend and simulation options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``-b, --backend <string>``
    Path to the backend configuration file.
    Optional.

``--noise-properties <string>``
    Path to the noise properties JSON file.
    This option is only supported when using the Aer simulator.
    Optional.

``-sim, --simulator <string>``
    Simulator responsible for running the simulations.
    Default: ``Aer``

FakeQmio-specific options
~~~~~~~~~~~~~~~~~~~~~~~~~

``-fq, --fakeqmio [<string>]``
    Raise a FakeQmio backend using a calibration file.
    If no value is provided, ``last_calibrations`` is used.
    Optional.

``--no-thermal-relaxation``
    Deactivate thermal relaxation effects in the FakeQmio backend.
    Default: ``false``

``--no-readout-error``
    Deactivate readout errors in the FakeQmio backend.
    Default: ``false``

``--no-gate-error``
    Deactivate gate errors in the FakeQmio backend.
    Default: ``false``

Grouping and communication options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``--family_name <string>``
    Name used to identify the group of QPUs that were raised together.
    Default: ``default``

``--co-located``
    Enable co-located mode.
    In this mode, the user can connect to any deployed QPU.
    TODO: clarify the exact behavior and constraints of co-located mode.

``--classical_comm``
    Enable classical communications between QPUs.

``--quantum_comm``
    Enable quantum communications between QPUs.

Infrastructure options
~~~~~~~~~~~~~~~~~~~~~~

``--infrastructure <string>``
    Path to an infrastructure description defining a set of QPUs.
    Optional.
    TODO: document the expected format and semantics of the infrastructure file.

Notes
-----

- Some options are backend- or simulator-specific and may not be supported in all execution
  environments.
- Invalid or incompatible combinations of options may result in an error at deployment time.

TODO
----

- Provide concrete usage examples.
- Clarify interaction rules between ``node_list``, ``n_nodes``, and ``qpus_per_node``.
- Document lifecycle management of raised QPUs (shutdown, expiration, reuse).
