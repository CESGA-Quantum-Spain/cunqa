qinfo
=====

Get information about deployed :term:`vQPUs <vQPU>`.

The ``qinfo`` command is used to display information about currently deployed :term:`virtual QPUs <virtual QPU>`.
It can show the :term:`QPUs <QPU>` deployed on a specific node, or on the current node.

Synopsis
--------

.. code-block:: bash

   qinfo [node] [OPTIONS]

Options
-------

Node selection options
~~~~~~~~~~~~~~~~~~~~~~

``node``
    Info about the :term:`QPUs <QPU>` on the selected node.

``--mynode``
    Info about the :term:`QPUs <QPU>` on the current node.


Basic usage
-----------

Command that bring information about the :term:`vQPUs <vQPU>` on a specific node:

.. code-block:: bash

   qinfo c7-13

Notes
-----

- If ``node`` is provided, information will be shown for that node.
- If ``--mynode`` is set, information will be shown for the node where the command is executed.
- If neither ``node`` nor ``--mynode`` is given, the number of :term:`QPUs <QPU>` per :term:`family`
  is shown for every node with deployed :term:`QPUs <QPU>`.
