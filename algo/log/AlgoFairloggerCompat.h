/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_COMPAT_ONLINEDATALOG_H
#define CBM_ALGO_BASE_COMPAT_ONLINEDATALOG_H

#ifndef NO_ROOT
// Do not include FairLogger in GPU code (header might not be available)
// FIXME: Ensure NO_ROOT is defined for GPU compilation
//#if !defined(__NVCC__) && !defined(__HIPCC__) && !defined(SYCL_LANGUAGE_VERSION)
#define CBM_ONLINE_USE_FAIRLOGGER
//#endif
#endif

#ifdef CBM_ONLINE_USE_FAIRLOGGER
// #warning "Using fair version of Logger" // Keeping in source but commented out until the main problem is resolved
#include <fairlogger/Logger.h>
#define L_(level) LOG(level)

#else
/*
 *  // Keeping in source but commented out until the main problem is resolved
#warning "Using algo/???? version of Logger"

#if defined(NO_ROOT)
#warning "Reason: NO_ROOT defined"
#endif
#if defined(__NVCC__)
#warning "Reason: __NVCC__ defined"
#endif
#if defined(__HIPCC__)
#warning "Reason: __HIPCC__ defined"
#endif
#if defined(SYCL_LANGUAGE_VERSION)
#warning "Reason: SYCL_LANGUAGE_VERSION defined"
#endif
*/

#ifndef LOG

#include <log.hpp>
static constexpr severity_level warn = severity_level::warning;
#define LOG(level) L_(level)

#endif  // LOG

#endif  // CBM_ONLINE_USE_FAIRLOGGER

#endif  // CBM_ALGO_BASE_COMPAT_ONLINEDATALOG_H
