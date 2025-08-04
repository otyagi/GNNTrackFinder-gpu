/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportBuilder.h
/// \brief  Base class for the report builder
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  23.02.2024

#pragma once

#include "CbmQaReportDefines.h"
#include "CbmQaReportEngine.h"
#include "CbmQaReportHeader.h"
#include "CbmQaReportSection.h"
#include "CbmQaReportTail.h"

class TCanvas;

namespace cbm::qa::report
{
  /// \class Builder
  /// \brief Report builder
  class Builder {
   public:
    /// \brief Constructor
    /// \param title    Report title
    /// \param outPath  Output path
    Builder(std::string_view title, fs::path outPath);

    /// \brief Destructor
    virtual ~Builder() = default;

    /// \brief Adds section
    void AddSection(std::shared_ptr<Section> pSection) { fvpSections.push_back(pSection); }

    /// \brief Gets header
    std::shared_ptr<Header> GetHeader() { return fpHeader; }

    /// \brief Gets tail
    std::shared_ptr<Tail> GetTail() { return fpTail; }

    /// \brief Saves script
    /// \param engine   Engine of the document
    /// \param bCompile Flag: true - script will be compiled, if the compilation rule is defined by engine
    void CreateScript(Engine& engine, bool bCompile = true);

    /// \brief Converts path of the object in the code to one in the document source
    /// \param path  Path to the object
    std::string PathInSource(fs::path path) const { return fs::relative(path, fScriptsPath).string(); }

    /// \brief Returns absolute path to the figure
    /// \param relPath  Relative path to the figure
    std::string AbsFigurePath(fs::path relPath) const { return (fFiguresPath / relPath).string(); }

    /// \brief  Saves canvas
    /// \param  canv    Pointer to canvas
    /// \param  relPath Relative path to the canvas image
    /// \return Path to canvas in source
    /// \note   Creates the directory, if it does not exist
    std::string SaveCanvas(const TCanvas* canv, const std::string& relPath) const;

    /// \brief  Sets figure extention
    void SetFigureExtention(std::string_view figExtention) { fsFigureExtention = figExtention; }

   private:
    static constexpr std::string_view ksSourceName = "source";  ///< Name of source

    std::vector<std::shared_ptr<Section>> fvpSections;  ///< List of report sections
    std::string fsFigureExtention = "pdf";              ///< Figure extention
    fs::path fScriptsPath;                              ///< Scripts path
    fs::path fFiguresPath;                              ///< Figures path
    std::shared_ptr<Header> fpHeader = nullptr;         ///< Header of the report
    std::shared_ptr<Tail> fpTail     = nullptr;         ///< Tail of the report
  };
}  // namespace cbm::qa::report
