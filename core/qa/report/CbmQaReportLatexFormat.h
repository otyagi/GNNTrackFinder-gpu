/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportLatexFormat.h
/// \brief  Common LaTeX utilities (header)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  28.03.2024

#pragma once

#include <string>
#include <string_view>

namespace cbm::qa::report
{
  // Latex formatting collection
  /// \class LatexFormat
  class LatexFormat {
   public:
    /// \brief Applies a LaTeX-friendly formatting
    static std::string Apply(std::string_view input);
  };
}  // namespace cbm::qa::report
