**************************
No-communications scheme
**************************

Ideal execution
===============
Let's showcase here more advanced examples of the no-communication model that showcase a more complex
use of CUNQA than the one displayed in the :doc:`../overview/embarrassingly_parallel` section.

.. nbgallery::
   notebooks/Multiple_circuits_execution.ipynb

For optimization algorithms, the :py:mod:`~cunqa.tools.mappers` submodule and the
:py:meth:`~cunqa.qjob.QJob.upgrade_parameters` method of the the :py:class:`~cunqa.qjob.QJob` were 
developed. The usage of these two features can be seen in the following examples.

.. nbgallery::
   notebooks/Optimizers_I_upgrading_parameters.ipynb
   notebooks/Optimizers_II_mapping.ipynb


The following example shows how to obtain different statistics from :py:class:`~cunqa.qjob.QJob` results:

.. nbgallery::
   notebooks/Result_statistics.ipynb

Finally, we present an example of the local `iterative QPE <https://arxiv.org/abs/quant-ph/0610214>`_, so that the results obtained can be compared with the ones in :doc:`classical_communications` and :doc:`quantum_communications`.

.. literalinclude:: ../../../examples/no_comm/05-iterative_QPE.py
      :language: python

Noisy execution
===============
Running on noisy :term:`vQPUs <vQPU>` requires two changes with respect to an ideal execution: the
:term:`vQPUs <vQPU>` must be deployed with a :term:`backend` configuration describing the noise model
to emulate, and the circuit must be transpiled to the :term:`vQPU`'s backend before being run.

To deploy the noisy :term:`vQPUs <vQPU>`, as it was explained in
:doc:`../overview/overview`, the :doc:`../reference/commands/qraise` Bash command or its
Python function counterpart :py:func:`~cunqa.qpu.qraise` have to be employed with the ``--backend`` flag,
in the first case, and with the ``backend`` argument, in the second; both being the path to a
:term:`backend` configuration JSON file. This :term:`backend` file points, through its
``noise_model.noise_properties_path`` field, to a noise properties JSON file. The format of both files
is shown in :doc:`../further_examples/json_examples/backend_json_example` and
:doc:`../further_examples/json_examples/noise_properties_example`.

.. tab:: Bash command

    .. code-block:: bash

        qraise -n 4 -t 01:00:00 --co-located --backend="complete/path/to/backend.json"

.. tab:: Python function

    .. code-block:: python

        family = qraise(4, "01:00:00", co_located=True, backend="complete/path/to/backend.json")

Once the noisy :term:`vQPUs <vQPU>` are deployed, the circuit has to be transpiled to the
:term:`vQPU`'s backend with the :py:func:`~cunqa.qiskit_deps.transpiler.transpiler` function before
running it. This step is required so that the circuit is expressed in terms of the basis gates and the
connectivity supported by the noisy backend. The following example shows the complete workflow:

.. literalinclude:: ../../../examples/no_comm/04-transpilation_and_noisy_execution.py
      :language: python
