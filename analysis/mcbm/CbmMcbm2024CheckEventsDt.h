/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */


#ifndef CBMMCBM2024CHECKEVENTSDT_H
#define CBMMCBM2024CHECKEVENTSDT_H 1


#include "CbmDefs.h"
#include "CbmDigiEvent.h"
#include "CbmEvent.h"

#include <FairTask.h>

#include <gsl/span>
#include <vector>

class TCanvas;
class TH1;
class TH2;

/** @class CbmMcbm2024CheckEventsDt
 ** @brief Create and fills plots of time differences to trigger in DigiEvents
 **
 ** Creates for each detector system two histograms, one for the time difference of each digi in DigiEvents to the event
 ** seed time and another for the same quantity as function of TS index (~time evolution).
 ** Fills these plots for each DigiEvent.
 **/
class CbmMcbm2024CheckEventsDt : public FairTask {


 public:
  /** @brief Constructor **/
  CbmMcbm2024CheckEventsDt();


  /** @brief Copy constructor (disabled) **/
  CbmMcbm2024CheckEventsDt(const CbmMcbm2024CheckEventsDt&) = delete;


  /** @brief Destructor **/
  virtual ~CbmMcbm2024CheckEventsDt();


  /** @brief Task execution **/
  virtual void Exec(Option_t* opt);


  /** @brief Finish timeslice **/
  virtual void Finish();


  /** @brief Assignment operator (disabled) **/
  CbmMcbm2024CheckEventsDt& operator=(const CbmMcbm2024CheckEventsDt&) = delete;


 private:  // methods
  /** @brief Task initialisation **/
  virtual InitStatus Init();

 private:                                              // members
  const std::vector<CbmDigiEvent>* fEvents = nullptr;  //! Input data (events)
  size_t fNumTs                            = 0;        ///< Number of processed timeslices
  size_t fNumEvents                        = 0;        ///< Number of events

  std::vector<std::string> fvDets              = {"Bmon", "Sts", "Much", "Trd1d", "Trd2d", "Tof", "Rich", "Fsd"};
  std::map<std::string, TH1*> fHistDt          = {};
  std::map<std::string, TH2*> fHistDtEvo       = {};
  std::map<std::string, TH1*> fHistDtToBmon    = {};
  std::map<std::string, TH2*> fHistDtToBmonEvo = {};
  std::map<std::string, TH1*> fHistMul         = {};
  std::map<std::string, TH2*> fHistDtMul       = {};

  TCanvas* fCanvDt    = nullptr;
  TCanvas* fCanvDtEvo = nullptr;

  TCanvas* fCanvDtToBmon    = nullptr;
  TCanvas* fCanvDtToBmonEvo = nullptr;

  std::map<std::string, TCanvas*> fCanvMul = {};

  ClassDef(CbmMcbm2024CheckEventsDt, 1);
};

#endif /* CBMMCBM2024CHECKEVENTSDT_H */
