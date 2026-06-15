from typing import Union
import copy
import numpy as np
from itertools import accumulate


from cunqa.circuit.core import CunqaCircuit
from cunqa.qc_protocols import cat_entangler, cat_disentangler
from cunqa.utils.constants import REMOTE_GATES
from cunqa.utils.logger import logger

def vsplit():
    """TODO: Vertical split of a quantum circuit."""
    pass # TODO

def hsplit(circuit: CunqaCircuit, qubits_or_sections: Union[list[int], int]) -> list[CunqaCircuit]:
    """
    Horizontal split of a quantum circuit.

    This function splits a circuit into a given number of subcircuits. This number is determined by
    the `qubits_or_sections` argument. If it is a list, then it specifies the number of qubits each
    subcircuit will have; however, if it is an int, it specifies the number of subcircuits to be
    created (each having the same number of qubits, except for one in case the split is not exact,
    which will take the remainder as its number of qubits).

    This operation is the inverse of the :py:func:`union`.

    Args:
        circuit (~cunqa.circuit.core.CunqaCircuit): circuit to be splited.
        qubits_or_sections(list[int], int): if is a list, qubits in which to split, if an int,
                                            number of subcircuits that result of the split.

    """
    num_qubits = circuit.num_qubits[0]

    if isinstance(qubits_or_sections, list):
        if np.sum(qubits_or_sections) != num_qubits:
            raise RuntimeError(f"Error: Incorrect hsplit of the circuit, {qubits_or_sections} does "
                               f"not add up to {num_qubits} qubits")
        Nsections = len(qubits_or_sections)
        initial_qubits = [0] + [int(x) for x in np.cumsum(qubits_or_sections)]

    elif isinstance(qubits_or_sections, int):
        Nsections = int(qubits_or_sections)
        if Nsections <= 0:
            raise ValueError('number sections must be larger than 0.') from None
        Neach_section, extras = divmod(num_qubits, Nsections)
        section_sizes = (extras * [Neach_section + 1] +
                         (Nsections - extras) * [Neach_section])
        initial_qubits = [0] + [int(x) for x in np.cumsum(section_sizes)]

    def get_subcircuits(circuit, initial_qubits, Nsections):
        sub_circuits = []
        circuits_linked = {}
        telegate_count = 0
        measures = {i: [] for i in range(Nsections)}
        clbits = {i: [] for i in range(Nsections)}

        for i in range(Nsections):
            num_qubits_i = initial_qubits[i + 1] - initial_qubits[i]
            sub_circuits.append(CunqaCircuit(num_qubits_i, id=circuit.id + f"_{i}"))

        def find_index(array, value):
            for i, elem in enumerate(array):
                if elem > value:
                    return i - 1

        last_idx = 0

        for inst in circuit.instructions[:]:
            if "qubits" not in inst:
                # Instructions without qubit indices (cif, endcif, send, recv, gen_ent):
                # assign to the last active subcircuit.
                sub_circuits[last_idx].add_instructions([inst])
                continue

            raw_qubits = inst["qubits"]
            qubits = [raw_qubits] if isinstance(raw_qubits, int) else list(raw_qubits)

            i = find_index(initial_qubits, qubits[0])
            last_idx = i
            sub_circuit = sub_circuits[i]

            if inst["name"] == "measure":
                from bisect import bisect_left
                b = int(inst["clbits"])
                pos = bisect_left(clbits[i], b)
                if pos == len(clbits[i]) or clbits[i][pos] != b:
                    clbits[i].insert(pos, b)
                measures[i].append(inst)
                inst["qubits"] = qubits[0] - initial_qubits[i]
                sub_circuit.add_instructions([inst])
            elif len(qubits) == 1:
                inst["qubits"] = qubits[0] - initial_qubits[i]
                sub_circuit.add_instructions([inst])
            elif len(qubits) == 2:
                j = find_index(initial_qubits, qubits[1])
                target_circuit = sub_circuits[j]
                if i != j:
                    linked_subcircuits = frozenset({i, j})
                    _, comm_1 = sub_circuit.get_qubits()
                    _, comm_2 = target_circuit.get_qubits()
                    
                    if not circuits_linked.get(linked_subcircuits):
                        circuits_linked[linked_subcircuits] = True
                        sub_circuit.add_comm_qubits(len(comm_1) + 1)
                        target_circuit.add_comm_qubits(len(comm_2) + 1)
                        
                    _, comm_1 = sub_circuit.get_qubits()
                    _, comm_2 = target_circuit.get_qubits()
                    
                    ctrl_qubit = inst["qubits"][0] - initial_qubits[i]
                    inst["qubits"][0] = comm_2[-1]
                    inst["qubits"][1] -= initial_qubits[j]
                    
                    cat_entangler(
                        [sub_circuit, target_circuit],
                        ctrl_qubit,
                        [comm_1[0], comm_2[0]],
                        [0, 0],
                        tag=f"telegate_{telegate_count}"
                    )
                    telegate_count += 1

                    target_circuit.add_instructions([inst])

                    cat_disentangler(
                        [sub_circuit, target_circuit],
                        ctrl_qubit,
                        [comm_2[0]],
                        [0, 1],
                        [0, 0]
                    )
                else:
                    inst["qubits"] = [qubits[0] - initial_qubits[i], qubits[1] - initial_qubits[i]]
                    sub_circuit.add_instructions([inst])
            else:
                raise ValueError("Three or more qubit gates cannot be partitioned.")

        for i, sub_circuit in enumerate(sub_circuits):
            if clbits[i]:
                sub_circuit.add_cl_register("subcl_0", len(clbits[i]))
                for measure_i in measures[i]:
                    measure_i["clbits"] = clbits[i].index(int(measure_i["clbits"]))

        return sub_circuits

    return get_subcircuits(copy.deepcopy(circuit), initial_qubits, Nsections)

