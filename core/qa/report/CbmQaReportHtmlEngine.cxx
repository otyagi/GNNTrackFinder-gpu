/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportHtmlEngine.h
/// \brief  HTML document engine for the cbm::qa::report (source)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  23.02.2024

#include "CbmQaReportHtmlEngine.h"

#include "CbmQaReportFigure.h"
#include "CbmQaReportHeader.h"
#include "CbmQaReportSection.h"
#include "CbmQaReportTable.h"
#include "CbmQaReportTail.h"

using cbm::qa::report::Figure;
using cbm::qa::report::Header;
using cbm::qa::report::HtmlEngine;
using cbm::qa::report::Section;
using cbm::qa::report::Table;
using cbm::qa::report::Tail;

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
//#include <regex>

#include <fmt/format.h>

// ---------------------------------------------------------------------------------------------------------------------
//
std::string HtmlEngine::FigureBody(const Figure& figure) const
{
  std::stringstream out;
  out << "<figure>\n";
  for (const auto& plot : figure.GetPlots()) {
    out << "  <embed src=\"" << plot.fsPath << "\" style=\"width:" << kFigureWidth << "\">\n";
  }
  if (!figure.GetCaption().empty()) {
    out << "  <figcaption>Fig.[" << figure.GetLabel() << "]: " << figure.GetCaption() << "</figcaption>\n";
  }
  out << "</figure>\n";

  return out.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string HtmlEngine::HeaderBody(const Header& header) const
{
  std::stringstream out;

  // TODO: Move these definitions to text-file (?)
  // Settings
  out << "<!DOCTYPE html>\n";
  out << "<html>\n";
  out << "<style>\n";
  out << "table {\n  boarder-collapse: collapse;\n  width: 100%;\n"
      << "}\n";
  out << "th, td {\n  text-align: " << kTableTextAlign << ";\n  padding: " << kTablePadding << "px;\n"
      << "}\n";
  out << "tr:nth-child(even) {\n  background-color: #EEEEEE;\n}\n";
  out << "</style>\n";
  out << "<head>\n";
  out << "  <title>" << header.GetTitle() << "</title>\n";
  out << "</head>\n\n";
  out << "<body>\n";
  out << "<h1>" << header.GetTitle() << "</h1>\n\n";
  if (!header.GetSubtitle().empty()) {
    out << "<h2>" << header.GetSubtitle() << "</h2>\n\n";
  }
  auto timeNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  out << "<table style=\"width:1\">\n";
  out << "  <tr>\n";
  out << "    <td>Generated on:</td>\n";
  out << "    <td>" << std::put_time(std::localtime(&timeNow), "%a %d.%m.%Y, %X") << "</td>\n";
  out << "  </tr>\n";
  out << "  <tr>\n";
  out << "    <td>Author:</td>\n";
  out << "    <td>" << header.GetAuthor() << "</td>\n";
  out << "  </tr>\n";
  if (!header.GetSetup().empty()) {
    out << "  <tr>\n";
    out << "    <td>Setup:</td>\n";
    out << "    <td>" << header.GetSetup() << "</td>\n";
    out << "  </tr>\n";
  }
  out << "</table>\n\n";

  return out.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string HtmlEngine::SectionBody(const Section& section) const
{
  std::stringstream out;
  std::string sSectionTag = fmt::format("h{}", std::min((section.GetLevel() + 2), 6));

  out << '<' << sSectionTag << '>' << section.GetTitle() << "</" << sSectionTag << ">\n\n";
  for (const auto& pElement : section.GetDaughterElements()) {
    out << pElement->GetBody(*this) << '\n';
  }
  out << '\n';
  return out.str();
}


// ---------------------------------------------------------------------------------------------------------------------
//
std::string HtmlEngine::TableBody(const Table& table) const
{
  std::stringstream out;
  int nRows = table.GetNofRows();
  int nCols = table.GetNofCols();

  out << "<table>\n";
  // tabular header
  if (!table.GetCaption().empty()) {
    out << "  <tablecaption>Table: " << table.GetCaption() << "</tablecaption>\n";
  }
  // table header
  out << "  <tr>\n";
  for (int iCol = 0; iCol < nCols; ++iCol) {
    out << "    <th>" << table.GetColumnTitle(iCol) << "</th>\n";
  }
  out << "  </tr>\n";
  // table body
  for (int iRow = 0; iRow < nRows; ++iRow) {
    out << "  <tr>\n";
    for (int iCol = 0; iCol < nCols; ++iCol) {
      out << "    <td>" << table(iRow, iCol) << "</td>\n";
    }
    out << "  </tr>\n";
  }
  out << "</table>\n";
  return out.str();
}


// ---------------------------------------------------------------------------------------------------------------------
//
std::string HtmlEngine::TailBody(const Tail&) const
{
  std::stringstream out;
  out << '\n';
  out << "</body>\n";
  out << "</html>\n";
  return out.str();
}
