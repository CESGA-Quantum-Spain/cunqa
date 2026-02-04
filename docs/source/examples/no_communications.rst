*****************
No communications
*****************

The no-communications scheme is the simplest one and, at the same time, the one that yields the
largest improvement in terms of execution time. This is because it entails a classical distribution
of quantum tasks, a kind of distribution that falls within the scope of classical scheduling and is
well known for accelerating computing processes. However, we will not go deeper into this topic;
for more information, we refer the reader to the CUNQA paper [vazquez2025]_.

To get started, let us pick up where the :doc:`basic_usage` section left off by looking at the
concepts presented in a complete program.

.. code-block:: python

    import os, sys
    sys.path.append(os.getenv("HOME"))

    from cunqa.qpu import get_QPUs, qraise, qdrop, run
    from cunqa.circuit import CunqaCircuit

    family = qraise(1, "01:00:00",  co_located = True)
    [qpu] = get_QPUs(co_located = True, family = family)

    c = CunqaCircuit(2, 2)
    c.h(0)
    c.measure(0, 0)

    with c.cif(0) as cgates:
        cgates.x(1)

    c.measure(0,0)
    c.measure(1,1)

    qjob = run(c, qpu, shots = 1024)
    counts = qjob.result.counts

    print("Counts: ", counts)

    qdrop(family)

This program includes all the core concepts of a CUNQA program. To see how varying the different
parameters affects CUNQA execution, we provide the following examples, which are more complex than
the previous one and go into more depth.

.. nbgallery::
   notebooks/Multiple_circuits_execution.ipynb

The circuit-design part can be made more complex and, without a doubt, preprocessing,
postprocessing, optimization layers, and so on can be added. In particular, for the optimization
layer we developed the :py:mod:`~cunqa.mappers` submodule and the
:py:meth:`~cunqa.qjob.QJob.upgrade_parameters` function, which belongs to the
:py:class:`~cunqa.qjob.QJob` class. The usage of these two features can be seen in the following
examples.

.. nbgallery::
   notebooks/Optimizers_I_upgrading_parameters.ipynb
   notebooks/Optimizers_II_mapping.ipynb