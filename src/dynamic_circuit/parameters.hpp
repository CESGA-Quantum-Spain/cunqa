#pragma once

#include <vector>
#include <array>
#include <stdexcept>
#include <deque>


namespace cunqa {

struct Parameters {
    std::deque<double> values;
    std::vector<double*> update_list;

    double* add_parameter(double value) {
        values.push_back(value);

        double* ptr = &values.back();
        update_list.push_back(ptr);

        return ptr;
    }

    template <std::size_t N>
    std::array<double*, N> add_parameters(const std::array<double, N>& input_values) {
        std::array<double*, N> ptrs{};
        for (std::size_t i = 0; i < N; ++i) {
            ptrs[i] = add_parameter(input_values[i]);
        }
        return ptrs;
    }

    std::vector<double*> add_parameters(const std::vector<double>& input_values) {
        std::vector<double*> ptrs;
        ptrs.reserve(input_values.size());
        for (double value : input_values) {
            ptrs.push_back(add_parameter(value));
        }
        return ptrs;
    }

    void update_params(const std::vector<double>& new_params) {
        if (new_params.size() != update_list.size()) {
            throw std::runtime_error(
                "Number of parameters is " + std::to_string(update_list.size()) +
                " but " + std::to_string(new_params.size()) +
                " params were given."
            );
        }

        for (std::size_t i = 0; i < update_list.size(); i++) {
            *(update_list[i]) = new_params[i];
        }
    }
};

}