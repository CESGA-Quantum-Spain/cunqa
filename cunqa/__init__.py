"""
CUNQA: a Distributed Quantum Computing emulator for HPC
=======================================================

Documentation is available in the docstrings.

Subpackages
-----------
- circuit       --- Circuit-related utilities

Top-level modules
-----------------
- qjob
- result
- qpu
- mappers

Public API in the main cunqa namespace
--------------------------------------
- get_QPUs
- qraise
- qdrop
"""

from __future__ import annotations
import importlib as _importlib

_submodules = [
    "circuit",
    "qiskit_deps",
    "qjob",
    "result",
    "qpu",
    "mappers"
]

_lazy_symbols = {
    "get_QPUs": ("cunqa.qpu", "get_QPUs"),
    "qraise": ("cunqa.qpu", "qraise"),
    "qdrop": ("cunqa.qpu", "qdrop"),
    "gather": ("cunqa.qjob", "gather")
}

__all__ = _submodules + list(_lazy_symbols.keys()) + ["__version__"]


def __dir__():
    return __all__


def __getattr__(name: str):
    if name in _submodules:
        mod = _importlib.import_module(f"{__name__}.{name}")
        globals()[name] = mod  # cache
        return mod

    if name in _lazy_symbols:
        module_name, attr_name = _lazy_symbols[name]
        try:
            mod = _importlib.import_module(module_name)
        except Exception as e:
            raise ImportError(
                f"Cannot import '{name}' from '{module_name}'. "
                f"This usually means optional dependencies are missing "
                f"or the install is incomplete. Original error: {e}"
            ) from e
        obj = getattr(mod, attr_name)
        globals()[name] = obj  # cache
        return obj

    raise AttributeError(f"Module '{__name__}' has no attribute '{name}'")
