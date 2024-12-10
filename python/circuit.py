
def qasm2_to_json(qasm_str):
    """
    Transforms a QASM circuit to json.

    Args:
    ---------
    qc (str): circuit to transform to json.

    Return:
    ---------
    json dict with the circuit information.
    """
    lines = qasm_str.splitlines()
    json_data = {
        "qasm_version": None,
        "includes": [],
        "registers": [],
        "instructions": []
    }
    
    for line in lines:
        line = line.strip()
        if line.startswith("OPENQASM"):
            json_data["qasm_version"] = line.split()[1].replace(";", "")
        elif line.startswith("include"):
            json_data["includes"].append(line.split()[1].replace(";", "").strip('"'))
        elif line.startswith("qreg") or line.startswith("creg"):
            parts = line.split()
            register_type = parts[0]
            name, size = parts[1].split("[")
            size = int(size.replace("];", ""))
            json_data["registers"].append({"type": register_type, "name": name, "size": size})
        else:
            if line:
                parts = line.split()
                operation_name = parts[0]
                if operation_name != "measure":
                    if "," not in parts[1]:
                        qubits = parts[1].lstrip("q[").rstrip("];")
                        json_data["instructions"].append({"name":operation_name, "qubits":[qubits]})

                    #qubits = parts[1].replace(";", "").split(",")
                    else:
                        parts_split = parts[1].rstrip(";").split(",")
                        q_first = parts_split[0].split("[")[1].rstrip("]")
                        q_second = parts_split[1].split("[")[1].rstrip("]")
                        json_data["instructions"].append({"name":operation_name, "qubits":[q_first, q_second]})
                else:
                    qubits = parts[1].split("[")[1].rstrip("]")
                    memory_aux = parts[3].rstrip(";")
                    if "meas" in memory_aux:
                        memory = memory_aux.lstrip("meas[").rstrip("]")
                    else:
                        memory = memory_aux.lstrip("c[").rstrip("]")
                    json_data["instructions"].append({"name":operation_name, "qubits":[qubits], "memory":memory})

    return json_data

def qc_to_json(qc):
    """
    Transforms a QuantumCircuit to json.

    Args:
    ---------
    qc (QuantumCircuit): circuit to transform to json.

    Return:
    ---------
    json dict with the circuit information.
    """
    json_data = {
        "instructions":[]
    }
    for i in range(len(qc.data)):
        if qc.data[i].name == "barrier":
            pass
        elif qc.data[i].name != "measure":
            json_data["instructions"].append({"name":qc.data[i].name, 
                                              "qubits":[qc.data[i].qubits[j]._index for j in range(len(qc.data[i].qubits))],
                                              "params":"{}".format(qc.data[i].params)
                                             })
        else:
            json_data["instructions"].append({"name":qc.data[i].name, 
                                              "qubits":[qc.data[i].qubits[j]._index for j in range(len(qc.data[i].qubits))],
                                              "memory":[qc.data[i].clbits[j]._index for j in range(len(qc.data[i].clbits))]
                                             })

    return json_data