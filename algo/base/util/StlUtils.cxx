/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "StlUtils.h"

using namespace cbm;

std::string cbm::Capitalize(std::string_view str)
{
  if (str.empty()) {
    return std::string(str);
  }

  std::string result(str);
  result[0] = std::toupper(result[0]);
  for (size_t i = 1; i < result.size(); ++i)
    result[i] = std::tolower(result[i]);

  return result;
}
