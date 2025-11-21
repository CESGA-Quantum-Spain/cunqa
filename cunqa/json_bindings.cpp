#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "utils/json.hpp"

namespace py = pybind11;

std::string read_from_file_str(const std::string &filename) {
    nlohmann::json j = cunqa::read_from_file(filename);
    return j.dump();
}

PYBIND11_MODULE(pyjson, m) {
    m.doc() = "Bindings Python para read_from_file de cunqa";

    m.def(
        "read_from_file",
        &read_from_file_str,
        py::arg("filename"),
        "Reads JSON with locks and returns a string"
    );
}