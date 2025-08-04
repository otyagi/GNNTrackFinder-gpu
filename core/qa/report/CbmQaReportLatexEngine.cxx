/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportLatexEngine.h
/// \brief  LaTeX plain document engine for the cbm::qa::report (source)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  23.02.2024

#include "CbmQaReportLatexEngine.h"

#include "CbmQaReportFigure.h"
#include "CbmQaReportHeader.h"
#include "CbmQaReportLatexFormat.h"
#include "CbmQaReportSection.h"
#include "CbmQaReportTable.h"
#include "CbmQaReportTail.h"
#include "Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <numeric>
#include <regex>
#include <sstream>

using cbm::qa::report::Figure;
using cbm::qa::report::Header;
using cbm::qa::report::LatexEngine;
using cbm::qa::report::LatexFormat;
using cbm::qa::report::Section;
using cbm::qa::report::Table;
using cbm::qa::report::Tail;


// ---------------------------------------------------------------------------------------------------------------------
//
std::string LatexEngine::FigureBody(const Figure& figure) const
{
  size_t nPlots = figure.GetPlots().size();
  if (nPlots == 0) {
    LOG(warn) << "No plots provided for figure " << figure.GetLabel() << ". Ignoring.";
    return "";
  }

  std::stringstream out;
  out << "\\begin{figure}[H]\n";
  out << "  \\centering\n";
  if (nPlots == 1) {
    out << "  \\includegraphics[width=" << LatexEngine::kFigureWidth << "\\textwidth]{" << figure.GetPlots()[0].fsPath
        << "}\n";
  }
  else {
    std::vector<size_t> vPlotGrid;
    bool bDefineDefaultGrid = figure.GetPlotGrid().empty();
    if (!bDefineDefaultGrid) {
      size_t nPlotsByGrid = std::accumulate(figure.GetPlotGrid().begin(), figure.GetPlotGrid().end(), 0);
      if (nPlotsByGrid < nPlots) {
        LOG(warn) << "Figure " << figure.GetLabel()
                  << ": provided grid does not fit the subfigures, define the default one";
        bDefineDefaultGrid = true;
      }
    }

    if (bDefineDefaultGrid) {
      constexpr size_t nX = 3;
      auto nY             = static_cast<size_t>(std::ceil(static_cast<double>(nPlots) / nX));
      vPlotGrid.resize(nY, nX);
    }
    else {
      vPlotGrid = figure.GetPlotGrid();
    }

    for (auto& i : vPlotGrid) {
      LOG(info) << "- " << i;
    }

    size_t iPlot = 0;
    for (size_t iY = 0; iY < vPlotGrid.size(); ++iY) {
      if (iPlot >= nPlots) {
        break;
      }
      size_t nX = vPlotGrid[iY];
      if (nX == 0) {
        continue;
      }
      double wSize = 0.90 / std::min(nX, nPlots - iPlot);
      for (size_t iX = 0; iX < nX; ++iX) {
        if (iPlot >= nPlots) {
          break;
        }
        const auto& plot = figure.GetPlots()[iPlot];
        out << "  \\begin{subfigure}[t]{" << wSize << "\\textwidth}\n";
        out << "    \\centering\n";
        out << "    \\includegraphics[width=\\linewidth]{" << plot.fsPath << "}\n";
        out << "    \\caption{" << plot.fsCaption << "}\n";
        if (!plot.fsLabel.empty()) {
          out << "    \\label{" << figure.GetLabel() << ":" << plot.fsLabel << "}\n";
        }
        out << "  \\end{subfigure}\n";
        if (iX == nX - 1) {
          out << '\n';
          if (iY != vPlotGrid.size() - 1) {
            out << "\\vspace{3mm}\n";
          }
        }
        else {
          out << "\\quad\n";
        }
        ++iPlot;
      }
    }
  }

  if (!figure.GetCaption().empty()) {
    out << "  \\caption{" << LatexFormat::Apply(figure.GetCaption()) << "}\n";
  }
  out << "  \\label{" << figure.GetLabel() << "}\n";
  out << "\\end{figure}\n";
  return out.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string LatexEngine::HeaderBody(const Header& header) const
{
  std::stringstream out;

  // TODO: Move these definitions to text-file (?)
  // Settings
  out << "\\documentclass[12pt]{article}\n";
  out << "\\usepackage[english]{babel}\n";
  out << "\\usepackage{array}\n";
  out << "\\usepackage{graphicx}\n";
  out << "\\usepackage{float}\n";
  out << "\\usepackage{longtable}\n";
  out << "\\usepackage{helvet}\n";
  out << "\\usepackage{sansmath}\n\\sansmath\n";
  out << "\\usepackage{geometry}\n";
  out << "\\usepackage{subcaption}\n";
  out << "\\geometry{a4paper, total={170mm,257mm}, left=25mm, right=25mm, top=30mm, bottom=30mm}\n";
  //out << "\\setlength{\\parindent}{0cm}\n";
  out << "\\usepackage{hyperref}\n";
  //out << "\\usepackage{color}\n";
  //out << "\\definecolor{hrefcolor}{rgb}{0.2, 0.2, 0.6}\n";
  //out << "\\hypersetup{color}{colorlinks=true, urlcolor=hrefcolor,linkcolor=black,citecolor=hrefcolor}\n";
  if (!header.GetPageHeader().empty()) {
    //out << "\\usepackage{fancyhdr}\n\\pagestyle{fancy}\n\\lhead{"
    //    << LatexFormat::Apply(header.GetPageHeader()) << "}\n";
  }
  out << "\n";
  out << "\\newcolumntype{L}[1]{>{\\flushleft\\arraybackslash}p{#1}}\n";

  // Title page settings
  out << "\\title{" << LatexFormat::Apply(header.GetTitle()) << "\\\\ \\vspace{1cm} \\Large "
      << LatexFormat::Apply(header.GetSubtitle()) << "}\n";
  out << "\\author{Author: " << LatexFormat::Apply(header.GetAuthor()) << "}\n";
  auto timeNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  out << "\\date{Generated on " << std::put_time(std::localtime(&timeNow), "%a %d.%m.%Y, %X") << "}\n";


  // Title page
  out << "\\begin{document}\n";
  out << "\\begin{titlepage}\n";
  out << "  \\maketitle\n";
  out << "  \\thispagestyle{empty}\n";
  out << "  \\vfill\n";
  //out << "  \\centering\n";
  out << "  Setup: " << LatexFormat::Apply(header.GetSetup()) << "\\\\\n";
  out << "\\end{titlepage}\n";
  out << "\\newpage\n";
  out << "\\tableofcontents\n";
  out << "\\newpage\n";

  out << '\n';
  return out.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string LatexEngine::SectionBody(const Section& section) const
{
  std::stringstream out;
  std::string sSectionType = "";
  if (section.GetLevel() == 0) {
    sSectionType = "section";
  }
  else if (section.GetLevel() == 1) {
    sSectionType = "subsection";
  }
  else {
    sSectionType = "subsubsection";
  }

  if (section.GetLevel() == 0) {
    out << "\\newpage\n";
  }
  out << "\\" << sSectionType << "{" << section.GetTitle() << "}\n\n";
  for (const auto& pElement : section.GetDaughterElements()) {
    out << pElement->GetBody(*this) << '\n';
  }
  out << '\n';
  return out.str();
}


// ---------------------------------------------------------------------------------------------------------------------
//
std::string LatexEngine::TableBody(const Table& table) const
{
  std::stringstream out;
  int nRows = table.GetNofRows();
  int nCols = table.GetNofCols();


  out << "\\begin{table}\n";
  out << "\\footnotesize\n";
  // tabular header
  if (!table.GetCaption().empty()) {
    out << "  \\caption{" << LatexFormat::Apply(table.GetCaption()) << "}\n";
  }
  out << "  \\begin{longtable}{" << std::string(nCols, 'c') << "}\n";
  // table header
  out << "    \\hline\n";
  out << "    ";
  for (int iCol = 0; iCol < nCols; ++iCol) {
    out << table.GetColumnTitle(iCol) << ((iCol < nCols - 1) ? " & " : " \\\\\n");
  }
  out << "    \\hline\n";
  // table body
  for (int iRow = 0; iRow < nRows; ++iRow) {
    out << "    ";
    for (int iCol = 0; iCol < nCols; ++iCol) {
      out << table(iRow, iCol) << ((iCol < nCols - 1) ? " & " : " \\\\\n");
    }
  }
  out << "    \\hline\n";

  out << "  \\end{longtable}\n";
  out << "  \\label{" << table.GetLabel() << "}\n";
  out << "\\end{table}\n";
  return out.str();
}


// ---------------------------------------------------------------------------------------------------------------------
//
std::string LatexEngine::TailBody(const Tail&) const
{
  std::stringstream out;
  out << "\\end{document}\n";
  return out.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void LatexEngine::Compile(const std::string& source) const
{
  // Search for compiler program in path
  std::string compiler = fsLatexCompiler.substr(0, fsLatexCompiler.find_first_of(' '));  // compiler name without opts
  int bCompilerFonud   = false;
  {
    std::stringstream stream(getenv("PATH"));
    std::string dir;
    while (!bCompilerFonud && std::getline(stream, dir, ':')) {
      bCompilerFonud = fs::exists(fs::path(dir) / compiler);
    }
  }

  if (!bCompilerFonud) {
    LOG(error) << "cbm::qa::report::LatexEngine::Compile(): compiler \"" << fsLatexCompiler << "\" not found in PATH. "
               << "Please use another compiler or compile the sourec in external program";
    return;
  }

  // Execute compiler
  fs::path sourceDir = fs::path(source).parent_path();
  fs::path currPath  = fs::current_path();
  fs::current_path(sourceDir);

  std::string logFile = compiler + ".log";
  std::string command = fmt::format("{} {} > {}", fsLatexCompiler, source, logFile);
  command             = command + ";" + command;  // compile two times to get contents

  LOG(info) << "cbm::qa::report::LatexEngine::Compile(): executing command: \n\t" << command;
  int res = std::system(command.c_str());
  fs::current_path(currPath);

  LOG(info) << "cbm::qa::report::LatexEngine::Compile(): compillation " << (res == 0 ? "succeed" : "failed")
            << "(compiler log: \"" << logFile << "\")";
}
