#include <pybind11/pybind11.h>
#include <pybind11/numpy.h> // Vectors returned as numpy arrays. Must appear before stl.h
#include <pybind11/stl.h>

#include <string>

#include "comm/client.hpp"
#include "utils/helpers/qasm2_to_json.hpp"
#include "utils/helpers/json_to_qasm2.hpp"
#include "json.hpp"

#include "utils/probabilities/process_counts.hpp"
 
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

    m.def("qasm2_to_json", [](const std::string& circuit_qasm) {
        return qasm2_to_json(circuit_qasm).dump();
    });
    m.def("json_to_qasm2", [](const std::string& circuit_str) {
        JSON circuit_json = JSON::parse(circuit_str);

        return json_to_qasm2(circuit_json["instructions"], circuit_json["config"]);
    });

}

PYBIND11_MODULE(counts_and_probs, m) {
    m.doc() = "Functions for manipulating measurement counts and estimating probabilities";

    m.def("counts_to_probs",
        [](const std::map<std::string, int>& counts,
           bool per_qubit = false,
           const std::optional<std::vector<int>>& partial = std::nullopt) {
            auto result = countsToProbs(
                counts,
                per_qubit,
                partial.has_value() ? &partial.value() : nullptr
            );
            // Conversion to numpy array
            return py::array_t<double>(result.size(), result.data());
        },
        py::arg("counts"),
        py::arg("per_qubit") = false,
        py::arg("partial") = py::none(),
        R"pbdoc(
            Convert quantum measurement counts to probabilities.
            
            Args:
                counts: Dictionary mapping bitstrings to measurement counts
                per_qubit: If True, marginalize to per-qubit probabilities
                partial: Optional list of qubit indices to marginalize over
                
            Returns:
                List of probabilities corresponding to bitstrings
        )pbdoc"
    );

    m.def("recombine_probs",
        [](const std::vector<double>& probs,
           bool per_qubit,
           const std::optional<std::vector<int>>& partial_ptr,
           int num_qubits) {
            auto result = recombineProbs(
                probs,
                per_qubit,
                partial_ptr.has_value() ? &partial_ptr.value() : nullptr,
                num_qubits
            );
            // Conversion to numpy array
            return py::array_t<double>(result.size(), result.data());
        },
        py::arg("probs"),
        py::arg("per_qubit"),
        py::arg("partial") = py::none(),
        py::arg("num_qubits"),
        R"pbdoc(
            Recombine probabilities based on marginalization parameters.
            
            Args:
                probs: List of probabilities for all bitstrings
                per_qubit: If True, marginalize to per-qubit probabilities
                partial: Optional list of qubit indices to marginalize over
                num_qubits: Total number of qubits in the system
                
            Returns:
                List of recombined probabilities
        )pbdoc"
    );

    m.def("marginalize_counts",
        [](const std::map<std::string, int>& counts,
           const std::vector<int>& region_sizes,
           bool check_length = false) {
            return marginalizeCountsByRegions(counts, region_sizes, check_length);
        },
        py::arg("counts"),
        py::arg("region_sizes"),
        py::arg("check_length") = false,
        R"pbdoc(
            Marginalize counts by grouping qubits into regions.
            
            Divides bitstrings into regions of specified sizes and sums counts
            for matching reduced bitstrings across all regions.
            
            Args:
                counts: Dictionary mapping bitstrings to measurement counts
                region_sizes: List of region sizes (must sum to bitstring length)
                check_length: If True, validate that region sizes match bitstring length
                
            Returns:
                List of dictionaries, one per region, with marginalized counts
                
            Example:
                counts = {"010101": 112, "001101": 34, "111111": 1700}
                regions = marginalize_counts(counts, [2, 2, 2])
                # Returns 3 dicts: {"01": 112, "00": 34, "11": 1700}, 
                #                  {"01": 112, "11": 1734},
                #                  {"01": 146, "11": 1700}
        )pbdoc"
    );
}