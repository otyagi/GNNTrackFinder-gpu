/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportTable.cxx
/// \brief  Base class for the report table (source)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  23.02.2024

#include "CbmQaReportTable.h"

#include "CbmQaTable.h"
#include "Logger.h"

using cbm::qa::report::Table;

#include <algorithm>
#include <sstream>

// ---------------------------------------------------------------------------------------------------------------------
//
Table::Table(std::string_view label, std::string_view title) : Table(label, 0, 0, title) {}

// ---------------------------------------------------------------------------------------------------------------------
//
Table::Table(std::string_view label, int nRows, int nCols, std::string_view title)
  : Element(std::string("tab:") + std::string(label), title)
  , fNofRows(nRows)
  , fNofCols(nCols)
{
  fvsTable.clear();
  fvsTable.resize(fNofRows * fNofCols);
  fvsTableHeader.clear();
  fvsTableHeader.resize(fNofCols);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Table::Set(const CbmQaTable* pQaTable, const std::string& cellFormat)
{
  if (!pQaTable) {
    LOG(fatal) << "cbm::qa::report::Table::Set(): A nullptr QA-table is passed to the function. The table label is \""
               << GetLabel() << '\"';
  }

  // Define table size
  fvsTable.clear();
  fvsTableHeader.clear();
  int nRowsQa = pQaTable->GetNrows();
  int nColsQa = pQaTable->GetNcols();
  fsCaption   = pQaTable->GetTitle();

  bool bRowsHaveTitles = false;
  for (int iRow = 0; iRow < nRowsQa; ++iRow) {
    if (!std::string(pQaTable->GetXaxis()->GetBinLabel(nRowsQa - iRow)).empty()) {
      bRowsHaveTitles = true;
    }
  }
  fNofRows = nRowsQa;
  fNofCols = nColsQa + static_cast<int>(bRowsHaveTitles);  // Extra column comes from row names

  // Parse row format
  // NOTE: If the sequence of formats was passed, and it is shorter, than number of columns, then the last
  //       columns will be formated with default format, which is {:.3}.
  bool bCommonRowFormat = cellFormat.find_first_of('|') == std::string::npos;
  std::vector<std::string> vCellFormat(nColsQa, bCommonRowFormat ? cellFormat : std::string(kDefaultFormat));
  if (!bCommonRowFormat) {
    std::stringstream stream(cellFormat);
    int iCol = 0;
    while (iCol < nColsQa && std::getline(stream, vCellFormat[iCol], '|')) {
      if (vCellFormat.empty()) {
        vCellFormat[iCol] = "{}";
      }
      ++iCol;
    }
  }

  // Test formatting:
  for (auto& entry : vCellFormat) {
    try {
      std::string sTest = fmt::format(entry, 0.);
    }
    catch (const std::runtime_error& err) {
      LOG(error) << "cbm::qa::report::Table(): Inacceptable format \"" << entry << "\" is passed, using default";
      entry = kDefaultFormat;
    }
  }

  // Filling the table
  fvsTable.resize(fNofCols * fNofRows);
  fvsTableHeader.resize(fNofCols);
  for (int iCol = static_cast<int>(bRowsHaveTitles); iCol < fNofCols; ++iCol) {
    this->SetColumnTitle(iCol, std::string(pQaTable->GetXaxis()->GetBinLabel(iCol)));
  }
  for (int iRow = 0; iRow < fNofRows; ++iRow) {
    // Print row title
    if (bRowsHaveTitles) {
      this->SetCell(iRow, 0, pQaTable->GetYaxis()->GetBinLabel(fNofRows - iRow));
    }
    for (int iColQa = 0; iColQa < nColsQa; ++iColQa) {
      double entry = pQaTable->GetCell(iRow, iColQa);
      int iCol     = iColQa + static_cast<int>(bRowsHaveTitles);
      this->SetCell(iRow, iCol, fmt::format(vCellFormat[iColQa], entry));
    }
  }
}
