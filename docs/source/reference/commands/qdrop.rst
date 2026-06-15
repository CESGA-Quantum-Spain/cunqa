qdrop
=====

Release the resource of one or more :term:`vQPUs <vQPU>`.

The ``qdrop`` command is used to terminate (drop) :term:`virtual QPUs <virtual QPU>` previously deployed with
``qraise``. Targets can be specified either by their Slurm job IDs, by a :term:`family` name, or by
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
    Slurm IDs of the :term:`QPUs <QPU>` to be dropped.
    Multiple IDs can be given at a time.

``--family <string>``
    :term:`Family` name of the :term:`QPUs <QPU>` to be dropped.
    Multiple :term:`family` names can be given at a time.

``--all``
    Drop all ``qraise`` jobs.

``--rm, --remove_logs``
    Also delete the ``qraise_XXXXXX`` log files of the dropped jobs from the current directory.


Basic usage
-----------

Command that relinquish all deployed :term:`vQPUs <vQPU>`:

.. code-block:: bash

   qdrop --all


Notes
-----

- Exactly one target selector must be used: either ``IDS...``, or ``--family``, or ``--all``.
  Combining ``IDS...`` with ``--family`` is not allowed and results in an error.
- Dropping :term:`QPUs <QPU>` will terminate the associated jobs and free the allocated resources.
