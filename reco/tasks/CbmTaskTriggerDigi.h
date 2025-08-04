/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#ifndef CBMTASKTRIGGERDIGI_H
#define CBMTASKTRIGGERDIGI_H 1

#include "CbmDefs.h"
#include "CbmDigiBranchBase.h"
#include "TimeClusterTrigger.h"

#include <FairTask.h>

#include <TStopwatch.h>

#include <boost/any.hpp>

#include <vector>

class CbmDigiBranchBase;
class CbmDigiManager;
class CbmDigiTimeslice;

using namespace std;

/** @class CbmTaskTriggerDigi
 ** @brief Task class for minimum-bias event trigger from time-distribution of digi data
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 20.11.2021
 **
 ** The tasks calls algo::TimeClusterTrigger with the digi time distribution of the trigger detector.
 **
 ** TOFO: The current implementation is for STS only as trigger detector.
 **/
class CbmTaskTriggerDigi : public FairTask {


 public:
  /** @brief Constructor **/
  CbmTaskTriggerDigi();


  /** @brief Copy constructor (disabled) **/
  CbmTaskTriggerDigi(const CbmTaskTriggerDigi&) = delete;


  /** @brief Destructor **/
  virtual ~CbmTaskTriggerDigi();


  /** @brief Task execution **/
  virtual void Exec(Option_t* opt);


  /** @brief Finish timeslice **/
  virtual void Finish();


  /** @brief Assignment operator (disabled) **/
  CbmTaskTriggerDigi& operator=(const CbmTaskTriggerDigi&) = delete;


  /** @brief Configure the trigger algorithm **/
  void SetConfig(const cbm::algo::evbuild::DigiTriggerConfig& config)
  {
    fConfig.reset(new cbm::algo::evbuild::DigiTriggerConfig(config));
  }

  /** @brief Add a detector system to the trigger algorithm
   ** @param system    System to be added
   **/
  void AddSystem(ECbmModuleId system)
  {
    if (std::find(fSystems.begin(), fSystems.end(), system) != fSystems.end()) return;
    fSystems.push_back(system);
  }

 private:  // methods
  /** @brief Task initialisation **/
  virtual InitStatus Init();

  /** @brief Extract digi times from digi branch
   ** @param digiBranch    Digi branch for one detector
   **/
  template<class TDigi>
  std::vector<double> GetDigiTimes(const CbmDigiBranchBase* digiBranch)
  {
    TStopwatch timerStep;
    // --- Get input digi vector
    const vector<TDigi>* digiVec = boost::any_cast<const vector<TDigi>*>(digiBranch->GetBranchContainer());
    assert(digiVec);

    // --- Extract digi times into to a vector
    timerStep.Start();
    std::vector<double> digiTimes(digiVec->size());
    std::transform(digiVec->begin(), digiVec->end(), digiTimes.begin(),
                   [](const TDigi& digi) { return digi.GetTime(); });
    timerStep.Stop();
    fTimeExtract += timerStep.RealTime();
    return digiTimes;
  }


  /** @brief Extract digi times from CbmDigiTimeslice
   ** @param system Detector system (enum ECbmModuleId)
   ** @return Vector of digi times for the specified system
   **/
  std::vector<double> GetDigiTimes(ECbmModuleId system);


 private:                                                                    // members
  const CbmDigiTimeslice* fTimeslice = nullptr;                              //! Input data (from unpacking)
  CbmDigiManager* fDigiMan           = nullptr;                              //! Input data (from simulation)
  std::vector<ECbmModuleId> fSystems{};                                      //  List of detector systems
  std::vector<double>* fTriggers                                 = nullptr;  //! Output data
  std::unique_ptr<cbm::algo::evbuild::TimeClusterTrigger> fAlgo  = nullptr;  //! Algorithm
  std::unique_ptr<cbm::algo::evbuild::DigiTriggerConfig> fConfig = nullptr;  //! Configuration / parameters
  double fTriggerWindow                                          = 0.;
  int32_t fMinNumDigis                                           = 0;
  double fDeadTime                                               = 0.;
  size_t fNumTs                                                  = 0;  //  Number of processed time slices
  size_t fNumDigis                                               = 0;  //  Number of digis from trigger detector
  size_t fNumTriggers                                            = 0;  //  Number of found triggers
  double fTimeExtract                                            = 0.;
  double fTimeFind                                               = 0.;
  double fTimeTot                                                = 0.;


  ClassDef(CbmTaskTriggerDigi, 1);
};

#endif /* CBMTASKTRIGGERDIGI_H */
