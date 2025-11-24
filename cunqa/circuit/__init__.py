from .circuit_extensions import cunqa_dunder_methods
from .cut_impl import hsplit

from .circuit import CunqaCircuit
CunqaCircuit = cunqa_dunder_methods(CunqaCircuit)

# TODO: fix circuilar import lo allow "from cunqa.circuit import convert"
from cunqa.circuit.converters import convert