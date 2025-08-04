/* Copyright (C) 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaInputQaMuch.h
/// \date   13.01.2023
/// \brief  QA-task for CA tracking input from MuCh detector (header)
/// \author S.Zharko <s.zharko@gsi.de>


#pragma once

#include "CbmCaInputQaBase.h"

/// A QA-task class, which provides assurance of TOF hits and geometry
///
class CbmCaInputQaMuch : public CbmCaInputQaBase<cbm::algo::ca::EDetectorID::kMuch> {
 public:
  /// @brief  Constructor from parameters
  /// @param  verbose   Verbose level
  /// @param  isMCUsed  Flag, whether MC information is available for this task
  CbmCaInputQaMuch(int verbose, bool isMCUsed);

 protected:
  /// \brief  Method to check, if the QA results are acceptable
  void Check() override { return CbmCaInputQaBase::Check(); }

  /// \brief Creates summary cavases, tables etc.
  void CreateSummary() override { CbmCaInputQaBase::CreateSummary(); }

  /// \brief Defines parameters of the task
  void DefineParameters() override;

  /// \brief De-initializes QA-task
  void DeInit() override { CbmCaInputQaBase::DeInit(); }

  /// \brief Fills histograms per hit
  void FillHistogramsPerHit() override {}

  /// \brief Fills histograms per MC point
  void FillHistogramsPerPoint() override {}

  /// \brief Fills histograms per event or time-slice
  void ExecQa() override { CbmCaInputQaBase::ExecQa(); }

  /// \brief Initializes QA
  InitStatus InitQa() override;

 private:
  ClassDefOverride(CbmCaInputQaMuch, 0);
};
