#pragma once

#include <string>
#include <vector>

#include "utils/constants.hpp"

namespace {
using namespace cunqa::constants;

std::vector<std::string> get_basis_gates(const std::string simulator)
{
  switch (SIMULATORS_MAP.at(simulator))
  {
  case AER:
      return AER_BASIS_GATES;
      break;
  case MUNICH:
      return MUNICH_BASIS_GATES;
      break;
  case MAESTRO:
      return MAESTRO_BASIS_GATES;
      break;
  case QULACS:
      return QULACS_BASIS_GATES;
      break;
  case CUNQASIM:
      return CUNQASIM_BASIS_GATES;
      break;
  default:
      return {};
      break;
  }
}


} // end namespace