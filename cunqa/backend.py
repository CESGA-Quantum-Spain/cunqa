"""
    Cointains the class :py:class:`~cunqa.backend.Backend` which serves as a description of the characteristics of the virtual QPUs.
"""

from typing import  TypedDict

class BackendData(TypedDict):
    """
        Class to gather the characteristics of a Backend.
    """
    basis_gates: "list[str]" #: Native gates that the Backend accepts. If other are used, they must be translated into the native gates.
    coupling_map: "list[list[int]]" #: Defines the physical connectivity of the qubits, in which pairs two-qubit gates can be performed.
    custom_instructions: str #: Any custom instructions that the Backend has defined.
    description: str #: Description of the Backend itself.
    gates: "list[str]" #: Specific gates supported.
    n_qubits: int #: Number of qubits that form the Backend, which determines the maximal number of qubits supported for a quantum circuit.
    name: str #: Name assigned to the Backend.
    noise_path: str #: Path to the noise model json file gathering the noise instructions needed for the simulator.
    simulator: str #: Name of the simulatior that simulates the circuits accordingly to the Backend.
    version: str #: Version of the Backend.


class Backend():
    """
        Class to define backend information of a QPU server.
    """
    def __init__(self, backend_dict: BackendData):
        for key, value in backend_dict.items():
            setattr(self, key, value)

    #TODO: make @property?; add more methods as is_ideal, incorporate noisemodel object ot leave for transpilation only?
    def info(self) -> None:
        """
        Prints a dictionary with the backend configuration.
        
        """
        print(f"""--- Backend configuration ---""")
        for attribute_name, attribute_value in self.__dict__.items():
            print(f"{attribute_name}: {attribute_value}")