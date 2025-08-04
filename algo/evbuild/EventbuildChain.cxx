/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "EventbuildChain.h"

#include "CbmDigiTimeslice.h"
#include "DigiData.h"
#include "HistogramContainer.h"
#include "evbuild/Config.h"

#include <sstream>
#include <string>

using namespace cbm::algo;
using namespace cbm::algo::evbuild;


// -----   Constructor   ------------------------------------------------------
EventbuildChain::EventbuildChain(const Config& config, std::shared_ptr<HistogramSender> sender)
  : fConfig(config)
  , fTriggerDet(config.fDigiTrigger.Detector())
  , fDigiMultTrigger(config.fDigiTrigger.Window(), config.fDigiTrigger.Threshold(), config.fDigiTrigger.DeadTime())
  , fHitMultTrigger(config.fHitMultTrigger)
  , fV0Trigger()
  , fBuilder(config.fBuilder)
  , fSelector(config.fSelector)
  , fQa(DigiEventQaConfig(config.fBuilder, 10., 100))
  , fSender(sender)
{
  Status();

  if (fSender) {
    /// FIXME: based on JdC question, decide whether config re-emitted on each iteration instead of only at startup?
    /// => Header for multi-part message with Configuration + data
    /// => Format: std::pair< Nb histogram configs, Nb canvas configs  >
    std::vector<std::pair<std::string, std::string>> histsCfg = fQa.GetConfig().GetHistosConfigs();
    std::vector<std::pair<std::string, std::string>> canvsCfg = fQa.GetConfig().GetCanvasConfigs();
    fSender->PrepareAndSendMsg(std::pair<uint32_t, uint32_t>(histsCfg.size(), canvsCfg.size()),
                               zmq::send_flags::sndmore);

    /// => Histograms configuration = destination folder in http browser, mandatory but can be empty (= root folder)
    /// => 1 ZMQ message per histogram (= 1 part)
    /// => If no (new) histograms declared (e.g. new canvas declaration), has to be en empty message + `0` in the header
    for (const auto& cfg : histsCfg) {
      fSender->PrepareAndSendMsg(cfg, zmq::send_flags::sndmore);
    }

    /// => Canvas configuration
    /// => 1 ZMQ message per canvas (= 1 part)
    /// => If no (new) canvas declared (e.g. only histos declaration), has to be en empty message + `0` in the header
    for (const auto& cfg : canvsCfg) {
      fSender->PrepareAndSendMsg(cfg, zmq::send_flags::sndmore);
    }

    /// => (empty) Histograms serialization and emission to close multi-part message
    fSender->PrepareAndSendMsg(qa::HistogramContainer{}, zmq::send_flags::none);
  }
}
// ----------------------------------------------------------------------------

// -----   Destructor   ------------------------------------------------------
EventbuildChain::~EventbuildChain() {}
// ----------------------------------------------------------------------------

// -----   Run event building on a timeslice   --------------------------------
typedef cbm::algo::ca::Vector<cbm::algo::ca::Track> TrackVector;

EventbuildChain::ResultType EventbuildChain::Run(const DigiData& digiData, const RecoResults& recoData)
{

  // --- Local variables
  std::vector<double> triggers;
  ResultType result;

  // --- If V0Trigger is configured, use it
  if (fConfig.fV0Trigger.IsSet()) {
    auto [v0Triggers, v0TriggerMon] = fV0Trigger(recoData.tracks, fConfig.fV0Trigger);
    triggers                        = std::move(v0Triggers);
    result.second.v0Trigger         = std::move(v0TriggerMon);
  }

  // --- Else, check the hit multiplicity trigger
  else if (fConfig.fHitMultTrigger.IsSet()) {
    auto [hitTriggers, digiTriggerMon] = fHitMultTrigger(recoData);
    triggers                           = std::move(hitTriggers);
    result.second.hitMultTrigger       = std::move(digiTriggerMon);
  }

  // --- Else, use the digi multiplicity trigger
  else if (fConfig.fDigiTrigger.IsSet()) {
    std::vector<double> digiTimes       = GetDigiTimes(digiData, fTriggerDet);
    auto [digiTriggers, digiTriggerMon] = fDigiMultTrigger(digiTimes);
    triggers                            = std::move(digiTriggers);
    result.second.digiMultTrigger       = std::move(digiTriggerMon);
  }

  // --- Else, throw exception
  else
    throw std::runtime_error("no trigger is configured");

  // --- Perform event building
  auto [events, evbuildMon] = fBuilder(digiData, triggers, fSelector);
  result.first              = std::move(events);
  result.second.evbuild     = evbuildMon;

  /// => Histograms serialization and emission
  if (fSender) {
    L_(info) << "Running DigiEventQa";
    // --- Run event QA
    DigiEventQaData qaData = fQa(result.first);
    L_(info) << "Running DigiEventQa: done";

    fSender->PrepareAndSendMsg(qaData.fHistContainer, zmq::send_flags::none);
    int nHistograms = std::distance(qaData.fHistContainer.fvH1.begin(), qaData.fHistContainer.fvH1.end());
    L_(info) << "Published histograms, nb: " << nHistograms;
  }

  // --- Some log
  L_(info) << "Triggers: " << triggers.size() << ", events " << result.first.size();

  return result;
}
// ----------------------------------------------------------------------------


