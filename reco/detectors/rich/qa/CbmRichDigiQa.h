/* Copyright (C) 2023-2024 UGiessen, Giessen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Martin Beyer [committer] */

/**
* \file CbmRichDigiQa.h
* \author M.Beyer
* \date 2023
**/

#ifndef CBMRICHDIGIQA_H
#define CBMRICHDIGIQA_H

#include "FairTask.h"

#include <map>
#include <vector>

class CbmDigiManager;
class CbmRichDigi;
class CbmHistManager;
class TClonesArray;

/**
* \class CbmRichDigiQa
*
* \brief Class for pixel deadtime and crosstalk Qa.
*
* \author M.Beyer
* \date 2023
**/
class CbmRichDigiQa : public FairTask {
 public:
  /** Default constructor */
  CbmRichDigiQa() : FairTask("CbmRichDigiQa"){};

  /** Destructor */
  ~CbmRichDigiQa() = default;

  /** Copy constructor (disabled) */
  CbmRichDigiQa(const CbmRichDigiQa&) = delete;

  /** Assignment operator (disabled) */
  CbmRichDigiQa operator=(const CbmRichDigiQa&) = delete;

  /** Inherited from FairTask */
  InitStatus Init();

  /** Inherited from FairTask */
  void Exec(Option_t* option);

  /** Inherited from FairTask */
  void Finish();

  /** Initialize histogram manager */
  void InitHistograms();

  /** 
    * \brief Set ToT cut
    * \param[in] low lower limit
    * \param[in] high upper limit
    */
  void SetToTLimits(Double_t low, Double_t high)
  {
    fToTLimitLow  = low;
    fToTLimitHigh = high;
  }

  /**
    * \brief Set time limit in which a digi is considered neighbour.
    * \param[in] limit absolue time limit
    */
  void SetNeighbourTimeLimit(Double_t limit) { fNeighbourTimeLimit = limit; }

 private:
  int fEventNum{};

  CbmDigiManager* fDigiMan{nullptr};
  CbmHistManager* fHM{nullptr};

  TClonesArray* fRichPoints{nullptr};

  std::map<Int_t, std::vector<Double_t>> fFiredTimes{};  // Store times of fired digis for each pixel in Ts
  std::map<Int_t, std::vector<std::pair<Double_t, Int_t>>>
    fPmtDigisTimeAddress{};  // Store times and addresses of fired digis for each pmt in Ts

  Double_t fNeighbourTimeLimit{5.};  // Time in which digi is considered a neighbour
  Double_t fToTLimitLow{-1.};        // Lower limit for ToT cut
  Double_t fToTLimitHigh{-1.};       // Upper limit for ToT cut

  ClassDef(CbmRichDigiQa, 1)
};

#endif
