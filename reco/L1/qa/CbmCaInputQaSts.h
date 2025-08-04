/* Copyright (C) 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaInputQaSts.h
/// \date   13.01.2023
/// \brief  QA-task for CA tracking input from MuCh detector (header)
/// \author S.Zharko <s.zharko@gsi.de>


#pragma once

#include "CbmCaInputQaBase.h"
#include "CbmMCDataManager.h"
#include "CbmQaTask.h"
#include "TMath.h"

#include <set>
#include <unordered_map>
#include <vector>

class CbmMatch;
class CbmMCEventList;
class CbmMCDataArray;
class CbmMCDataManager;
class CbmMCTrack;
class CbmStsHit;
class CbmStsPoint;
class CbmStsTrackingInterface;
class CbmTimeSlice;
class TClonesArray;
class TH1F;
class TH2F;
class TProfile;
class TProfile2D;

/// A QA-task class, which provides assurance of MuCh hits and geometry
class CbmCaInputQaSts : public CbmCaInputQaBase<cbm::algo::ca::EDetectorID::kSts> {
 public:
  /// Constructor from parameters
  /// \param  verbose   Verbose level
  /// \param  isMCUsed  Flag, whether MC information is available for this task
  CbmCaInputQaSts(int verbose, bool isMCUsed);

 protected:
  /// \brief  Method to check, if the QA results are acceptable
  void Check() override;

  /// \brief Creates summary cavases, tables etc.
  void CreateSummary() override;

  /// \brief Defines parameters of the task
  void DefineParameters() override;

  /// De-initializes histograms
  void DeInit() override;

  /// \brief Fills histograms per hit
  void FillHistogramsPerHit() override;

  /// \brief Fills histograms per MC point
  void FillHistogramsPerPoint() override;

  /// \brief Fills histograms per event or time-slice
  void ExecQa() override { CbmCaInputQaBase::ExecQa(); }

  /// \brief Initializes QA
  InitStatus InitQa() override;

 private:
  // ----- Data branches
  TClonesArray* fpClusters = nullptr;  ///< Array of hit clusters

  // ----- Histograms (additional to ones from the base class)
  // NOTE: the last element of each vector stands for integral distribution over all stations
  const int fkMaxDigisInClusterForPulls{5};  ///< max digis in cluster for separate histogramming of puls

  std::vector<TH1F*> fvph_pull_u_Ndig;  ///< pull for u coordinate, depending on N digis in the cluster
  std::vector<TH1F*> fvph_pull_v_Ndig;  ///< pull for v coordinate, depending on N digis in the cluster

  ClassDefOverride(CbmCaInputQaSts, 0);
};
