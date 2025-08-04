/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_COMPAT_ALGORITHMS_H
#define CBM_ALGO_BASE_COMPAT_ALGORITHMS_H

/**
 * @file Algorithms.h
 * @brief This file contains compatibility wrappers for parallel stl algorithms.
 *
 * The parallel algorithms are only available if the compiler supports C++17. Some older
 * compilers don't ship with the parallel algorithms, so this wrapper falls back to
 * sequential algorithms in that case.
 * Also gcc requires the TBB library to be installed to use the parallel algorithms.
 * If TBB is not available, we also falls back to sequential algorithms.
**/

#include "BuildInfo.h"

#include <algorithm>
#include <poolstl/algorithm>
#include <poolstl/execution>

#ifdef HAVE_PARALLEL_ALGORITHM
#include <execution>
#endif

namespace cbm::algo
{

  /**
   * @brief Get the global thread pool for parallel stl algorithms
   * @details This function returns a reference to the global thread pool used by the parallel stl algorithms.
   * At the beginning it's initialized with the number of available threads. Otherwise this function should only be
   * used in conjunction with the parallel stl algorithms via poolstl.
  **/
  task_thread_pool::task_thread_pool& GetGlobalSTLThreadPool();

  /**
   * @brief Wrapper for std::sort
   *
   * Attempts to use the parallel version of std::sort if available. Falls back to the sequential version otherwise.
   * Parallel version currently requires Linux, GCC compiler and libTBB.
  */
  template<typename It, typename Compare>
  void Sort(It first, It last, Compare comp)
  {
// Disable parallel sorting for the moment
// The underlying implementation in libTBB has a massive memory leak:
// https://community.intel.com/t5/Intel-oneAPI-Threading-Building/std-sort-std-execution-par-unseq-has-a-memory-leak-on-Linux/m-p/1580910
//
// Update 2024-05-02: Add poolstl as a replacement for libTBB
#if 0
// #ifdef HAVE_PARALLEL_STL_LIBTBB
    std::sort(std::execution::par_unseq, first, last, comp);
#elif defined(HAVE_PARALLEL_STL_POOLSTL)
    std::sort(poolstl::par.on(GetGlobalSTLThreadPool()), first, last, comp);
#else
    std::sort(first, last, comp);
#endif
  }
}  // namespace cbm::algo


#endif
