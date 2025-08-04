/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmQaCheckerHist1DHandler.h
/// @brief  Handler class for 1D-histograms (including TProfile objects) (declaration)
/// @author S. Zharko <s.zharko@gsi.de>
/// @since  09.02.2023

#ifndef CbmQaCheckerHist1DHandler_h
#define CbmQaCheckerHist1DHandler_h 1

#include "CbmQaCheckerObjectHandler.h"

#include <memory>
#include <string>

class TObject;
class TH1;

namespace cbm::qa::checker
{
  /// @brief Handler for 1D-histograms.
  ///
  /// The handler keeps one-dimensional histogram objects of the same quantity within different code versions
  /// and provides several comparison methods including point-by-point comparison, test of ratio deviation and
  /// statistical hypothesis tests (Chi2 test).
  ///
  class Hist1DHandler : public ObjectHandler {
   public:
    /// @brief Constructor
    /// @param iObject  Index of object
    /// @param iFile    Index of file
    /// @param iDataset Index of dataset
    Hist1DHandler(int iObject, int iFile, int iDataset);

    /// @brief Destructor
    ~Hist1DHandler() = default;

    /// @brief Compares objects to default
    /// @param iVersion     Version index
    /// @return  Comparison inference
    ECmpInference Compare(int iVersion) const override;

    /// @brief Creates object comparison canvas
    /// @param opt  Canvas options
    void CreateCanvases(Option_t* opt = "") override;
  };
}  // namespace cbm::qa::checker

#endif  // CbmQaCheckerHist1DHandler_h
