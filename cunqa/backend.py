from typing import  TypedDict

class BackendData(TypedDict):
    basis_gates: "list[str]"
    coupling_map: "list[list[int]]"
    custom_instructions: str
    description: str
    gates: "list[str]"
    n_qubits: int
    name: str
    noise_model: str
    simulator: str
    version: str


class Backend():
    """
        Class to define backend information of a QPU server.
    """
    def __init__(self, backend_dict: BackendData):
        for key, value in backend_dict.items():
            setattr(self, key, value)

    def info(self) -> None: 
        """
        Prints a dictionary with the backend configurations
        
        """
        print(f"""--- Backend configuration ---""")
        for attribute_name, attribute_value in self.__dict__.items():
            print(f"{attribute_name}: {attribute_value}")