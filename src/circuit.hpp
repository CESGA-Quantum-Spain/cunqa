#pragma once

#include <vector>

namespace cunqa {

struct Circuit {
    virtual ~Circuit() = default;

    virtual void update_params(const std::vector<double>& new_params) = 0;
};

}