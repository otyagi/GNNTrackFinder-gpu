/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_EXCEPTIONS_H
#define CBM_ALGO_BASE_EXCEPTIONS_H

#include <exception>
#include <string_view>

#include <fmt/format.h>

namespace cbm::algo
{

  namespace detail
  {
    /**
     * @brief Base class for exceptions.
     *
     * @note Should not be thrown directly. Use one of the derived classes instead.
     */
    struct Exception : std::runtime_error {
      template<typename... Args>
      Exception(std::string_view fmt, Args&&... args)
        : std::runtime_error(fmt::format(fmt, std::forward<Args>(args)...))
      {
      }
    };
  }  // namespace detail

  /**
   * @brief Indicates an unrecoverable error. Should tear down the process.
   */
  struct FatalError : detail::Exception {
    using Exception::Exception;
  };

  /**
   * Indicates an error during timeslice processing. Timeslice will be discarded.
   * Processing can continue with new timeslice.
   */
  struct ProcessingError : detail::Exception {
    using Exception::Exception;
  };

}  // namespace cbm::algo

#endif
