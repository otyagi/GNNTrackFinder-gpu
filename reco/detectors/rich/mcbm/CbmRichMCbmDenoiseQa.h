/* Copyright (C) 2024 UGiessen, Giessen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Martin Beyer [committer] */

/**
* \file CbmRichMCbmDenoiseQa.h
* \author M.Beyer
* \date 2024
**/

#pragma once

#include "FairTask.h"

class CbmDigiManager;
class CbmRichDigi;
class CbmHistManager;
class CbmEvent;
class TClonesArray;

/**
* \class CbmRichMCbmDenoiseQa
*
* \brief QA for MCbm mRICH noise removal
*
* \author M.Beyer
* \date 2024
**/
class CbmRichMCbmDenoiseQa : public FairTask {
 public:
  /** Default constructor */
  CbmRichMCbmDenoiseQa() : FairTask("CbmRichMCbmDenoiseQa"){};

  /** Destructor */
  ~CbmRichMCbmDenoiseQa() = default;

  /** Copy constructor (disabled) */
  CbmRichMCbmDenoiseQa(const CbmRichMCbmDenoiseQa&) = delete;

  /** Assignment operator (disabled) */
  CbmRichMCbmDenoiseQa operator=(const CbmRichMCbmDenoiseQa&) = delete;

  /** Inherited from FairTask */
  InitStatus Init();

  /** Inherited from FairTask */
  void Exec(Option_t* option);

  /** Inherited from FairTask */
  void Finish();

  /** Initialize histogram manager */
  void InitHistograms();

  /** 
   * \brief Process data and fill histograms
   * \param event if CbmEvent* is nullptr -> process full Ts
   */
  void Process(CbmEvent* event);

  /** Draw a Single-event-display */
  void DrawSED(CbmEvent* event);

  /** Set maximum number of SEDs drawn */
  void SetMaxSEDs(Int_t maxSEDs) { fMaxSEDs = maxSEDs; }

 private:
  Int_t fTsNum{};
  Int_t fEventNum{};
  Int_t fnSEDs{};

  Int_t fMaxSEDs{20};

  std::unique_ptr<CbmHistManager> fHM{nullptr};
  std::unique_ptr<CbmHistManager> fHMSed{nullptr};  // For single event displays

  TClonesArray* fCbmEvents{nullptr};
  TClonesArray* fRichHits{nullptr};
  TClonesArray* fRichRings{nullptr};

  ClassDef(CbmRichMCbmDenoiseQa, 1)
};
