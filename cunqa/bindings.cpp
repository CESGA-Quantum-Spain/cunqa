#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>

#include "comm/client.hpp"
#include "json.hpp"
 
namespace py = pybind11;
using namespace cunqa::comm;

PYBIND11_MODULE(qclient, m) {

    m.doc() = "TODO";
 
    py::class_<FutureWrapper<Client>>(m, "FutureWrapper")
        .def("get", &FutureWrapper<Client>::get)
        .def("valid", &FutureWrapper<Client>::valid);

    py::class_<Client>(m, "QClient")
 
        .def(py::init<>())

        .def("connect", [](Client &c, const std::string& endpoint) { 
            c.connect(endpoint); 
        })
 
        .def("send_circuit", [](Client &c, const std::string& circuit) { 
            return FutureWrapper<Client>(c.send_circuit(circuit)); 
        })

        .def("send_parameters", [](Client &c, const std::string& parameters) { 
            return FutureWrapper<Client>(c.send_parameters(parameters)); 
        });

    m.def("write_on_file", [](const std::string& local_data, const std::string& filename, const std::string& suffix) {
        cunqa::JSON j = cunqa::JSON::parse(local_data);
        cunqa::write_on_file(j, filename, suffix);
    },
    py::arg("local_data"),
    py::arg("filename"),
    py::arg("suffix") = "");
}