Parameters and `upgrade_parameters`
=======================================
Parameters are placeholders for the values of parametric gates like `rx`, `rzz` or `u3`. They are 
designed to facilitate the execution of the same circuit multiple times with different values on its
parametric gates.

Parameters are given as a string when adding a gate. The strings determine the label for each parameter
or a expression built out of parameters:

.. code-block:: python

    circuit.rx(param="x",            qubit=0)
    circuit.rz(param="x + y + z",    qubit=0) # Parameter expression with 3 variables. the "x" variable has the same value as in the previous gate
    circuit.ry(param="cos(2*pi*z))", qubit=0) # Common functions can be used, and pi is interpreted as 3.1415..., the ratio of a circle's circumference to its diameter.

Internally, parameters and parameter expressions are handled with the symbolic 
calculus library `sympy <docs.sympy.org/latest/index.html>`_ so valid strings are restricted by the 
`sympify function <https://docs.sympy.org/latest/modules/core.html#module-sympy.core.sympify>`_ that we use
to convert to `sympy` objects. The functions recognized include trigonometric, hyperbolic, exponetial and logarithmic ones.
Check a more complete list at the `sympy documentation <https://docs.sympy.org/latest/modules/functions/elementary.html#trigonometric-functions>`_ .

Values for parameters are giving as an argument of the run function, which can be a dict of a list.
The dict would contain as keys strings with the labels of the variables present across all parameters
with their corresponding int or float value associated, whereas the list would contain the values of
the parameters in order. Dict format is preferrable when complex expressions and repeated parameters
appear, while the list is fast in cases where there are no repeated parameters and each parameter 
contains a single variable.

.. code-block:: python
    
    # Parameters are given values when running
    run(circuit, qpu, param_values={"x": np.pi, "y": 0, "z": 4.5}, shots= 1024)
    run(circuit, qpu, param_values=[np.pi, 0, 4.5], shots= 1024)

For evaluating the same circuit that has been run but with new parameters, use :py:meth:`~cunqa.qjob.QJob.upgrade_parameters`
on its associated :py:meth:`~cunqa.qjob.QJob` object, where the parameters can be given again as a 
list or a dict. Note that variables that are not given a new value keep the previous one.

.. code-block:: python
    
    # For another execution with new parameters, use QJob.upgrade_parameters()
    qjob.upgrade_parameters({"x": 1004, "y": np.pi/4, "z": 4})
    qjob.upgrade_parameters([     1004,      np.pi/4,      4]) # Same  with list
    qjob.upgrade_parameters({"x": 1004}) # Upgrade just the value for "x"

Check the following complete example:

.. literalinclude:: ../../../examples/python/no_comm/03-upgrade_parameters.py
    :language: python