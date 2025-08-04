/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportLatexFormat.cxx
/// \brief  Common LaTeX utilities (source)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  28.03.2024

#include "CbmQaReportLatexFormat.h"

#include <regex>

using cbm::qa::report::LatexFormat;

// ---------------------------------------------------------------------------------------------------------------------
//
std::string LatexFormat::Apply(std::string_view input)
{
  auto output = std::string(input);

  // Replace all underscores with "\_"
  // TODO: ignore replacing in LaTeX formulas
  {
    std::regex rex("_");
    output = std::regex_replace(output, rex, "\\_");
  }

  return output;
}
