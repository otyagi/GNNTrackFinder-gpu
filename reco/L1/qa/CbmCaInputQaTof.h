/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaInputQaTof.h
/// \date   30.01.2023
/// \brief  QA-task for CA tracking input from TOF detector (header)
/// \author S.Zharko <s.zharko@gsi.de>


#pragma once

#include "CbmCaInputQaBase.h"
#include "CbmMCDataManager.h"
#include "CbmQaTask.h"
#include "CbmTofCell.h"
#include "TMath.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

class CbmMCEventList;
class CbmMCDataArray;
class CbmMCDataManager;
class CbmTimeSlice;
class CbmMatch;
class CbmTofHit;
class CbmTofPoint;
class CbmTofTrackingInterface;
class TClonesArray;
class TH1F;
class CbmQaEff;
class CbmTofDigiPar;
class CbmTofDigiBdfPar;


/// A QA-task class, which provides assurance of TOF hits and geometry
///
class CbmCaInputQaTof : public CbmCaInputQaBase<cbm::algo::ca::EDetectorID::kTof> {
 public:
  /// @brief  Constructor from parameters
  /// @param  verbose   Verbose level
  /// @param  isMCUsed  Flag, whether MC information is available for this task
  CbmCaInputQaTof(int verbose, bool isMCUsed);

 protected:
  /// \brief  Method to check, if the QA results are acceptable
  void Check() override { CbmCaInputQaBase::Check(); }

  /// \brief Creates summary cavases, tables etc.
  void CreateSummary() override;

  /// \brief De-initializes histograms
  void DeInit() override;

  /// \brief Defines parameters of the task
  void DefineParameters() override;

  /// \brief Fills histograms per hit
  void FillHistogramsPerHit() override;

  /// \brief Fills histograms per MC point
  void FillHistogramsPerPoint() override {}

  /// \brief Fills histograms per event or time-slice
  void ExecQa() override;

  /// \brief Initializes histograms, data-branches and other modules in the beginning of the run
  InitStatus InitQa() override;

 private:
  /// @brief Fills channel info map
  void FillChannelInfoMap();

  CbmTofDigiPar* fDigiPar       = nullptr;
  CbmTofDigiBdfPar* fDigiBdfPar = nullptr;

  int fNbXo = 400;   ///< Number of bins in occupancy
  int fNbYo = 400;   ///< Number of bins in occupancy
  int fUpXo = +100;  ///< X max in occupancy [cm]
  int fLoXo = -100;  ///< X min in occupancy [cm]
  int fUpYo = +100;  ///< Y max in occupancy [cm]
  int fLoYo = -100;  ///< Y min in occupancy [cm]
  template<typename T>
  using Vector3D_t = std::vector<std::vector<std::vector<T>>>;
  Vector3D_t<TH2F*> fvph_hit_xy_vs_cell;  ///< Hit occupancy vs. TOF cell ix XY-plane
  Vector3D_t<TH2F*> fvph_hit_zx_vs_cell;  ///< Hit occupancy vs. TOF cell in ZX-plane
  Vector3D_t<TH2F*> fvph_hit_zy_vs_cell;  ///< Hit occupancy vs. TOF cell in ZY-plane

  ClassDefOverride(CbmCaInputQaTof, 0);
};
