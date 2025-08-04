/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportLatexEngine.h
/// \brief  LaTeX Beamer engine for the cbm::qa::report (source)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  28.03.2024

#include "CbmQaReportBeamerEngine.h"

#include "CbmQaReportFigure.h"
#include "CbmQaReportHeader.h"
#include "CbmQaReportLatexFormat.h"
#include "CbmQaReportSection.h"
#include "CbmQaReportTable.h"
#include "CbmQaReportTail.h"
#include "Logger.h"

#include <boost/algorithm/string/join.hpp>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <numeric>
#include <regex>
#include <sstream>

using cbm::qa::report::BeamerEngine;
using cbm::qa::report::Figure;
using cbm::qa::report::Header;
using cbm::qa::report::LatexFormat;
using cbm::qa::report::Section;
using cbm::qa::report::Table;
using cbm::qa::report::Tail;

// ---------------------------------------------------------------------------------------------------------------------
//
std::string BeamerEngine::FigureBody(const Figure& figure) const
{
  size_t nPlots = figure.GetPlots().size();
  if (nPlots == 0) {
    LOG(warn) << "No plots provided for figure " << figure.GetLabel() << ". Ignoring.";
    return "";
  }

  std::stringstream out;
  out << "\\begin{frame}{" << figure.GetMother()->GetTitle() << "}\n";
  out << "  \\begin{figure}\n";
  out << "    \\def\\hgt{\\textheight-2\\baselineskip}\n";
  out << "    \\centering\n";
  if (nPlots == 1) {
    out << "    \\begin{adjustbox}{width=\\textwidth, totalheight=0.9\\hgt, keepaspectratio}\n";
    out << "      \\includegraphics[height=0.75\\paperheight]{" << figure.GetPlots()[0].fsPath << "}\n";
    out << "    \\end{adjustbox}\n";
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
      if (nPlots < 4) {
        vPlotGrid.resize(1, nPlots);
      }
      else {
        int nY = nPlots < 13 ? 2 : 3;
        vPlotGrid.resize(nY, size_t(std::ceil(double(nPlots) / nY)));
      }
    }
    else {
      vPlotGrid = figure.GetPlotGrid();
    }

    size_t iPlot = 0;
    double hSize = 0.9 / vPlotGrid.size();
    for (size_t iY = 0; iY < vPlotGrid.size(); ++iY) {
      if (iPlot >= nPlots) {
        break;
      }
      size_t nX = vPlotGrid[iY];
      if (nX == 0) {
        continue;
      }
      out << "    \\begin{adjustbox}{width=\\textwidth, totalheight=" << hSize << "\\hgt, keepaspectratio}\n";
      for (size_t iX = 0; iX < nX; ++iX) {
        if (iPlot >= nPlots) {
          break;
        }
        const auto& plot = figure.GetPlots()[iPlot];
        out << "      \\stackunder[1pt]{\\includegraphics[height=0.75\\paperheight]{" << plot.fsPath << "}}{";
        out << plot.fsCaption << "}\n";
        if (iX == nX - 1) {
          out << '\n';
        }
        else {
          out << "      \\quad\n";
        }
        ++iPlot;
      }
      out << "    \\end{adjustbox}\n\n";
      if (iY != vPlotGrid.size() - 1) {
        out << '\n';
      }
    }
  }

  if (!figure.GetCaption().empty()) {
    out << "    \\caption{" << LatexFormat::Apply(figure.GetCaption()) << "}\n";
  }
  out << "    \\label{" << figure.GetLabel() << "}\n";
  out << "  \\end{figure}\n";
  out << "\\end{frame}\n";
  return out.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string BeamerEngine::HeaderBody(const Header& header) const
{
  std::stringstream out;

  // TODO: Move these definitions to text-file (?)
  // Settings

  out << "\\documentclass[aspectratio=169,xcolor=dvipsnames]{beamer}\n";
  out << "\\usetheme{Madrid}\n";
  out << "\\useinnertheme{circles}\n";
  out << "\\setbeamerfont{structure}{family=\\sffamily,series=\\mdseries}\n";
  out << "\\setbeamerfont{title}{size=\\LARGE,parent=structure}\n";
  out << "\\setbeamerfont{subtitle}{size=\\normalsize,parent=title}\n";
  out << "\\setbeamerfont{date}{size=\\scriptsize,series=\\mdseries,parent=structure}\n";
  out << "\\setbeamerfont{author}{size=\\Large,series=\\mdseries,parent=structure}\n";
  out << "\\setbeamerfont{institute}{size=\\scriptsize,series=\\mdseries,parent=structure}\n";
  out << "\\setbeamerfont{section in toc}{size=\\Large,parent=structure}\n";
  out << "\\setbeamerfont{section in head/foot}{size=\\tiny,parent=structure}\n";
  out << "\\setbeamerfont{subsection in toc}{size=\\large,parent={section in toc}}\n";
  out << "\\setbeamerfont{frametitle}{parent=structure,size=\\LARGE}\n";
  out << "\\setbeamerfont{framesubtitle}{parent=frametitle,size=\\Large}\n";
  out << "\\setbeamerfont{caption}{size=\\footnotesize}\n";
  out << "\\setbeamerfont{item}{parent=structure,series=\\mdseries}\n";
  out << "\\setbeamerfont{block title}{size=\\large,series=\\mdseries,parent={structure,block body}}\n";
  out << "\\definecolor{InvisibleRed}{rgb}{0.92, 0.9, 0.9}\n";
  out << "\\definecolor{InvisibleGreen}{rgb}{0.9, 0.92, 0.9}\n";
  out << "\\definecolor{InvisibleBlue}{rgb}{0.9, 0.9, 0.92}\n";
  out << "\\definecolor{LightBlue}{rgb}{0.4, 0.55, 0.65}\n";
  out << "\\definecolor{LightBlue}{rgb}{0.4, 0.55, 0.65}\n";
  out << "\\definecolor{MediumRed}{rgb}{0.92549, 0.34509, 0.34509}\n";
  out << "\\definecolor{MediumGreen}{rgb}{0.36862, 0.66666, 0.65882}\n";
  out << "\\definecolor{MediumBlue}{rgb}{0.01176, 0.31372, 0.43529}\n";
  out << "\\definecolor{DarkBlue}{rgb}{0.05, 0.15, 0.3} \n";
  out << "\\usecolortheme[named=DarkBlue]{structure}\n";
  out << "\\setbeamercolor{alerted text}{fg=LightBlue}\n";
  out << "\\setbeamercolor{palette primary}{bg=DarkBlue,fg=white}\n";
  out << "\\setbeamercolor{palette secondary}{bg=MediumBlue,fg=white}\n";
  out << "\\setbeamercolor{palette tertiary}{bg=LightBlue,fg=white}\n";
  out << "\\setbeamercolor{block title}{bg=MediumBlue}\n";
  out << "\\setbeamercolor{block body}{bg=InvisibleBlue}\n";
  out << "\\setbeamercolor{block title example}{bg=MediumGreen}\n";
  out << "\\setbeamercolor{block body example}{bg=InvisibleGreen}\n";
  out << "\\setbeamercolor{block title alerted}{bg=MediumRed}\n";
  out << "\\setbeamercolor{block body alerted}{bg=InvisibleRed}\n";
  out << "\\setbeamertemplate{footline}[page number]\n";
  out << "\\setbeamertemplate{navigation symbols}{}\n";
  out << "\\setbeamertemplate{blocks}[rounded][shadow=true]\n";
  out << "\\setbeamertemplate{enumerate items}[default]\n";
  out << "\\setbeamertemplate{section in toc}[sections numbered]\n";
  out << "\\setbeamertemplate{subsection in toc}[default]\n";

  out << "\\usepackage{hyperref}\n";
  out << "\\usepackage{graphicx}\n";
  out << "\\usepackage{booktabs}\n";
  out << "\\usepackage{listings}\n";
  out << "\\usepackage{amsmath}\n";
  out << "\\usepackage{listings}\n";
  out << "\\usepackage{xcolor}\n";
  //out << "\\usepackage{adjustbox}\n";
  out << "\\usepackage{subfig}\n";
  out << "\\usepackage{hyperref}\n";
  out << "\\usepackage{stackengine}\n";
  out << "\\usepackage{longtable}\n";
  out << "\\usepackage[export]{adjustbox}\n";

  out << "\\setbeamertemplate{caption}[numbered]\n";

  out << "\\AtBeginSection[]{\n";
  out << "  \\begin{frame}\n";
  out << "  \\vfill\n";
  out << "  \\begin{beamercolorbox}[sep=8pt,shadow=true,rounded=true]{title}\n";
  out << "    \\usebeamerfont{title}Section: \\insertsectionhead\\par%\n";
  out << "  \\end{beamercolorbox}\n";
  out << "  \\tableofcontents[currentsection]\n";
  out << "  \\vfill\n";
  out << "  \\end{frame}\n";
  out << "}\n";
  out << "\n";
  out << "\\AtBeginSubsection[]{\n";
  out << "  \\begin{frame}\n";
  out << "  \\vfill\n";
  out << "    \\begin{beamercolorbox}[sep=6pt,shadow=true,rounded=true]{title}\n";
  out << "      \\usebeamerfont{title}Subsection: \\insertsubsectionhead\\par%\n";
  out << "    \\end{beamercolorbox}\n";
  out << "    \\tableofcontents[currentsubsection]\n";
  out << "  \\vfill\n";
  out << "  \\end{frame}\n";
  out << "}\n";

  //out << "\\usepackage{color}\n";
  //out << "\\definecolor{hrefcolor}{rgb}{0.2, 0.2, 0.6}\n";
  //out << "\\hypersetup{color}{colorlinks=true, urlcolor=hrefcolor,linkcolor=black,citecolor=hrefcolor}\n";
  if (!header.GetPageHeader().empty()) {
    //out << "\\usepackage{fancyhdr}\n\\pagestyle{fancy}\n\\lhead{"
    //    << LatexFormat::Apply(header.GetPageHeader()) << "}\n";
  }
  out << "\n";
  // Title page settings
  out << "\\title{" << LatexFormat::Apply(header.GetTitle()) << "}\n";
  out << "\\subtitle{" << LatexFormat::Apply(header.GetSubtitle()) << "}\n";
  auto timeNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  out << "\\date{Generated on " << std::put_time(std::localtime(&timeNow), "%a %d.%m.%Y, %X") << "}\n";


  // Title page
  out << "\\begin{document}\n\n";
  out << "\\begin{frame}\n";
  out << "  \\titlepage\n";
  out << "  \\begin{tabular}{r@{ }l}\n";
  out << "     author: & " << header.GetAuthor() << " \\\\\n";
  out << "     setup: & " << LatexFormat::Apply(header.GetSetup()) << "\\\\\n";
  out << "     tags: & " << boost::algorithm::join(header.GetTags(), ", ") << '\n';
  out << "  \\end{tabular}\n";
  out << "\\end{frame}\n";

  // Table of contents
  out << "\\begin{frame}{Outline}\n";
  out << "  \\tableofcontents\n";
  out << "\\end{frame}\n";

  out << '\n';
  return out.str();
}


