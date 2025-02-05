
class Backend():
    """
        Class to define backend information of a QPU server.
    """

    def __init__(self, backend_dict):
        for k,v in backend_dict.items():
            setattr(self, k, v)

    def info(self):
        print(f"""--- Backend configuration ---""")
        for k, v in self.__dict__.items():
            print(f"{k}: {v}")

    def set(self, key, value):
        setattr(self, key, value)

