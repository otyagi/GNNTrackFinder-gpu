/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_COMPAT_OPENMP_H
#define CBM_ALGO_BASE_COMPAT_OPENMP_H

#include "BuildInfo.h"

#ifdef HAVE_OMP
#include <omp.h>
#endif

#define CBM_PRAGMA(...) _Pragma(#__VA_ARGS__)

// OpenMP parallel for
// If OpenMP is not available, this macro expands to nothing
//
// Hiding the pragma in a macro isn't technically necessary, as the compiler will ignore it if OpenMP is not available.
// But it slightly increases readability as it's indented to the same level as the code it applies to.
//
// Accepts the same arguments as the OpenMP parallel for pragma.
#ifdef HAVE_OMP
#define CBM_PARALLEL_FOR(...) CBM_PRAGMA(omp parallel for __VA_ARGS__)
#else
#define CBM_PARALLEL_FOR(...)
#endif

// OpenMP parallel
#ifdef HAVE_OMP
#define CBM_PARALLEL(...) CBM_PRAGMA(omp parallel __VA_ARGS__)
#else
#define CBM_PARALLEL(...)
#endif

// generic omp pragma for other commands
#ifdef HAVE_OMP
#define CBM_OMP(...) CBM_PRAGMA(omp __VA_ARGS__)
#else
#define CBM_OMP(...)
#endif

namespace cbm::algo::openmp
{

#ifndef HAVE_OMP
  inline int GetMaxThreads() { return 1; }
  inline int GetThreadNum() { return 0; }
  inline int GetNumThreads() { return 1; }
  inline void SetNumThreads(int) {}
#else
  inline int GetMaxThreads() { return omp_get_max_threads(); }
  inline int GetThreadNum() { return omp_get_thread_num(); }
  inline int GetNumThreads() { return omp_get_num_threads(); }
  inline void SetNumThreads(int n) { omp_set_num_threads(n); }
#endif

}  // namespace cbm::algo::openmp

#endif  // CBM_ALGO_BASE_COMPAT_OPENMP_H
