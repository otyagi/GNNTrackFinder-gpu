/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportBeamerEngine.h
/// \brief  LaTeX Beamer engine for the cbm::qa::report (header)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  28.03.2024

#pragma once

#include "CbmQaReportEngine.h"

#include <string>
#include <string_view>

namespace cbm::qa::report
{
  /// \class BeamerEngine
  /// \brief Plain LaTeX document engine
  class BeamerEngine : public Engine {
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
    std::string ScriptExtention() const override { return "tex"; };

    /// \brief  Returns engine name
    std::string MyName() const override { return "LaTeX beamer presentation engine"; }

    /// \brief  Compiles source
    /// \param  source  Source path
    void Compile(const std::string& source) const override;

    /// \brief  Sets the LaTeX compilation program name
    /// \param  latexCompiler A LaTeX compiler
    /// \note   The compiler can contain options
    void SetLatexCompiler(std::string_view latexCompiler) { fsLatexCompiler = latexCompiler; }

   private:
    static constexpr double kFigureWidth = 1.0;  ///< Figure width [in textwidth]

    std::string fsLatexCompiler = "pdflatex -interaction=nonstopmode";
  };
}  // namespace cbm::qa::report