// ---------------------------------------------------------------------------------------------------------------------
//
std::string BeamerEngine::SectionBody(const Section& section) const
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

  out << "\\" << sSectionType << "{" << section.GetTitle() << "}\n\n";
  for (const auto& pElement : section.GetDaughterElements()) {
    out << pElement->GetBody(*this) << '\n';
  }
  out << '\n';
  return out.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string BeamerEngine::TableBody(const Table& table) const
{
  std::stringstream out;
  int nRows = table.GetNofRows();
  int nCols = table.GetNofCols();

  out << "\\begin{frame}{" << table.GetMother()->GetTitle() << "}\n";
  out << "  \\begin{table}\n";
  out << "  \\footnotesize\n";
  // tabular header
  if (!table.GetCaption().empty()) {
    out << "    \\caption{" << LatexFormat::Apply(table.GetCaption()) << "}\n";
  }
  out << "    \\begin{longtable}{" << std::string(nCols, 'c') << "}\n";
  // table header
  out << "      \\hline\n";
  out << "    ";
  for (int iCol = 0; iCol < nCols; ++iCol) {
    out << table.GetColumnTitle(iCol) << ((iCol < nCols - 1) ? " & " : " \\\\\n");
  }
  out << "      \\hline\n";
  // table body
  for (int iRow = 0; iRow < nRows; ++iRow) {
    out << "    ";
    for (int iCol = 0; iCol < nCols; ++iCol) {
      out << table(iRow, iCol) << ((iCol < nCols - 1) ? " & " : " \\\\\n");
    }
  }
  out << "      \\hline\n";

  out << "    \\end{longtable}\n";
  out << "    \\label{" << table.GetLabel() << "}\n";
  out << "  \\end{table}\n";
  out << "\\end{frame}\n";
  return out.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string BeamerEngine::TailBody(const Tail& figure) const
{
  std::stringstream out;
  out << "\\end{document}\n";
  return out.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void BeamerEngine::Compile(const std::string& source) const
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
    LOG(error) << "cbm::qa::report::BeamerEngine::Compile(): compiler \"" << fsLatexCompiler << "\" not found in PATH. "
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

  LOG(info) << "cbm::qa::report::BeamerEngine::Compile(): executing command: \n\t" << command;
  int res = std::system(command.c_str());
  fs::current_path(currPath);

  LOG(info) << "cbm::qa::report::BemaerEngine::Compile(): compillation " << (res == 0 ? "succeed" : "failed")
            << "(compiler log: \"" << logFile << "\")";
}
