#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "comm/client.hpp"
 
namespace py = pybind11;

template <typename T>
class FutureWrapper {
public:
    explicit FutureWrapper(std::future<T> f) : fut_(std::move(f)) {}

    // Obtener el resultado (bloquea hasta que esté disponible)
    T get() {
        return fut_.get();
    }

    // Comprobar si el futuro aún es válido
    bool valid() const {
        return fut_.valid();
    }

private:
    std::future<T> fut_;
}; 


PYBIND11_MODULE(qclient, m) {

    m.doc() = "TODO";
 
    py::class_<FutureWrapper<std::string>>(m, "FutureWrapper")
        .def("get", &FutureWrapper<std::string>::get)
        .def("valid", &FutureWrapper<std::string>::valid);

    py::class_<Client>(m, "QClient")
 
        .def(py::init<const std::optional<std::string> &>(), py::arg("filepath") = std::nullopt)  // Constructor sin argumentos
 
        .def("connect", &Client::connect, py::arg("task_id") = "", py::arg("net") = "ib0") // Metodo
 
        .def("send_circuit", [](Client &c, const std::string& circuit) { 
            return FutureWrapper<std::string>(c.send_circuit(circuit)); 
        });
}