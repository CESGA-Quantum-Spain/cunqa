#from .circuit_extensions import cunqa_dunder_methods
from .partitioning import hsplit
from .ir import to_ir
from .core import CunqaCircuit

# TODO: fix circuilar import lo allow "from cunqa.circuit import convert"
from cunqa.circuit.converters import convert