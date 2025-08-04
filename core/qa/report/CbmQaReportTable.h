/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportTable.h
/// \brief  Base class for the report table (header)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  21.02.2024

#pragma once

#include "CbmQaReportElement.h"
#include "CbmQaReportEngine.h"

class CbmQaTable;

namespace cbm::qa::report
{
  /// \class Table
  /// \brief Table element in the report
  class Table : public Element {
   private:
    static constexpr std::string_view kDefaultFormat = "{:.3}";  ///< Default format of double entries

   public:
    /// \brief Constructor
    /// \param label  Label of the element
    /// \param title  Title
    explicit Table(std::string_view label, std::string_view title = "");

    /// \brief Constructor
    /// \param label  Label of the element
    /// \param nRows  Number of rows
    /// \param nCols  Number of columns
    /// \param title  Title
    Table(std::string_view label, int nRows, int nCols, std::string_view title = "");

    /// \brief Destructor
    virtual ~Table(){};

    /// \brief Cell access operator
    /// \param iRow  Index of row
    /// \param iCol  Index of column
    const std::string& operator()(int iRow, int iCol) const { return fvsTable[GetIndex(iRow, iCol)]; }

    /// \brief Cell access operator
    /// \param iRow  Index of row
    /// \param iCol  Index of column
    std::string& operator()(int iRow, int iCol) { return fvsTable[GetIndex(iRow, iCol)]; }

    /// \brief Gets body of the element
    /// \param engine  A concrete implementation of the Engine to get the element body
    std::string GetBody(const Engine& engine) const override { return engine.TableBody(*this); }

    /// \brief Gets caption
    const std::string& GetCaption() const { return fsCaption; }

    /// \brief Gets cell
    /// \param iRow  Index of row
    /// \param iCol  Index of column
    const std::string& GetCell(int iRow, int iCol) const { return fvsTable[GetIndex(iRow, iCol)]; }

    /// \brief Gets column title
    /// \param iCol  Index of column
    const std::string& GetColumnTitle(int iCol) const { return fvsTableHeader[iCol]; }

    /// \brief Gets number of rows
    int GetNofRows() const { return fNofRows; }

    /// \brief Gets number of columns
    int GetNofCols() const { return fNofCols; }

    /// \brief Sets caption
    /// \param caption Table caption
    void SetCaption(std::string_view caption) { fsCaption = caption; }

    /// \brief Sets a table from CbmQaTable
    /// \param pQaTable   Pointer to the QA-table object
    /// \param cellFormat Defines format of the rows in fmt::format formating scheme
    ///
    /// The parameter format provides ether a common format for all cells of the CbmQaTabl, or individual
    /// formats for each  column (excluding the first row, if it is defined with the QA-table row names).
    /// For latter one have to pass a sequence of the formats, separated by the bar symbol:
    ///    "<format_0>|<format_1>|<...>|<format_n>".
    /// Each of the <format_i> entries represents a fmt::format formatting line. For example, "{:.2}"
    /// formats a double into two digits after floating point, "{:.2e}" converts the number in
    /// the scientific format.
    void Set(const CbmQaTable* pQaTable, const std::string& cellFormat = std::string(kDefaultFormat));

    /// \brief Sets cell
    /// \param iRow  Index of row
    /// \param iCol  Index of column
    /// \param cell  Cell
    void SetCell(int iRow, int iCol, std::string_view cell) { fvsTable[GetIndex(iRow, iCol)] = cell; }

    /// \brief Sets column title
    /// \param iCol  Index of column
    void SetColumnTitle(int iCol, std::string_view title) { fvsTableHeader[iCol] = title; };

   protected:
    /// \brief Gets index of cell in the table vector
    inline int GetIndex(int iRow, int iCol) const { return iRow * fNofCols + iCol; }

   private:
    std::vector<std::string> fvsTable;
    std::vector<std::string> fvsTableHeader;
    std::string fsCaption = "";
    int fNofRows          = 0;
    int fNofCols          = 0;
  };
}  // namespace cbm::qa::report
