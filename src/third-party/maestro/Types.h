/**
 * @file Types.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Some types to make the code portable.
 *
 */

#pragma once

#ifndef _TYPES_H_
#define _TYPES_H_

#include <memory>
#include <vector>

namespace Types {
	using qubit_t = uint_fast64_t; /**< The type of a qubit. */
	using qubits_vector = std::vector<qubit_t>; /**< The type of a vector of qubits. */
	using time_type = double; /**< The type of time. */
}

#endif // !_TYPES_H_
