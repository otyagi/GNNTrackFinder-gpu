/* Copyright (C) 2019 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "CbmFormatDecHexPrintout.h"

std::string FormatDecPrintout(uint64_t ulVal, char cFill, uint uWidth)
{
  std::stringstream ss;

  /// Set fill character and/or width if provided by user
  if (0 != cFill) ss << std::setfill(cFill);
  if (0 < uWidth) ss << std::setw(uWidth);

  /// push value
  ss << ulVal;

  /// Restore fill character if needed
  if (0 != cFill) ss << std::setfill(' ');

  return ss.str();
}

std::string FormatHexPrintout(uint64_t ulVal, char cFill, uint uWidth, bool bUppercase)
{
  std::stringstream ss;

  /// Set hex printout mode
  ss << std::hex;

  /// Set fill character and/or width if provided by user
  if (0 != cFill) ss << std::setfill(cFill);
  if (0 < uWidth) ss << std::setw(uWidth);
  if (bUppercase) ss << std::uppercase;

  /// push value
  ss << ulVal << std::dec;

  /// Restore fill character if needed
  if (0 != cFill) ss << std::setfill(' ');

  return ss.str();
}
