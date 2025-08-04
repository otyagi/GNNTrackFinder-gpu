/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#ifndef CBM_ALGO_EVBUILD_EVBUILDCHAIN_H
#define CBM_ALGO_EVBUILD_EVBUILDCHAIN_H 1

#include "CbmDefs.h"
#include "Config.h"
#include "DigiEventQa.h"
#include "DigiEventSelector.h"
#include "EventBuilder.h"
#include "HistogramSender.h"
#include "HitMultTrigger.h"
#include "RecoResults.h"
#include "SubChain.h"
#include "TimeClusterTrigger.h"
#include "V0Trigger.h"

#include <memory>

namespace cbm::algo
{
  struct DigiData;
}

namespace cbm::algo::evbuild
{

  struct EventbuildChainMonitorData {
    EventBuilderMonitorData evbuild;
    TimeClusterTriggerMonitorData digiMultTrigger;
    TimeClusterTriggerMonitorData hitMultTrigger;
    V0TriggerMoniData v0Trigger;
  };

  /** @class EventbuildChain
   ** @brief Steering class for event building from digi timeslices
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 11 July 2023
   **
   ** Constructs a vector of CbmDigiEvents from a CbmDigiTimeslice. Uses the algorithms
   ** - TimeClusterTrigger for the event time definition
   ** - EventBuilder for construction of events by time intervals around the trigger time
   ** - DigiEventSelector for selection of constructed events according to the specified criteria
   **/
  class EventbuildChain : public SubChain {

   public:
    using ResultType = std::pair<std::vector<DigiEvent>, EventbuildChainMonitorData>;

    /** @brief Constructor **/
    EventbuildChain(const Config& config, std::shared_ptr<HistogramSender> sender = nullptr);

    /** @brief Destructor **/
    ~EventbuildChain();

    /** @brief Execution **/
    ResultType Run(const DigiData&, const RecoResults&);

    /** @brief Status info to logger **/
    void Status() const;

    /** @brief Registers tracking setup **/
    void RegisterTrackingSetup(std::shared_ptr<TrackingSetup> pSetup) { fSelector.RegisterTrackingSetup(pSetup); }

    /** @brief Sets V0 trigger QA
     ** @param pQa  Qa module
     **/
    void SetV0TriggerQa(std::shared_ptr<V0TriggerQa> pQa) { fV0Trigger.SetQa(pQa); }

   private:                                              // members
    Config fConfig;                                      ///< Global configuration
    ECbmModuleId fTriggerDet = ECbmModuleId::kNotExist;  ///< Trigger detector
    TimeClusterTrigger fDigiMultTrigger;                 ///< Digi multiplicity trigger algorithm
    HitMultTrigger fHitMultTrigger;                      ///< Hit multiplicity trigger algorithm
    V0Trigger fV0Trigger;                                ///< V0 trigger algorithm
    EventBuilder fBuilder;                               ///< Event builder algorithm
    DigiEventSelector fSelector;                         ///< Event selector algorithm
    DigiEventQa fQa;                                     ///< Event QA algorithm
    std::shared_ptr<HistogramSender> fSender;            ///< Histogram sender

   private:  // methods
    /** @brief Extract digi times from CbmDigiTimeslice
     ** @param system Detector system (enum ECbmModuleId)
     ** @return Vector of digi times for the specified system
     **/
    std::vector<double> GetDigiTimes(const DigiData& timeslice, ECbmModuleId system);
  };

}  // namespace cbm::algo::evbuild


#endif  //CBM_ALGO_EVBUILD_EVBUILDCHAIN
