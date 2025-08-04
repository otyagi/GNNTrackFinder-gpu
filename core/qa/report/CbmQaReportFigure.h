/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportFigure.h
/// \brief  Base class for the report figure (header)
/// \author S. Zharko <s.zharko@gsi.de>
/// \since  21.02.2024

#pragma once

#include "CbmQaReportElement.h"
#include "CbmQaReportEngine.h"

namespace cbm::qa::report
{
  /// \class Figure
  /// \brief Figure in the report
  class Figure : public Element {
   public:
    /// \struct Plot
    /// \brief  A structure to handle the plot details
    struct Plot {
      Plot() = default;

      /// \brief Constructor
      Plot(std::string_view path, std::string_view caption, std::string_view label)
        : fsPath(path)
        , fsCaption(caption)
        , fsLabel(label)
      {
      }

      std::string fsPath    = "";
      std::string fsCaption = "";
      std::string fsLabel   = "";
    };

    /// \brief Constructor
    /// \param label  Label of the element
    /// \param title  Title
    Figure(std::string_view label, std::string_view title = "")
      : Element(std::string("fig:") + std::string(label), title)
    {
    }

    /// \brief Destructor
    virtual ~Figure() = default;

    /// \brief Add plot
    /// \param path to the plot
    /// \param caption caption of the plot
    /// \param label label of the plot
    void AddPlot(std::string_view path, std::string_view caption = "", std::string_view label = "")
    {
      fvsPlots.emplace_back(path, caption, label);
    }

    /// \brief Gets body of the element
    /// \param engine  A concrete implementation of the Engine to get the element body
    std::string GetBody(const Engine& engine) const override { return engine.FigureBody(*this); }

    /// \brief Gets caption
    const std::string& GetCaption() const { return fsCaption; }

    /// \brief Gets plot grid
    const std::vector<size_t>& GetPlotGrid() const { return fvGrid; }

    /// \brief Gets plot vector
    const std::vector<Plot>& GetPlots() const { return fvsPlots; }

    /// \brief Sets caption
    void SetCaption(std::string_view caption) { fsCaption = caption; }

    /// \brief Sets plot grid
    /// \param nX  Number of plots along the horizontal direction
    /// \param nY  Number of plots along the vertical direction
    void SetPlotGrid(size_t nX, size_t nY)
    {
      fvGrid.clear();
      fvGrid.resize(nY, nX);
    }

    /// \brief Set plot grid
    /// \param grid  Grid {X1, X2, ..., Xn}, where Xi -- number of plots in the ith line
    void SetPlotGrid(const std::vector<size_t>& grid) { fvGrid = grid; }

   private:
    std::vector<Plot> fvsPlots{};  ///< Vector of plots (subfigures)
    std::vector<size_t> fvGrid{};  ///< Plot grid
    std::string fsCaption{};       ///< Figure caption
  };
}  // namespace cbm::qa::report
