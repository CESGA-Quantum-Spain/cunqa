import os
import sys
import numpy as np
import sympy
from typing import Union
from cunqa.logger import logger
import symengine.lib.symengine_wrapper as se

# SymPy can use SymEngine as a backend
sympy.use_symengine = True

class Variable(sympy.Symbol):
    """
    Object that signals to a parametric gate that its value can vary. Variables can be summed, multiplied by 
    other variables or numbers, exponentiated, divided, etc to create parametric expressions. To apply functions
    to variables please use those of the `sympy` module, e.g. `sin_a = sympy.sin(Variable('a'))`. 

    .. note::
        Some available `sympy` functions are 
        "sin", "cos", "tan", "exp", "log", "log10", "sqrt", "abs", "arcsin", "arccos", "arctan", "sinh", 
        "cosh", "tanh", "arcsinh", "arccosh", "arctanh", "deg2rad", "rad2deg", "floor", "round", "gamma",
        "factorial".
    """
    def __new__(cls, name, **assumptions):
        # Primary instance creation
        # Ensures unique symbol instances
        return sympy.Symbol.__new__(cls, name, **assumptions)

    # Dunder methods already implemented on the parent class
    # In particular, as sympy defines __hash__ and __eq__, so 
    # sympy objects and therefore cunqa.Variable objects can be used as keys of a dict

def variables(names, **args):
    r"""
    Wrapper of sympy.symbols. Transform strings into instances of :class:`Variable` class.

    :func:`variables` function returns a sequence of variables with names taken
    from ``names`` argument, which can be a comma or whitespace delimited
    string, or a sequence of strings::

        >>> from sympy import variables, Function

        >>> x, y, z = variables('x,y,z')
        >>> a, b, c = variables('a b c')

    The type of output is dependent on the properties of input arguments::

        >>> variables('x')
        x
        >>> variables('x,')
        (x,)
        >>> variables('x,y')
        (x, y)
        >>> variables(('a', 'b', 'c'))
        (a, b, c)
        >>> variables(['a', 'b', 'c'])
        [a, b, c]
        >>> variables({'a', 'b', 'c'})
        {a, b, c}

    If an iterable container is needed for a single parameter, set the ``seq``
    argument to ``True`` or terminate the parameter name with a comma::

        >>> variables('x', seq=True)
        (x,)

    To reduce typing, range syntax is supported to create indexed variables.
    Ranges are indicated by a colon and the type of range is determined by
    the character to the right of the colon. If the character is a digit
    then all contiguous digits to the left are taken as the nonnegative
    starting value (or 0 if there is no digit left of the colon) and all
    contiguous digits to the right are taken as 1 greater than the ending
    value::

        >>> variables('x:10')
        (x0, x1, x2, x3, x4, x5, x6, x7, x8, x9)

        >>> variables('x5:10')
        (x5, x6, x7, x8, x9)
        >>> variables('x5(:2)')
        (x50, x51)

        >>> variables('x5:10,y:5')
        (x5, x6, x7, x8, x9, y0, y1, y2, y3, y4)

        >>> variables(('x5:10', 'y:5'))
        ((x5, x6, x7, x8, x9), (y0, y1, y2, y3, y4))

    If the character to the right of the colon is a letter, then the single
    letter to the left (or 'a' if there is none) is taken as the start
    and all characters in the lexicographic range *through* the letter to
    the right are used as the range::

        >>> variables('x:z')
        (x, y, z)
        >>> variables('x:c')  # null range
        ()
        >>> variables('x(:c)')
        (xa, xb, xc)

        >>> variables(':c')
        (a, b, c)

        >>> variables('a:d, x:z')
        (a, b, c, d, x, y, z)

        >>> variables(('a:d', 'x:z'))
        ((a, b, c, d), (x, y, z))

    Multiple ranges are supported; contiguous numerical ranges should be
    separated by parentheses to disambiguate the ending number of one
    range from the starting number of the next::

        >>> variables('x:2(1:3)')
        (x01, x02, x11, x12)
        >>> variables(':3:2')  # parsing is from left to right
        (00, 01, 10, 11, 20, 21)

    Only one pair of parentheses surrounding ranges are removed, so to
    include parentheses around ranges, double them. And to include spaces,
    commas, or colons, escape them with a backslash::

        >>> variables('x((a:b))')
        (x(a), x(b))
        >>> variables(r'x(:1\,:2)')  # or r'x((:1)\,(:2))'
        (x(0,0), x(0,1))

    All newly created variables have assumptions set according to ``args``::

        >>> a = variables('a', integer=True)
        >>> a.is_integer
        True

        >>> x, y, z = variables('x,y,z', real=True)
        >>> x.is_real and y.is_real and z.is_real
        True
    """
    return sympy.symbols(names=names, cls=Variable, **args)   