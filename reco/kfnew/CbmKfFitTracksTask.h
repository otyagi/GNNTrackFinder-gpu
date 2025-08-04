/* Copyright (C) 2023-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: S.Gorbunov[committer] */

/// @file    CbmKfFitTracksTask.h
/// @author  Sergey Gorbunov
/// @date    15.11.2023
/// @brief   Task class for refitting global or sts tracks


#pragma once

#include "CbmKfTrackFitter.h"
#include "FairTask.h"

class TClonesArray;

/// @class CbmKfFitTracksTask
/// @brief Task class for refitting global or sts tracks
///
class CbmKfFitTracksTask : public FairTask {
 public:
  enum FitMode
  {
    kSts,
    kMcbm,
    kGlobal
  };

  // Constructors/Destructors ---------
  CbmKfFitTracksTask(FitMode mode = FitMode::kSts, Int_t iVerbose = 0);

  const CbmKfFitTracksTask& operator=(const CbmKfFitTracksTask&) = delete;
  CbmKfFitTracksTask(const CbmKfFitTracksTask&)                  = delete;

  virtual ~CbmKfFitTracksTask();

  InitStatus Init() override;
  void Exec(Option_t* opt) override;
  void Finish() override;

  void SetFitGlobalTracks() { fFitMode = FitMode::kGlobal; }
  void SetFitStsTracks() { fFitMode = FitMode::kSts; }
  void SetFitMcbmTracks() { fFitMode = FitMode::kMcbm; }

 private:
  FitMode fFitMode{FitMode::kGlobal};  ///> fit mode

  /// input data arrays

  TClonesArray* fGlobalTracks{nullptr};  ///< global tracks
  TClonesArray* fStsTracks{nullptr};     ///< sts tracks
  TClonesArray* fMuchTracks{nullptr};    ///< much tracks
  TClonesArray* fTrdTracks{nullptr};     ///< trd tracks
  TClonesArray* fTofTracks{nullptr};     ///< tof tracks

  CbmKfTrackFitter fFitter;  ///< track fitter

  Int_t fNeventsProcessed{0};  ///< number of processed events

  // ClassDefOverride(CbmKfFitTracksTask, 0);
};
