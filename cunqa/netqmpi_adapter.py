"""Adapter layer to plug CUNQA and NetQASM under a QMPI-like communication API.

This module introduces a lightweight backend abstraction inspired by the
NetQMPI architecture sketch:

- A common communication facade (``QMPIComm``)
- A backend adapter selecting a concrete backend implementation
- Concrete backends for CUNQA and NetQASM

The implementation is intentionally transport-agnostic: users can provide
``send``/``recv`` callables from their runtime (MPI, sockets, RPC, etc.) and
reuse the same QMPI-facing flow.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable, Optional


InitFn = Callable[[], None]
FinalizeFn = Callable[[], None]
SendFn = Callable[[Any, int, int], None]
RecvFn = Callable[[int, int], Any]
BarrierFn = Callable[[], None]


class CommunicationBackend:
    """Interface expected by :class:`QMPIComm`."""

    def init(self) -> None:
        """Initialize backend resources."""

    def finalize(self) -> None:
        """Finalize backend resources."""

    def send(self, payload: Any, dst: int, tag: int = 0) -> None:
        """Send ``payload`` to destination rank ``dst``."""

    def recv(self, src: int, tag: int = 0) -> Any:
        """Receive a payload from source rank ``src``."""

    def barrier(self) -> None:
        """Synchronize all participants."""


@dataclass
class _CallableBackend(CommunicationBackend):
    """Base backend that delegates communication primitives to callables."""

    name: str
    send_fn: SendFn
    recv_fn: RecvFn
    init_fn: Optional[InitFn] = None
    finalize_fn: Optional[FinalizeFn] = None
    barrier_fn: Optional[BarrierFn] = None

    def init(self) -> None:
        if self.init_fn:
            self.init_fn()

    def finalize(self) -> None:
        if self.finalize_fn:
            self.finalize_fn()

    def send(self, payload: Any, dst: int, tag: int = 0) -> None:
        self.send_fn(payload, dst, tag)

    def recv(self, src: int, tag: int = 0) -> Any:
        return self.recv_fn(src, tag)

    def barrier(self) -> None:
        if self.barrier_fn:
            self.barrier_fn()


class CUNQAComm(_CallableBackend):
    """CUNQA-backed communicator implementation."""

    def __init__(
        self,
        send_fn: SendFn,
        recv_fn: RecvFn,
        init_fn: Optional[InitFn] = None,
        finalize_fn: Optional[FinalizeFn] = None,
        barrier_fn: Optional[BarrierFn] = None,
    ) -> None:
        super().__init__(
            name="cunqa",
            send_fn=send_fn,
            recv_fn=recv_fn,
            init_fn=init_fn,
            finalize_fn=finalize_fn,
            barrier_fn=barrier_fn,
        )


class NetQASMComm(_CallableBackend):
    """NetQASM-backed communicator implementation."""

    def __init__(
        self,
        send_fn: SendFn,
        recv_fn: RecvFn,
        init_fn: Optional[InitFn] = None,
        finalize_fn: Optional[FinalizeFn] = None,
        barrier_fn: Optional[BarrierFn] = None,
    ) -> None:
        super().__init__(
            name="netqasm",
            send_fn=send_fn,
            recv_fn=recv_fn,
            init_fn=init_fn,
            finalize_fn=finalize_fn,
            barrier_fn=barrier_fn,
        )


class BackendAdapter:
    """Factory/adapter that returns the backend requested by NetQMPI code."""

    def __init__(self, backend: CommunicationBackend):
        self._backend = backend

    @property
    def backend(self) -> CommunicationBackend:
        return self._backend

    @classmethod
    def from_name(
        cls,
        backend_name: str,
        *,
        send_fn: SendFn,
        recv_fn: RecvFn,
        init_fn: Optional[InitFn] = None,
        finalize_fn: Optional[FinalizeFn] = None,
        barrier_fn: Optional[BarrierFn] = None,
    ) -> "BackendAdapter":
        normalized = backend_name.strip().lower()
        if normalized == "cunqa":
            backend = CUNQAComm(
                send_fn=send_fn,
                recv_fn=recv_fn,
                init_fn=init_fn,
                finalize_fn=finalize_fn,
                barrier_fn=barrier_fn,
            )
        elif normalized == "netqasm":
            backend = NetQASMComm(
                send_fn=send_fn,
                recv_fn=recv_fn,
                init_fn=init_fn,
                finalize_fn=finalize_fn,
                barrier_fn=barrier_fn,
            )
        else:
            raise ValueError(
                f"Unsupported backend '{backend_name}'. Use 'cunqa' or 'netqasm'."
            )
        return cls(backend)


class QMPIComm:
    """QMPI-like facade that talks to a pluggable backend via ``BackendAdapter``."""

    def __init__(self, adapter: BackendAdapter):
        self._adapter = adapter
        self._initialized = False

    def MPI_Init(self) -> None:
        """Compatibility no-op to mirror MPI init call ordering."""

    def QMPI_Init(self) -> None:
        self._adapter.backend.init()
        self._initialized = True

    def send(self, payload: Any, dst: int, tag: int = 0) -> None:
        self._ensure_initialized()
        self._adapter.backend.send(payload, dst, tag)

    def recv(self, src: int, tag: int = 0) -> Any:
        self._ensure_initialized()
        return self._adapter.backend.recv(src, tag)

    def barrier(self) -> None:
        self._ensure_initialized()
        self._adapter.backend.barrier()

    def finalize(self) -> None:
        if self._initialized:
            self._adapter.backend.finalize()
            self._initialized = False

    def _ensure_initialized(self) -> None:
        if not self._initialized:
            raise RuntimeError("QMPI_Init must be called before communication primitives.")
