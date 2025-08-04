/* Copyright (C) 2022 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer]*/
#ifndef CBM_ALGO_BUILD_INFO_H
#define CBM_ALGO_BUILD_INFO_H

#include <string>

#define MAKE_GCC_VERSION(major, minor, patch) ((major) *10000 + (minor) *100 + (patch))
#define GCC_VERSION MAKE_GCC_VERSION(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)

#if __has_include(<execution>) && !defined(__CLING__)
#include <execution>  // for feature test macro __cpp_lib_parallel_algorithm
#endif

#if defined(HAVE_TBB) && defined(__cpp_lib_parallel_algorithm)
#define HAVE_PARALLEL_STL_LIBTBB
#endif

// PoolSTL triggers an internal error in GCC 10, so only enable it for GCC 11 and later
#if GCC_VERSION >= MAKE_GCC_VERSION(11, 0, 0)
#define HAVE_PARALLEL_STL_POOLSTL
#endif

#if __has_include(<omp.h>)
#define HAVE_OMP
#endif

// Ensure we have the boost compression header AND flesnet is compiled with compression enabled
#if __has_include(<boost/iostreams/filter/zstd.hpp>) && defined(BOOST_IOS_HAS_ZSTD)
#define HAVE_ZSTD
#endif

namespace cbm::algo::BuildInfo
{

  extern const std::string GIT_HASH;
  extern const std::string BUILD_TYPE;
  extern const bool GPU_DEBUG;

  inline constexpr bool WITH_TBB =
#ifdef HAVE_TBB
    true;
#else
    false;
#endif

  inline constexpr bool WITH_PARALLEL_STL =
#ifdef HAVE_PARALLEL_STL_LIBTBB
    true;
#else
    false;
#endif

  inline constexpr bool WITH_OMP =
#ifdef HAVE_OMP
    true;
#else
    false;
#endif

  inline constexpr bool WITH_ZSTD =
#ifdef HAVE_ZSTD
    true;
#else
    false;
#endif

}  // namespace cbm::algo::BuildInfo

#endif  // CBM_ALGO_BUILD_INFO_H
