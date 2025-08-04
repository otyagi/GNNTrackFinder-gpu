/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#pragma once

#include <cstddef>

/**
 * @file System.h
 * @brief System functions
**/

namespace cbm::algo
{

  /**
   * @brief Get the current resident set size (pyhysical memory usage) of the process
   * @return The current resident set size in bytes
   * @note Returns zero if the value cannot be determined
  **/
  size_t GetCurrentRSS();

  /**
   * @brief Get the peak resident set size (pyhysical memory usage) of the process
   * @return The peak resident set size in bytes
   * @note Returns zero if the value cannot be determined
  **/
  size_t GetPeakRSS();

}  // namespace cbm::algo
