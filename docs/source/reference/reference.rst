CUNQA reference
===============
CUNQA, as an emulator of distributed quantum computing (DQC), exige tanto la gestión de la 
infraestructura emulada como la interacción con la misma. Para ello, dos pilares de la plataforma 
fueron construídos: un conjunto de comandos, que se encarga de gestionar la infraestructura, y una 
API de Python, que es la responsable de la interacción con esta infraestructura.

Commands
--------
Los comandos son sencillamente tres: ``qraise``, ``qdrop`` y ``qinfo``. Cada uno de ellos se encarga de un 
aspecto diferente en la gestión de la infraestructura.

- :doc:`qraise <commands/qraise>`. Este comando se encarga de levantar las vQPUs (QPUs simuladas), 
  que conforman la infraestructura cuántica emulada. Permite levantar un número determinado de vQPUs 
  con las características especificadas a través de sus opciones (ver :doc:`su manual <commands/qraise>` 
  para más info).
- :doc:`qdrop <commands/qdrop>`.
- :doc:`qinfo <commands/qinfo>`. 

.. toctree::
    :maxdepth: 1
    :hidden:

        qraise <commands/qraise>
        qdrop <commands/qdrop>
        qinfo <commands/qinfo>

Python API
----------

.. autosummary::
    :toctree: api
    :template: module.rst

    cunqa.mappers
    cunqa.qjob
    cunqa.qpu
    cunqa.result

cunqa.circuit
~~~~~~~~~~~~~

.. autosummary::
    :toctree: api
    :template: 

    cunqa.circuit.core.CunqaCircuit

.. autosummary::
    :toctree: api
    :template: module.rst

    cunqa.circuit.transformations