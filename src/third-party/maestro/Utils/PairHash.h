/**
 * @file PairHash.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Class for hashing std::pair objects, useful for unordered_map and unordered_set.
 */

#pragma once


#ifndef _PAIR_HASH_H_
#define _PAIR_HASH_H_

namespace Utils {

	// or simply use boost::hash
	template <typename T1, typename T2 = T1> class PairHash {
	public:
		size_t operator()(const std::pair<T1, T2>& p) const
		{
			const auto h1 = std::hash<T1>{}(p.first);
			const auto h2 = std::hash<T2>{}(p.second);

			// alternative with boost combine:
			/*
			std::size_t seed = 0;
			boost::hash_combine(seed, h1);
			boost::hash_combine(seed, h1);

			return seed;
			*/

			return (h1 << 5) ^ h2;
		}
	};

}

#endif
