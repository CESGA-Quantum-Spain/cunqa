Further examples
================

Here some examples will be displayed and explained in order to understand CUNQA 
functionalities and purposes. Each subsection follows the structure outlined below:

1. Code examples that explore the underlying concepts.
2. A functional example that can be executed by users to validate them.

As CUNQA supports the main three DQC schemes, this section will cover
all of them by the following:

- :doc:`no_communications` (also called embarrasingly parallel): distributing quantum tasks without 
  inter-QPU communications.
- :doc:`classical_communications`: send and receive classical bits between vQPUs.
- :doc:`quantum_communications`: quantum communications through **teledata** and **telegate** protocols.

All together, these topics provide a solid foundation for understanding the design principles and
capabilities of CUNQA in DQC scenarios.

.. toctree::
   :maxdepth: 1
   :hidden:
   
      No-communications <no_communications.rst>
      Classical-communications <classical_communications.rst>
      Quantum-communications <quantum_communications.rst>
      Raw JSON examples <json_examples/json_examples.rst>