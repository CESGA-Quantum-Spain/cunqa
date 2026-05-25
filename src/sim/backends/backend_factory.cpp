// simulator_factory.cpp

#include "backend_factory.hpp"
#include "no_comm/nc_backend.hpp"
#include "classical_comm/cc_backend.hpp"
#include "quantum_comm/qc_backend.hpp"

#include "utils/json.hpp"
#include "utils/helpers/murmur_hash.hpp"

#include <stdexcept>

namespace cunqa {
namespace sim {

std::unique_ptr<Backend> make_backend(
    std::unique_ptr<Simulator> simulator,
    const std::string& backend_type, 
    const JSON& backend_json
)
{
    switch(murmur::hash(backend_type)) {
        case murmur::hash("nc"): 
            return std::make_unique<NCBackend>(std::move(simulator), backend_json);
        case murmur::hash("cc"):
            return std::make_unique<CCBackend>(std::move(simulator), backend_json);
        case murmur::hash("qc"):
            return std::make_unique<QCBackend>(std::move(simulator), backend_json);
        default:
            throw std::invalid_argument("Unknown backend type: " + backend_type);
    }
}

}
}