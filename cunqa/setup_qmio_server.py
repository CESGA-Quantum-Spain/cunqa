import zmq
import os, sys
import random
import pickle

sys.path.append(os.getenv("HOME"))

from cunqa.logger import logger

ZMQ_SERVER = os.getenv('ZMQ_SERVER') 

if len(sys.argv) > 1:
    intermediary_endpoint = sys.argv[1]
else:
    logger.error("Intermediary endpoint not provided.")

qpu_context = zmq.Context()
qpu_client = qpu_context.socket(zmq.REQ)  
qpu_client.connect(ZMQ_SERVER)

intermediary_context = zmq.Context()
intermediary_server = intermediary_context.socket(zmq.ROUTER)  
intermediary_port = random.randint(49152, 65535)
intermediary_server.bind(intermediary_endpoint)

#inter_server.connect(ZMQ_SERVER)

waiting = True
while waiting:
    logger.debug("Waiting for a circuit to be executed in the QPU...")
    message = intermediary_server.recv_multipart() # circuit = (instructions, config)
    logger.debug("Circuit received for the intermediary server on the QPU node")
    client_address, serialized_circuit = message
    circuit = pickle.loads(serialized_circuit)
    qpu_client.send_pyobj(circuit)
    logger.debug("Circuit sent to QPU. Waiting for results...")
    result = qpu_client.recv_pyobj()
    logger.debug(f"Result: {result}")
    serialized_result = pickle.dumps(result)
    intermediary_server.send_multipart([client_address, serialized_result])

logger.debug("Everything was OK. Closing sockets...")
qpu_client.close()
qpu_context.term()

intermediary_server.close()
intermediary_context.term()
    
