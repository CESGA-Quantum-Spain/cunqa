qinfo
=====

Get information about deployed vQPUs.

The ``qinfo`` command is used to display information about currently deployed virtual QPUs.
It can show the QPUs deployed on a specific node, or on the current node.

Synopsis
--------

.. code-block:: bash

   qinfo [node] [OPTIONS]

Options
-------

Node selection options
~~~~~~~~~~~~~~~~~~~~~~

``node``
    Info about the QPUs on the selected node.

``-m, --mynode``
    Info about the QPUs on the current node.


Basic usage
-----------

Command that bring information about the vQPUs on a specific node:

.. code-block:: bash

   qinfo c7-13

Notes
-----

- If ``node`` is provided, information will be shown for that node.
- If ``--mynode`` is set, information will be shown for the node where the command is executed.
