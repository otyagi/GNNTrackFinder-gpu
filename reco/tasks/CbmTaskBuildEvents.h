/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#ifndef CBMTASKBUILDEVENTS_H
#define CBMTASKBUILDEVENTS_H 1

#include "CbmDefs.h"
#include "CbmDigiEvent.h"
#include "CbmDigiTimeslice.h"
#include "DigiEventSelector.h"
#include "DigiEventSelectorConfig.h"
#include "EventBuilder.h"

#include <FairTask.h>

#include <vector>

class CbmDigiManager;

/** @class CbmTaskBuildEvents
 ** @brief Task class for associating digis to events
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 15.11.2021
 **
 ** Creates objects of class CbmDigiEvent and fills them with digi objects,
 ** using the algorithm EventBuilder.
 **
 ** TOFO: The current implementation is for STS only and with a dummy trigger list
 ** just to establish the framework integration of algorithm and data interfaces.
 **/
class CbmTaskBuildEvents : public FairTask {

 public:
  /** @brief Constructor **/
  CbmTaskBuildEvents();


  /** @brief Copy constructor (disabled) **/
  CbmTaskBuildEvents(const CbmTaskBuildEvents&) = delete;


  /** @brief Destructor **/
  virtual ~CbmTaskBuildEvents();


  /** @brief Task execution **/
  virtual void Exec(Option_t* opt);


  /** @brief Finish timeslice **/
  virtual void Finish();


  /** @brief Assignment operator (disabled) **/
  CbmTaskBuildEvents& operator=(const CbmTaskBuildEvents&) = delete;


  /** @brief Configure the event builder algorithm **/
  void SetConfig(const cbm::algo::evbuild::EventBuilderConfig& config)
  {
    fConfig.reset(new cbm::algo::evbuild::EventBuilderConfig(config));
  }


  /** @brief Activate event selector which requires a minimum number of fired layers
   ** @param params Struct with minimum number of layers for different detectors
   **/

  void SetDigiEventSelector(cbm::algo::evbuild::DigiEventSelectorConfig config)
  {
    fSelector.reset(new cbm::algo::evbuild::DigiEventSelector(config));
  }


 private:  // methods
  /** @brief Task initialisation **/
  virtual InitStatus Init();


  /** @brief Construct a DigiTimeslice from the data in CbmDigiManager **/
  CbmDigiTimeslice FillTimeSlice();


  /** @brief Number of digis for a given system
   ** @param data    CbmDigiData object (DigiTimeslice or DigiEvent)
   ** @param system  System identifier (enum ECbmModuleId)
   ** @return        Number of digis for the system
   **/
  size_t GetNumDigis(const CbmDigiData& data, ECbmModuleId system);

 private:                                                            // members
  const CbmDigiTimeslice* fTimeslice   = nullptr;                    //! Input data (from unpacking)
  CbmDigiManager* fDigiMan             = nullptr;                    //! Input data (from simulation)
  const std::vector<double>* fTriggers = nullptr;                    //! Input data (triggers)
  std::vector<CbmDigiEvent>* fEvents   = nullptr;                    //! Output data (events)
  std::unique_ptr<cbm::algo::evbuild::DigiEventSelector> fSelector;  //! Event selector
  std::unique_ptr<cbm::algo::evbuild::EventBuilder> fAlgo;           //! Algorithm
  std::unique_ptr<cbm::algo::evbuild::EventBuilderConfig> fConfig;   //! Event builder configuration

  // for diagnostics
  std::map<ECbmModuleId, size_t> fNumDigisTs;  //  Number of digis in timeslices
  std::map<ECbmModuleId, size_t> fNumDigisEv;  //  Number of digis in events
  size_t fNumTs           = 0;                 //  Number of processed time slices
  size_t fNumTriggers     = 0;                 //  Number of triggers
  size_t fNumEvents       = 0;                 //  Number of produced events
  double fTimeFillTs      = 0.;
  double fTimeBuildEvt    = 0.;
  double fTimeSelectorEvt = 0.;
  double fTimeTot         = 0.;

  ClassDef(CbmTaskBuildEvents, 1);
};

#endif /* CBMTASKBUILDEVENTS_H */
