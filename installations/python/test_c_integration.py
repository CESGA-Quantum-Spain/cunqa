

class _Cluster():
    def __init__(list_qpus = None):
        self.list_qpus = list_qpus


def deploy_cluster(num_qpus, list_backends=None):
    print("Levanto {} servidores y creo el fichero qpus.json".format(num_qpus))
    #Entiendo que esto lo hace la parte de c++ de Jorge. DUDA:?Que hace exactamente
    # el metodo connect de la clase Client de c++? Para instanciar la clase Client es
    # necesario que exista el archivo qpus.json, en particular esto significa que ya
    # tenemos levantados los servidores. En tal caso, ?que papel juega el connect?

    list_qpus = []
    with open("./qpus.json") as net_config:
        net_config_json = json.load(net_config)
        for key, value in net_config_json.items():
            if int(key) < len(list_backends):
                qpu_aux = QPU(id = key, server_endpoint = "tcp://" + value[0]["hostname"] + ":" + value[0]["port"]), backend = list_backends[int(key)])
                list_qpus.append(qpu_aux)
            else:
                qpu_aux = QPU(id = key, server_endpoint = "tcp://" + value[0]["hostname"] + ":" + value[0]["port"]))
                list_qpus.append(qpu_aux)

    cluster = _Cluster(list_qpus = list_qpus)
    return cluster

    

