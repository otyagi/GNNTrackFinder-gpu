/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaCompare.h
/// \brief  A histogram comparison module for the QA task (declaration)
/// \author S.Zharko <s.zharko@lx-pool.gsi.de>
/// \since  23.12.2023

#pragma once

#include <cassert>
#include <string>

class TH1;
class TCanvas;

/// \class  CbmQaCompare
/// \brief  Class to compare histograms of the QA task with default ones
/// \tparam Obj Root object type
template<class Obj>
class CbmQaCompare {
 public:
  struct Result {
    bool fPointByPoint = true;
    double fRatioLo    = 1.;    ///< Lower bound of the ratio
    double fRatioUp    = 1.;    ///< Upper bound of the ratio
    double fChi2NDF    = 0.;    ///< Chi2/NDF value
    bool fConsistent   = true;  ///< Consistency flag
  };

  /// \brief Constructor
  /// \param pHistL   Left object
  /// \param pHistR   Right object
  /// \param verbose  Verbosity level
  CbmQaCompare(const Obj* pHistL, const Obj* pHistR, int verbose);

  /// \brief Destructor
  ~CbmQaCompare();

  /// \brief  Compares two histograms, returns a comparison status
  /// \param  opt Comparison options
  ///              - p: point-by-point
  ///              - r: ratio
  ///              - s[opt]: stat-test
  /// \param  optStat  Option for the stat. test (see TH1 documentation)
  /// \return Comparison results. By default the result is true
  Result operator()(const std::string& opt, const std::string& optStat = "UU") const;

  /// \brief  Creates a comparison canvas
  /// \param  opt Canvas options
  ///              - d: draw difference
  ///              - r: draw ratio
  TCanvas* GetComparisonCanvas(const std::string& opt);

  /// \brief Set version lables
  /// \param labelL  Left object label
  /// \param labelR  Right object label
  void SetObjectLabels(const std::string& labelL, const std::string& labelR);

 private:
  const Obj* fpObjL    = nullptr;  ///< Left (new) histogram
  const Obj* fpObjR    = nullptr;  ///< Right (default) histogram
  std::string fsLabelL = "this";
  std::string fsLabelR = "default";
  int fVerbose         = 0;  ///< Verbosity
};
