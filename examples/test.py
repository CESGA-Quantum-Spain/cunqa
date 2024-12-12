
install_path = os.getenv("INSTALL_PATH")
sys.path.insert(0, install_path)
sys.path.insert(0, "/mnt/netapp1/Store_CESGA/home/cesga/mlosada/api/api-simulator/python")

from qpu import QPU, getQPUs

lista = getQPUs()
print("QPUs we are going to work with: ")
print(" ")
for q in lista:
    print("QPU backend info:")
    print(" ")
    q.backend.info()


# we define a circuit




