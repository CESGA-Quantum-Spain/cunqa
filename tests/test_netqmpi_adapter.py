from cunqa.netqmpi_adapter import BackendAdapter, QMPIComm


def test_backend_adapter_cunqa_send_recv_flow():
    sent = []

    def send_fn(payload, dst, tag):
        sent.append((payload, dst, tag))

    def recv_fn(src, tag):
        return {"src": src, "tag": tag, "value": "ok"}

    adapter = BackendAdapter.from_name("cunqa", send_fn=send_fn, recv_fn=recv_fn)
    comm = QMPIComm(adapter)

    comm.MPI_Init()
    comm.QMPI_Init()
    comm.send("hello", dst=2, tag=7)
    msg = comm.recv(src=3, tag=5)

    assert sent == [("hello", 2, 7)]
    assert msg == {"src": 3, "tag": 5, "value": "ok"}


def test_backend_adapter_netqasm_lifecycle_hooks_are_called():
    events = []

    def init_fn():
        events.append("init")

    def finalize_fn():
        events.append("finalize")

    def send_fn(payload, dst, tag):
        events.append(("send", payload, dst, tag))

    def recv_fn(src, tag):
        events.append(("recv", src, tag))
        return "answer"

    adapter = BackendAdapter.from_name(
        "netqasm",
        send_fn=send_fn,
        recv_fn=recv_fn,
        init_fn=init_fn,
        finalize_fn=finalize_fn,
    )
    comm = QMPIComm(adapter)
    comm.QMPI_Init()
    comm.send(payload=123, dst=4, tag=1)
    assert comm.recv(src=4, tag=1) == "answer"
    comm.finalize()

    assert events == [
        "init",
        ("send", 123, 4, 1),
        ("recv", 4, 1),
        "finalize",
    ]


def test_comm_requires_qmpi_init():
    def send_fn(payload, dst, tag):
        return None

    def recv_fn(src, tag):
        return None

    adapter = BackendAdapter.from_name("cunqa", send_fn=send_fn, recv_fn=recv_fn)
    comm = QMPIComm(adapter)

    try:
        comm.send("x", dst=1)
        raised = False
    except RuntimeError:
        raised = True

    assert raised
