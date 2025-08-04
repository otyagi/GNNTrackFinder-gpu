/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#ifndef CBMTASKDIGIEVENTQA_H
#define CBMTASKDIGIEVENTQA_H 1

#include "CbmDefs.h"
#include "CbmDigiEvent.h"
#include "algo/evbuild/Config.h"
#include "algo/qa/DigiEventQa.h"
#include "algo/qa/Histo1D.h"

#include <FairTask.h>

#include <vector>


class TH1D;

/** @class CbmTaskDigiEventQa
 ** @brief QA task class for digi events produced by the event builder
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 15.03.2022
 **
 ** Currently implemented functionality: histogram the digi time within each event.
 ** To be expanded for more QA figures.
 ** The histograms are published to the THttpServer.
 **/
class CbmTaskDigiEventQa : public FairTask {

 public:
  /** @brief Constructor **/
  CbmTaskDigiEventQa();


  /** @brief Copy constructor (disabled) **/
  CbmTaskDigiEventQa(const CbmTaskDigiEventQa&) = delete;


  /** @brief Destructor **/
  virtual ~CbmTaskDigiEventQa();


  /** @brief Configuration
   ** @param config  Reconstruction configuration
   **
   ** Histograms are created with limits adjusted to the windows use by the event builder.
   **/
  void Config(const cbm::algo::evbuild::Config& config);


  /** @brief Task execution **/
  virtual void Exec(Option_t* opt);


  /** @brief Finish timeslice **/
  virtual void Finish();


  /** @brief Assignment operator (disabled) **/
  CbmTaskDigiEventQa& operator=(const CbmTaskDigiEventQa&) = delete;


 private:  // methods
  /** @brief Task initialisation **/
  virtual InitStatus Init();


  /** @brief Create a ROOT TH1D from a H1D object
   ** @param Source histogram
   ** @param ROOT histogram
   */
  TH1D* ToTH1D(const cbm::algo::qa::H1D& source);


 private:                                              // members
  const std::vector<CbmDigiEvent>* fEvents = nullptr;  //! Input data (events)
  size_t fNumTs                            = 0;        ///< Number of processed timeslices
  size_t fNumEvents                        = 0;        ///< Number of analysed events
  size_t fNumDigis                         = 0;        ///< Number of analysed digis
  double fExecTime                         = 0.;       ///< Execution time [s]

  std::unique_ptr<cbm::algo::evbuild::DigiEventQa> fAlgo;
  cbm::algo::evbuild::DigiEventQaConfig fConfig;

  // ---- Histograms with digi times
  std::map<ECbmModuleId, TH1D*> fDigiTimeHistos = {};


  ClassDef(CbmTaskDigiEventQa, 1);
};

#endif /* CBMTASKDIGIEVENTQA_H */
