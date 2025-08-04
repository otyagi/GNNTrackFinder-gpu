/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer] */

/// \file   CbmQaUtil.h
/// \brief  Useful utilities for CBM QA tasks
/// \author S.Gorbunov
/// \data   31.07.2023

#ifndef CbmQaUtil_h
#define CbmQaUtil_h 1

#include <tuple>
class TH1;
class TPaveStats;

/// namespace cbm::qa::util contains useful utilities for CBM QA tasks
namespace cbm::qa::util
{

  /// @brief Finds/Creates stats window for a histogram
  /// @param pHist histogram
  /// @return stats window
  TPaveStats* GetHistStats(TH1* pHist);

  /// @brief Fit a histogram with Kaniadakis Gaussian Distribution
  /// @return  mean and std.dev of the fit
  std::tuple<double, double> FitKaniadakisGaussian(TH1* pHist);

  /// @brief Set large stat. window
  ///
  void SetLargeStats(TH1* pHist);

}  // namespace cbm::qa::util

#endif  // CbmQaUtil_h
