import os
import sys
import numpy as np
import sympy
from sympy import Symbol, Number
import symengine.lib.symengine_wrapper as SymEng

# SymPy can use SymEngine as a backend
sympy.use_symengine = True


class ParameterError(Exception):
    """ Class for signaling errors with the Parameter class."""
    pass

class Parameter(Symbol):
    """ """
    def __init__(name, **assumptions):
        super().__init__(name, **assumptions)

    

        