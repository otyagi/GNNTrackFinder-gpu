/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */


#ifndef CBMMCBM2024CHECKBMONSCVD_H
#define CBMMCBM2024CHECKBMONSCVD_H 1


#include "CbmDefs.h"
#include "CbmDigiEvent.h"
#include "CbmEvent.h"

#include <FairTask.h>

#include <gsl/span>
#include <vector>

class TCanvas;
class TH1;
class TH2;

/** @class CbmMcbm2024CheckBmonScvd
 ** @brief Create and fills plots of time differences to trigger in DigiEvents
 **
 ** Creates for each detector system two histograms, one for the time difference of each digi in DigiEvents to the event
 ** seed time and another for the same quantity as function of TS index (~time evolution).
 ** Fills these plots for each DigiEvent.
 **/
class CbmMcbm2024CheckBmonScvd : public FairTask {


 public:
  /** @brief Constructor **/
  CbmMcbm2024CheckBmonScvd();


  /** @brief Copy constructor (disabled) **/
  CbmMcbm2024CheckBmonScvd(const CbmMcbm2024CheckBmonScvd&) = delete;


  /** @brief Destructor **/
  virtual ~CbmMcbm2024CheckBmonScvd();


  /** @brief Task execution **/
  virtual void Exec(Option_t* opt);


  /** @brief Finish timeslice **/
  virtual void Finish();


  /** @brief Assignment operator (disabled) **/
  CbmMcbm2024CheckBmonScvd& operator=(const CbmMcbm2024CheckBmonScvd&) = delete;


 private:  // methods
  /** @brief Task initialisation **/
  virtual InitStatus Init();

 private:                                              // members
  const std::vector<CbmDigiEvent>* fEvents = nullptr;  //! Input data (events)
  size_t fNumTs                            = 0;        ///< Number of processed timeslices
  size_t fNumEvents                        = 0;        ///< Number of events

  TH1* fHistMapBmonOld     = nullptr;
  TH2* fHistMapBmonScvd    = nullptr;
  TH2* fHistMapEvoBmonOld  = nullptr;
  TH2* fHistMapEvoBmonScvd = nullptr;

  TH1* fHistDtBmon    = nullptr;
  TH2* fHistDtEvoBmon = nullptr;
  TH2* fHistDtDxBmon  = nullptr;
  TH2* fHistDxCorBmon = nullptr;

  TCanvas* fCanvMap  = nullptr;
  TCanvas* fCanvCorr = nullptr;

  ClassDef(CbmMcbm2024CheckBmonScvd, 1);
};

#endif /* CBMMCBM2024CHECKBMONSCVD_H */
