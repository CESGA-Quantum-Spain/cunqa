#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "comm/client.hpp"
#include "comm/future_wrapper.hpp"
#include "logger/logger.hpp"
 
namespace py = pybind11;

PYBIND11_MODULE(qclient, m) {

    m.doc() = "TODO";
 
    py::class_<FutureWrapper>(m, "FutureWrapper")
        .def("get", &FutureWrapper::get)
        .def("valid", &FutureWrapper::valid);

    py::class_<Client>(m, "QClient")
 
        .def(py::init<const std::optional<std::string> &>(), py::arg("filepath") = std::nullopt)  // Constructor sin argumentos
 
        .def("connect", &Client::connect, py::arg("task_id") = "") // Metodo
 
        .def("send_circuit", [](Client &c, const std::string& circuit) { 
            return FutureWrapper(c.send_circuit(circuit)); 
        })

        .def("send_parameters", [](Client &c, const std::string& parameters) { 
            return FutureWrapper(c.send_parameters(parameters)); 
        });
}