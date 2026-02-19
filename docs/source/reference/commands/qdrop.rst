qdrop
=====

Release the resource of one or more vQPUs.

The ``qdrop`` command is used to terminate (drop) virtual QPUs previously deployed with
``qraise``. Targets can be specified either by their Slurm job IDs, by a family name, or by
dropping all active ``qraise`` jobs.

Synopsis
--------

.. code-block:: bash

   qdrop [IDS...] [OPTIONS]

Options
-------

Target selection options
~~~~~~~~~~~~~~~~~~~~~~~~

``IDS...``
    Slurm IDs of the QPUs to be dropped.
    Multiple IDs can be given at a time.

``--fam, --family_name <string>``
    Family name of the QPUs to be dropped.

``--all``
    Drop all ``qraise`` jobs.


Basic usage
-----------

Command that relinquish all deployed vQPUs:

.. code-block:: bash

   qdrop --all


Notes
-----

- If multiple target selectors are provided (e.g., ``IDS...`` and ``--family_name``), the command
  will attempt to drop all QPUs matching any of them.
- Dropping QPUs will terminate the associated jobs and free the allocated resources.
