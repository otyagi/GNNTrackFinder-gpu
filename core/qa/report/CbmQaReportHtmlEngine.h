/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportHtmlEngine.h
/// \brief  HTML document engine for the cbm::qa::report
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  24.02.2024

#pragma once

#include "CbmQaReportEngine.h"

#include <string>
#include <string_view>

namespace cbm::qa::report
{
  /// \class HtmlEngine
  /// \brief HTML document engine
  class HtmlEngine : public Engine {
   public:
    /// \brief  Creates a body for figure
    /// \param  figure  Reference to figure
    /// \return Figure body
    std::string FigureBody(const Figure& figure) const override;

    /// \brief  Creates a body for header
    /// \param  header  Reference to header
    /// \return Header body
    std::string HeaderBody(const Header& header) const override;

    /// \brief  Creates a body for section
    /// \param  section  Reference to section
    /// \return Section body
    std::string SectionBody(const Section& section) const override;

    /// \brief  Creates a body for table
    /// \param  table  Reference to table
    /// \return Table body
    std::string TableBody(const Table& table) const override;

    /// \brief  Creates a body for tail
    /// \param  tail  Reference to tail
    /// \return Figure body
    std::string TailBody(const Tail& tail) const override;

    /// \brief  Returns script extention
    std::string ScriptExtention() const override { return "html"; };

    /// \brief  Returns engine name
    std::string MyName() const override { return "HTML engine"; }

   private:
    // TODO: control with config
    static constexpr double kFigureWidth              = 0.9;     ///< Figure width [in page width]
    static constexpr std::string_view kTableTextAlign = "left";  ///< Table: align
    static constexpr int kTablePadding                = 4;       ///< Table: rows padding [px]
  };
}  // namespace cbm::qa::report
