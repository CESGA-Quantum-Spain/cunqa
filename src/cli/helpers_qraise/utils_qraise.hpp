#pragma once

#include <algorithm>
#include <ranges>

#include "utils/registry.hpp"
#include "logger.hpp"

enum class Communications { NC, CC, QC };

bool exists_family_name(const std::string& family)
{
    return !cunqa::read_qpus(family).empty();
}

// The family name is used as the name of its registry directory
bool valid_family_name(const std::string& family)
{
    return !family.empty() && family[0] != '.' && family.find('/') == std::string::npos;
}

bool communication_supported_by_simulator(const std::string& simulator, const Communications& scheme) 
{
    switch (scheme) {
        case Communications::NC:
            return std::ranges::find(cunqa::SUPPORTED_SIMPLE_SIMULATORS, simulator)
                != cunqa::SUPPORTED_SIMPLE_SIMULATORS.end();
        case Communications::CC:
            return std::ranges::find(cunqa::SUPPORTED_CC_SIMULATORS, simulator)
                != cunqa::SUPPORTED_CC_SIMULATORS.end();
        case Communications::QC:
            return std::ranges::find(cunqa::SUPPORTED_QC_SIMULATORS, simulator)
                != cunqa::SUPPORTED_QC_SIMULATORS.end();
    }

    return false;
}

std::string get_communications_name(const Communications& scheme)
{
    switch (scheme) {
        case Communications::NC:
            return "nc";
        case Communications::CC:
            return "cc";
        case Communications::QC:
            return "qc";
    }

    return std::string();
};

Communications get_communications_class(const std::string_view& scheme_name)
{
    if (scheme_name == "nc")
        return Communications::NC;
    if (scheme_name == "cc")
        return Communications::CC;
    if (scheme_name == "qc")
        return Communications::QC;
    return Communications::NC;
};


void remove_tmp_files(const std::string filepath = "")
{
    if (!filepath.empty()) {
        std::string rmv_cmd = "rm " + filepath;
        std::system(rmv_cmd.c_str());
    } else {
        std::system("rm qraise_sbatch_tmp.sbatch");
    }
}


