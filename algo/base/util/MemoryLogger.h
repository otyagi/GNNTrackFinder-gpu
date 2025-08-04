/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#pragma once

#include <cstddef>

/**
 * @file MemoryLogger.h
 * @brief Memory logging
**/

namespace cbm::algo
{

  /**
   * @brief Track the memory usage of the process and write it to the log
  **/
  class MemoryLogger {

   public:
    /**
     * @brief Constructor
    **/
    MemoryLogger() = default;

    /**
     * @brief Destructor
    **/
    ~MemoryLogger() = default;

    /**
     * @brief Log the current memory usage
    **/
    void Log();

   private:
    size_t mLastRSS = 0;

    // Convert bytes to MB
    // Template to allow for different integer types
    template<typename T>
    T BytesToMB(T bytes) const;
  };

}  // namespace cbm::algo
