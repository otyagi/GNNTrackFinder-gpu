/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "EnumDict.h"

#include <sstream>

void cbm::algo::detail::RaiseUnknownEntry(std::string_view str, const std::vector<std::string_view>& validEntries)
{
  std::ostringstream oss;
  oss << "Could not parse '" << str << "'. Valid entries are: ";

  for (size_t i = 0; i < validEntries.size(); ++i) {
    oss << validEntries[i];
    if (i != validEntries.size() - 1) {
      oss << ", ";
    }
  }
  throw std::invalid_argument(oss.str());
}
