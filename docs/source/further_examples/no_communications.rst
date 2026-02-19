**************************
No-communications scheme
**************************

Ideal execution
===============
Let's showcase here more advanced examples of the no-communication model that showcase a more complex
use of CUNQA than the one displayed in the :doc:`../cunqa_overview/embarrassingly_parallel` section.

.. nbgallery::
   notebooks/Multiple_circuits_execution.ipynb

For optimization algorithms, the :py:mod:`~cunqa.mappers` submodule and the
:py:meth:`~cunqa.qjob.QJob.upgrade_parameters` method of the the :py:class:`~cunqa.qjob.QJob` were 
developed. The usage of these two features can be seen in the following examples.

.. nbgallery::
   notebooks/Optimizers_I_upgrading_parameters.ipynb
   notebooks/Optimizers_II_mapping.ipynb


The following example shows how to obtain different statistics from :py:class:`~cunqa.qjob.QJob` results:

.. nbgallery::
   notebooks/Result_statistics.ipynb

Finally, we present an example of the local `iterative QPE <https://arxiv.org/abs/quant-ph/0610214>`_, so that the results obtained can be compared with the ones in :doc:`classical_communications` and :doc:`quantum_communications`.

.. literalinclude:: ../../../examples/python/no_comm/05-iterative_QPE.py
      :language: python

Noisy execution
===============
The program scheme does not change one bit with the existing of noise in the vQPUs. It just changes 
the way these vQPUs are deployed, because they need to receive the path a JSON specifying the noise 
model properties. 

To do this, as it was explained in :doc:`../cunqa_overview/cunqa_overview`, the 
:doc:`../reference/commands/qraise` Bash command or its Python function counterpart 
:py:func:`~cunqa.qpu.qraise` have to be employed with the flag `noise-properties`, in the first 
case, and with the argument `noise_path`, in the second; with both being the path 
to a noise properties JSON file. The format of this JSON file is shown in 
:doc:`../further_examples/json_examples/noise_properties_example`.

.. tab:: Bash command

    .. code-block:: bash

        qraise -n 4 -t 01:00:00 --co-located --noise-properties="complete/path/to/noise.json"

.. tab:: Python function

    .. code-block:: python

        family = qraise(4, "01:00:00", co_located=True, noise_path="complete/path/to/noise.json")

Moreover, as also described in :doc:`../cunqa_overview/cunqa_overview`, it is possible to deploy a 
vQPU configured with the noise model of `CESGA's QMIO quantum computer <https://www.cesga.es/infraestructuras/cuantica/>`_, 
provided that CUNQA is running within the CESGA infrastructure. This setup can be enabled by using 
the ``fakeqmio`` option in the :doc:`../reference/commands/qraise` Bash command or by passing the 
``fakeqmio`` argument to the :py:func:`~cunqa.qpu.qraise` Python function.

.. tab:: Bash command

    .. code-block:: bash

        qraise -n 4 -t 01:00:00 --co-located --fakeqmio

.. tab:: Python function

    .. code-block:: python

        family = qraise(4, "01:00:00", co_located=True, fakeqmio=True)