def union(circuits: list[CunqaCircuit]) -> CunqaCircuit:
    """
    Union of circuits (addition of qubits).

    This function joins the qubits of several :py:class:`~cunqa.circuit.core.CunqaCircuit` objects
    into a single circuit. Circuits connected by quantum communication protocols have those
    protocols collapsed into their equivalent local operations:

    - **Telegate** (a :py:func:`~cunqa.qc_protocols.cat_entangler` / remote-controlled gate(s) /
      :py:func:`~cunqa.qc_protocols.cat_disentangler` block). The whole protocol is removed and the
      gate(s) the receiver applied controlled on its comm qubit are reissued as direct gate(s)
      controlled on the sender's data qubit.
    - **Teledata** (a :py:func:`~cunqa.qc_protocols.qsend` / :py:func:`~cunqa.qc_protocols.qrecv`
      block). The whole protocol is removed and replaced by a ``swap`` between the sender's and the
      receiver's data qubits.

    This operation is the inverse of the :py:func:`hsplit`.

    Args:
        circuits (list[~cunqa.circuit.core.CunqaCircuit]): circuits to join.
    """
    if not circuits:
        raise ValueError("Empty list passed to perform union.")
    if len(circuits) == 1:
        logger.warning("Not enough circuits to perform a union, returning the original circuit.")
        return circuits[0]

    circuits = copy.deepcopy(circuits)

    qubit_offsets = [0] + list(accumulate(
        c.num_qubits[0] + c.num_qubits[1] for c in circuits[:-1]
    ))
    clbit_offsets = [0] + list(accumulate(c.num_clbits for c in circuits[:-1]))
    id_to_idx = {c.id: i for i, c in enumerate(circuits)}

    def gq(idx: int, qubit: int) -> int:
        """Map a circuit-local qubit index to its index in the merged circuit."""
        return qubit + qubit_offsets[idx]

    def reindex(instr: dict, idx: int, alias: dict = None) -> dict:
        """
        Copy ``instr`` remapping its qubit/clbit indices into the merged-circuit space.
        ``alias`` optionally maps specific local qubit indices to already-global indices (used to
        rewrite a comm qubit into the sender's data qubit when collapsing a telegate).
        """
        alias = alias or {}

        def map_qubit(q: int) -> int:
            return alias[q] if q in alias else gq(idx, q)

        new_instr = dict(instr)
        if "instructions" in new_instr:
            new_instr["instructions"] = [reindex(sub, idx, alias)
                                         for sub in new_instr["instructions"]]
        if "qubits" in new_instr:
            q = new_instr["qubits"]
            new_instr["qubits"] = (map_qubit(q) if isinstance(q, int)
                                   else [map_qubit(qi) for qi in q])
        if "comm_qubit" in new_instr:
            new_instr["comm_qubit"] = gq(idx, new_instr["comm_qubit"])
        if "clbits" in new_instr:
            c = new_instr["clbits"]
            new_instr["clbits"] = (c + clbit_offsets[idx] if isinstance(c, int)
                                   else [ci + clbit_offsets[idx] for ci in c])
        return new_instr

    # -----------------------------------------------------------------
    # 1. Discover the quantum-communication protocols by their gen_ent tag.
    # -----------------------------------------------------------------
    groups: dict = {}  # tag -> {"genpos": {idx: pos}, "comms": {idx: comm_qubit}}
    for idx, circuit in enumerate(circuits):
        for pos, instr in enumerate(circuit.instructions):
            if (instr.get("name") == "gen_ent"
                    and all(cid in id_to_idx for cid in instr["circuits"])):
                tag = instr["tag"]
                group = groups.setdefault(tag, {"genpos": {}, "comms": {}})
                group["genpos"][idx] = pos
                group["comms"][idx] = instr["comm_qubit"]

    # -----------------------------------------------------------------
    # 2. Parse each protocol, collapse it into its local equivalent and record, for every
    #    participant circuit, the instruction range that the protocol occupies.
    # -----------------------------------------------------------------
    collapsed: dict = {}   # tag -> local gates to emit at the barrier
    blocks: dict = {idx: [] for idx in range(len(circuits))}  # idx -> [(start, end, tag), ...]

    def find_sender(group: dict) -> int:
        """The sender is the participant whose first directive after gen_ent is a ``send``."""
        for idx, genpos in group["genpos"].items():
            instrs = circuits[idx].instructions
            for pos in range(genpos + 1, len(instrs)):
                if instrs[pos]["name"] == "send":
                    return idx
                if instrs[pos]["name"] == "recv":
                    break
        raise ValueError("Could not identify the sender of a quantum communication protocol "
                         "while performing the union.")

    def skip_cif_blocks(instrs: list, pos: int) -> int:
        """
        Skip consecutive ``cif ... endcif`` blocks (the comm-qubit reset corrections emitted by
        the teledata/telegate protocols) starting at ``pos``. Returns the position right after the
        last skipped block.
        """
        while pos < len(instrs) and instrs[pos]["name"] == "cif":
            while pos < len(instrs) and instrs[pos]["name"] != "endcif":
                pos += 1
            pos += 1  # skip the matching endcif
        return pos

    def parse_sender(idx: int, genpos: int, comm: int) -> tuple:
        """Consume the sender block. Returns (data_qubit, end_pos, is_teledata)."""
        instrs = circuits[idx].instructions
        pos = genpos + 1  # skip gen_ent
        cx_instr = instrs[pos]
        if cx_instr["name"] != "cx" or cx_instr["qubits"][1] != comm:
            raise ValueError("Unexpected telegate/teledata sender structure during union.")
        data_qubit = cx_instr["qubits"][0]
        pos += 1
        if instrs[pos]["name"] == "h" and instrs[pos]["qubits"] == data_qubit:
            # teledata sender: h(data), measure(data), measure(comm),
            #                  cif/x(data)/endcif, cif/x(comm)/endcif, send(s)
            pos += 1  # h(data)
            while instrs[pos]["name"] == "measure":
                pos += 1
            pos = skip_cif_blocks(instrs, pos)
            while pos < len(instrs) and instrs[pos]["name"] == "send":
                pos += 1
            return data_qubit, pos, True
        # telegate sender: measure(comm), cif/x(comm)/endcif, send(s), recv(s),
        #                  cif(xor)/z(data)/endcif
        pos += 1  # measure(comm)
        pos = skip_cif_blocks(instrs, pos)
        while pos < len(instrs) and instrs[pos]["name"] == "send":
            pos += 1
        while pos < len(instrs) and instrs[pos]["name"] == "recv":
            pos += 1
        pos = skip_cif_blocks(instrs, pos)  # cif(xor), z(data), endcif
        return data_qubit, pos, False

    def parse_receiver_telegate(tag: str, idx: int, genpos: int, comm: int, data_global: int) -> int:
        """Consume a telegate receiver block, recording the collapsed gates. Returns end_pos."""
        instrs = circuits[idx].instructions
        pos = genpos + 1                      # skip gen_ent
        pos += 1                              # recv
        pos = skip_cif_blocks(instrs, pos)    # cif/x(comm)/endcif (entangler correction)
        # The locally-controlled gates run until the disentangler starts: h(comm), measure(comm).
        local_gates = []
        while not (instrs[pos]["name"] == "h" and instrs[pos]["qubits"] == comm
                   and instrs[pos + 1]["name"] == "measure"):
            local_gates.append(instrs[pos])
            pos += 1
        pos += 2                              # h(comm), measure(comm)
        pos = skip_cif_blocks(instrs, pos)    # cif/x(comm)/endcif (disentangler correction)
        while pos < len(instrs) and instrs[pos]["name"] == "send":
            pos += 1
        for gate in local_gates:
            collapsed[tag].append(reindex(gate, idx, alias={comm: data_global}))
        return pos

    def parse_receiver_teledata(tag: str, idx: int, genpos: int, comm: int, data_global: int) -> int:
        """Consume a teledata receiver block, recording the swap. Returns end_pos."""
        instrs = circuits[idx].instructions
        pos = genpos + 1                      # skip gen_ent
        pos += 1                              # recv
        pos = skip_cif_blocks(instrs, pos)    # cif/x(comm)/endcif, cif/z(comm)/endcif
        swap_instr = instrs[pos]
        if swap_instr["name"] != "swap":
            raise ValueError("Unexpected teledata receiver structure during union.")
        sw = swap_instr["qubits"]
        data_recv = sw[0] if sw[1] == comm else sw[1]
        pos += 1
        collapsed[tag].append({"name": "swap", "qubits": [data_global, gq(idx, data_recv)]})
        return pos

    for tag, group in groups.items():
        collapsed[tag] = []
        sender = find_sender(group)
        data_qubit, sender_end, is_teledata = parse_sender(
            sender, group["genpos"][sender], group["comms"][sender]
        )
        data_global = gq(sender, data_qubit)
        blocks[sender].append((group["genpos"][sender], sender_end, tag))

        for idx, genpos in group["genpos"].items():
            if idx == sender:
                continue
            comm = group["comms"][idx]
            if is_teledata:
                end = parse_receiver_teledata(tag, idx, genpos, comm, data_global)
            else:
                end = parse_receiver_telegate(tag, idx, genpos, comm, data_global)
            blocks[idx].append((genpos, end, tag))

    # -----------------------------------------------------------------
    # 3. Turn every circuit into a token stream: plain instructions to emit (reindexed) and the
    #    barrier that marks the rendezvous point of each protocol.
    # -----------------------------------------------------------------
    tokens: dict = {}
    for idx, circuit in enumerate(circuits):
        starts = {start: (end, tag) for start, end, tag in blocks[idx]}
        stream = []
        pos = 0
        while pos < len(circuit.instructions):
            if pos in starts:
                end, tag = starts[pos]
                stream.append(("barrier", tag))
                pos = end
            else:
                stream.append(("plain", reindex(circuit.instructions[pos], idx)))
                pos += 1
        tokens[idx] = stream

    # -----------------------------------------------------------------
    # 4. Merge the streams, releasing each barrier (and emitting its collapsed gates) only once
    #    every participant of the protocol has reached it.
    # -----------------------------------------------------------------
    union_instructions: list = []
    pointers = [0] * len(circuits)
    arrived: dict = {}
    emitted: set = set()

    while any(pointers[idx] < len(tokens[idx]) for idx in range(len(circuits))):
        progress = False
        for idx in range(len(circuits)):
            if pointers[idx] >= len(tokens[idx]):
                continue
            kind, payload = tokens[idx][pointers[idx]]
            if kind == "plain":
                union_instructions.append(payload)
                pointers[idx] += 1
                progress = True
            else:  # barrier
                tag = payload
                participants = set(groups[tag]["genpos"].keys())
                arrived.setdefault(tag, set()).add(idx)
                if arrived[tag] == participants:
                    if tag not in emitted:
                        union_instructions.extend(collapsed[tag])
                        emitted.add(tag)
                    for j in participants:
                        pointers[j] += 1
                    progress = True
        if not progress:
            raise ValueError("Deadlock while performing the union: the communication protocols "
                             "between the given circuits could not be matched.")

    union_circuit = CunqaCircuit(
        num_qubits=sum(c.num_qubits[0] + c.num_qubits[1] for c in circuits),
        num_clbits=sum(c.num_clbits for c in circuits),
        id="|".join(c.id for c in circuits),
    )
    union_circuit.add_instructions(union_instructions)
    union_circuit.is_dynamic = any(instr.get("name") == "cif" for instr in union_instructions)
    return union_circuit

