#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "comm/client.hpp"
 
namespace py = pybind11;
 
PYBIND11_MODULE(qclient, m) {
 
    m.doc() = "Clase QClient, que corresponde con la clase Client() en C++";
 
    py::class_<Client>(m, "QClient")
 
        .def(py::init<const std::optional<std::string> &>(), py::arg("filepath") = std::nullopt)  // Constructor sin argumentos
 
        .def("connect", &Client::connect, py::arg("qpu_id") = 0, py::arg("net") = "ib0") // Metodo
 
        .def("read_result", &Client::read_result) // Metodo
 
        .def("send_data", py::overload_cast<const std::string&>(&Client::send_data)) 
        .def("send_data", py::overload_cast<std::ifstream&>(&Client::send_data)) 

        .def("stop", &Client::stop); // Metodo
 
}