from cunqa.circuit.circuit import CunqaCircuit
from cunqa.circuit.parameter import Variable, variables
from cunqa.circuit.circuit_extensions import cunqa_dunder_methods

CunqaCircuit = cunqa_dunder_methods(CunqaCircuit)

# TODO: fix circuilar import lo allow "from cunqa.circuit import convert"
from cunqa.circuit.converters import convert
