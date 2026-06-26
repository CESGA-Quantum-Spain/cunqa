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
  case cunqa::constants::AER:
      return AER_BASIS_GATES;
      break;
  case cunqa::constants::MUNICH:
      return MUNICH_BASIS_GATES;
      break;
  case cunqa::constants::MAESTRO:
      return MAESTRO_BASIS_GATES;
      break;
  case cunqa::constants::QULACS:
      return QULACS_BASIS_GATES;
      break;
  case cunqa::constants::QSIM:
      return QSIM_BASIS_GATES;
      break;
  case cunqa::constants::QUEST:
      return QUEST_BASIS_GATES;
      break;
  case cunqa::constants::CUNQASIM:
      return CUNQASIM_BASIS_GATES;
      break;
  default:
      return {};
      break;
  }
}


} // end namespace