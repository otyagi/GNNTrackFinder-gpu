/* Copyright (C) 2010-2021 Frankfurt Institute for Advanced Studies, Goethe-Universität Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Maksym Zyzak, Igor Kulakov [committer], Sergey Gorbunov */

/// \file CaDefines.h
/// \brief Macros for the CA tracking algorithm
/// \details Try to minimize the amount of macros. Try to call this header only from cxx files.

#pragma once  // include this header only once per compilation unit

#include "AlgoFairloggerCompat.h"

#include <cassert>
#include <sstream>

// #define CBMCA_DEBUG_MODE

#if defined(CBMCA_DEBUG_MODE)

#define CBMCA_DEBUG_ASSERT(v)                                                                                          \
  if (!(v)) {                                                                                                          \
    LOG(error) << __FILE__ << ":" << __LINE__ << " assertion failed: " << #v << " = " << (v);                          \
    assert(v);                                                                                                         \
  }

#define CBMCA_DEBUG_SHOW(expr)                                                                                         \
  LOG(info) << __FILE__ << ":" << __LINE__ << ": \033[01;38;5;208m" << (#expr) << "\033[0m = " << (expr);

#define CBMCA_DEBUG_SHOWF(msg)                                                                                         \
  LOG(info) << "(!) " << __FILE__ << ":" << __LINE__ << ": \033[01;38;5;208m" << (#msg) << "\033[0m";

#define CBMCA_DEBUG_SHOWCONTAINER(cont)                                                                                \
  std::stringstream ss;                                                                                                \
  ss << __FILE__ << ":" << __LINE__ << ": \033[01;38;5;208m" << (#cont) << "\033[0m: ";                                \
  std::for_each(cont.cbegin(), cont.cend(), [](const auto& el) { ss << el << " "; });                                  \
  LOG(info) << ss.str();

#else  // not CBMCA_DEBUG_MODE

#define CBMCA_DEBUG_ASSERT(v)

#define CBMCA_DEBUG_SHOW(expr)

#define CBMCA_DEBUG_SHOWF(msg)

#define CBMCA_DEBUG_SHOWCONTAINER(cont)

#endif  // CBMCA_DEBUG_MODE