// -----   Status info   ------------------------------------------------------
void EventbuildChain::Status() const
{
  L_(info) << "===   Eventbuilder configuration   ===================";
  if (fConfig.fV0Trigger.IsSet())
    L_(info) << fV0Trigger.ToString();
  else if (fConfig.fHitMultTrigger.IsSet())
    L_(info) << fHitMultTrigger.ToString();
  else {
    L_(info) << "--- Using digi multiplicity trigger with trigger detector " << ::ToString(fTriggerDet);
    L_(info) << fDigiMultTrigger.ToString();
  }
  L_(info) << fBuilder.ToString();
  L_(info) << fSelector.ToString();
  L_(info) << fQa.ToString();
  L_(info) << "======================================================";
}
// ----------------------------------------------------------------------------


// -----   Get digi times from CbmDigiTimeslice   -----------------------------
std::vector<double> EventbuildChain::GetDigiTimes(const DigiData& timeslice, ECbmModuleId system)
{
  std::vector<double> result;
  switch (system) {
    case ECbmModuleId::kSts: {
      result.resize(timeslice.fSts.size());
      auto it1 = timeslice.fSts.begin();
      auto it2 = timeslice.fSts.end();
      std::transform(it1, it2, result.begin(), [](const CbmStsDigi& digi) { return digi.GetTime(); });
      break;
    }
    case ECbmModuleId::kRich: {
      result.resize(timeslice.fRich.size());
      auto it1 = timeslice.fRich.begin();
      auto it2 = timeslice.fRich.end();
      std::transform(it1, it2, result.begin(), [](const CbmRichDigi& digi) { return digi.GetTime(); });
      break;
    }
    case ECbmModuleId::kMuch: {
      result.resize(timeslice.fMuch.size());
      auto it1 = timeslice.fMuch.begin();
      auto it2 = timeslice.fMuch.end();
      std::transform(it1, it2, result.begin(), [](const CbmMuchDigi& digi) { return digi.GetTime(); });
      break;
    }
    case ECbmModuleId::kTrd: {
      result.resize(timeslice.fTrd.size());
      auto it1 = timeslice.fTrd.begin();
      auto it2 = timeslice.fTrd.end();
      std::transform(it1, it2, result.begin(), [](const CbmTrdDigi& digi) { return digi.GetTime(); });
      break;
    }
    case ECbmModuleId::kTof: {
      result.resize(timeslice.fTof.size());
      auto it1 = timeslice.fTof.begin();
      auto it2 = timeslice.fTof.end();
      std::transform(it1, it2, result.begin(), [](const CbmTofDigi& digi) { return digi.GetTime(); });
      break;
    }
    case ECbmModuleId::kPsd: {
      result.resize(timeslice.fPsd.size());
      auto it1 = timeslice.fPsd.begin();
      auto it2 = timeslice.fPsd.end();
      std::transform(it1, it2, result.begin(), [](const CbmPsdDigi& digi) { return digi.GetTime(); });
      break;
    }
    case ECbmModuleId::kFsd: {
      result.resize(timeslice.fFsd.size());
      auto it1 = timeslice.fFsd.begin();
      auto it2 = timeslice.fFsd.end();
      std::transform(it1, it2, result.begin(), [](const CbmFsdDigi& digi) { return digi.GetTime(); });
      break;
    }
    case ECbmModuleId::kBmon: {
      result.resize(timeslice.fBmon.size());
      auto it1 = timeslice.fBmon.begin();
      auto it2 = timeslice.fBmon.end();
      std::transform(it1, it2, result.begin(), [](const CbmTofDigi& digi) { return digi.GetTime(); });
      break;
    }
    default: {
      L_(error) << "EventbuildChain::GetDigiTimes: Unknown system " << system;
      break;
    }
  }  //? system

  return result;
}
// ----------------------------------------------------------------------------