def add(circuits: list[CunqaCircuit]) -> CunqaCircuit:
    """
    This function concatenates the instructions of two circuits.

    It appends the gates from a list of circuits into a final resulting circuit. The
    order of the list passed as an argument is important, since the instructions will be appended
    in that same order.

    In this operation, if two circuits in the list contain communication directives between them,
    an exception will be raised to prevent potential deadlocks and undefined or incorrectly
    specified behavior.

    This operation is the inverse of the :py:func:`vsplit`.
    """
    if not circuits:
        raise ValueError("Empty list passed to perform union.")
    if len(circuits) == 1:
        logger.warning("Not enough circuits to perform an addition, returning the original circuit.")
        return circuits[0]

    circuits = copy.deepcopy(circuits)
    circuit_ids = {c.id for c in circuits}

    addition_circuit = CunqaCircuit(
        num_qubits=max(c.num_qubits[0] for c in circuits),
        num_clbits=max(c.num_clbits for c in circuits),
        id="+".join(c.id for c in circuits),
    )

    addition_instructions: list[dict] = []

    for circuit in circuits:
        for instr in circuit.instructions:
            if instr["name"] in REMOTE_GATES:
                for circ_id in instr.get("circuits", []):
                    if circ_id != circuit.id and circ_id in circuit_ids:
                        raise ValueError("Cannot add two circuits that communicate with each other.")
                    
            addition_instructions.append(instr)

    addition_circuit.add_instructions(addition_instructions)
    return addition_circuit
