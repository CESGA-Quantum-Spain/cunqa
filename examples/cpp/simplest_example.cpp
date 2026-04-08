#include <iostream>
#include <string>

#include "comm/client.hpp"
#include "utils/json.hpp"
#include "utils/constants.hpp"

std::string circuit1 = R"(
    {
        "id": "circuit1", 
        "config": {
            "shots": 10, 
            "method": "automatic", 
            "num_clbits": 2, 
            "num_qubits": 2, 
            "seed": 123123, 
            "device":{"device_name":"CPU", "target_devices":[]}
        }, 
        "instructions": [
            {"name": "h", "qubits": [0]}, 
            {"name": "cx", "qubits": [0, 1]}, 
            {"name": "measure", "qubits": [0], "clbits": [0]}, 
            {"name": "measure", "qubits": [1], "clbits": [1]}
        ], 
        "sending_to": [], 
        "is_dynamic": false
    }
)";

using namespace std::string_literals;
using namespace cunqa::comm;

cunqa::JSON read_file(const std::string& filename)
{
    std::ifstream in(filename);

    if (!in.is_open()) {
        throw std::runtime_error("Error opening the communications file.");
    }

    cunqa::JSON j;
    if (in.peek() != std::ifstream::traits_type::eof())
        in >> j;
    in.close();
    return j;
}


int main()
{
    cunqa::JSON qpus = read_file(cunqa::constants::QPUS_FILEPATH);

    std::vector<Client> clients(3);
    std::vector<std::string> circuits{circuit1, circuit1, circuit1};
    int i=0;
    for (const auto& qpu: qpus) {
        clients[i].connect(qpu.at("net").at("endpoint"));
        clients[i].send_circuit(circuit1);
        i++;
    }

    std::cout << clients[0].recv_results() << "\n";
    std::cout << clients[1].recv_results() << "\n";
    std::cout << clients[2].recv_results() << "\n";
    
    return 0;
}