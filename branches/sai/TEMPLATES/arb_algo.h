// =========================================================== //
//                                                             //
//   File      : arb_algo.h                                    //
//   Purpose   : idioms based on algorithm                     //
//                                                             //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2012   //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef ARB_ALGO_H
#define ARB_ALGO_H

#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif

template <typename T>
inline const T& force_in_range(const T& lower_bound, const T& value, const T& upper_bound) {
    return std::min(std::max(lower_bound, value), upper_bound);
}

#else
#error arb_algo.h included twice
#endif // ARB_ALGO_H
