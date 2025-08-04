/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#pragma once

/**
 * @file StlUtils.h
 *
 * @brief This file contains utility functions for STL containers.
 */

#include <algorithm>
#include <string>
#include <string_view>

namespace cbm
{

  template<class C, class T>
  bool Contains(const C& container, const T& value)
  {
    return std::find(container.begin(), container.end(), value) != container.end();
  }

  /**
   * @brief Capitalize the first letter of a string. The rest of the string is made lowercase.
   */
  std::string Capitalize(std::string_view str);

}  // namespace cbm
