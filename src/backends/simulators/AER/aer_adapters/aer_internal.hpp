#pragma once

#include "aer_internal_fwd.hpp"

namespace cunqa {
namespace sim {
namespace aer_detail {

NoiseModelPtr make_noise_model();
NoiseModelPtr make_noise_model(const JSON& j);


} // namespace aer_detail
} // namespace sim
} // namespace cunqa