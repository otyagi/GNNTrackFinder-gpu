/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportHeader.h
/// \brief  Base class for the report tail (header)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  23.02.2024

#pragma once

#include "CbmQaReportEngine.h"

#include <string>
#include <string_view>

namespace cbm::qa::report
{
  /// \class  Tail
  /// \brief  Tail of the report
  class Tail {
   public:
    /// \brief Destructor
    virtual ~Tail() = default;

    /// \brief Gets body of the element
    /// \param engine  A concrete implementation of the Engine to get the element body
    std::string GetBody(const Engine& engine) const { return engine.TailBody(*this); }
  };
}  // namespace cbm::qa::report
