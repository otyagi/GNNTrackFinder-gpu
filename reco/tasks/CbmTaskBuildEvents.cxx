/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "CbmTaskBuildEvents.h"

#include "CbmDigiBranchBase.h"
#include "CbmDigiManager.h"
#include "CbmDigiTimeslice.h"

#include <FairRootManager.h>
#include <Logger.h>

#include <TStopwatch.h>

#include <cassert>
#include <iomanip>

using namespace std;

// -----   Constructor   -----------------------------------------------------
CbmTaskBuildEvents::CbmTaskBuildEvents() : FairTask("BuildEvents") {}
// ---------------------------------------------------------------------------


// -----   Destructor   ------------------------------------------------------
CbmTaskBuildEvents::~CbmTaskBuildEvents()
{
  if (fEvents) delete fEvents;
}
// ---------------------------------------------------------------------------


// ------ Construct a DigiTimeslice from the data in CbmDigiManager ----------
CbmDigiTimeslice CbmTaskBuildEvents::FillTimeSlice()
{
  CbmDigiTimeslice ts;
  for (const auto& entry : fConfig->fWindows) {
    auto system                   = entry.first;
    CbmDigiBranchBase* digiBranch = fDigiMan->GetBranch(system);
    if (digiBranch == nullptr) continue;
    switch (system) {
      case ECbmModuleId::kSts: {
        const vector<CbmStsDigi>* digiVec =
          boost::any_cast<const vector<CbmStsDigi>*>(digiBranch->GetBranchContainer());
        assert(digiVec);
        std::copy(digiVec->begin(), digiVec->end(), std::back_inserter(ts.fData.fSts.fDigis));
        break;
      }
      case ECbmModuleId::kRich: {
        const vector<CbmRichDigi>* digiVec =
          boost::any_cast<const vector<CbmRichDigi>*>(digiBranch->GetBranchContainer());
        assert(digiVec);
        std::copy(digiVec->begin(), digiVec->end(), std::back_inserter(ts.fData.fRich.fDigis));
        break;
      }
      case ECbmModuleId::kMuch: {
        const vector<CbmMuchDigi>* digiVec =
          boost::any_cast<const vector<CbmMuchDigi>*>(digiBranch->GetBranchContainer());
        assert(digiVec);
        std::copy(digiVec->begin(), digiVec->end(), std::back_inserter(ts.fData.fMuch.fDigis));
        break;
      }
      case ECbmModuleId::kTrd: {
        const vector<CbmTrdDigi>* digiVec =
          boost::any_cast<const vector<CbmTrdDigi>*>(digiBranch->GetBranchContainer());
        assert(digiVec);
        std::copy(digiVec->begin(), digiVec->end(), std::back_inserter(ts.fData.fTrd.fDigis));
        break;
      }
      case ECbmModuleId::kTrd2d: {
        const vector<CbmTrdDigi>* digiVec =
          boost::any_cast<const vector<CbmTrdDigi>*>(digiBranch->GetBranchContainer());
        assert(digiVec);
        std::copy(digiVec->begin(), digiVec->end(), std::back_inserter(ts.fData.fTrd2d.fDigis));
        break;
      }
      case ECbmModuleId::kTof: {
        const vector<CbmTofDigi>* digiVec =
          boost::any_cast<const vector<CbmTofDigi>*>(digiBranch->GetBranchContainer());
        assert(digiVec);
        std::copy(digiVec->begin(), digiVec->end(), std::back_inserter(ts.fData.fTof.fDigis));
        break;
      }
      case ECbmModuleId::kFsd: {
        const vector<CbmFsdDigi>* digiVec =
          boost::any_cast<const vector<CbmFsdDigi>*>(digiBranch->GetBranchContainer());
        assert(digiVec);
        std::copy(digiVec->begin(), digiVec->end(), std::back_inserter(ts.fData.fFsd.fDigis));
        break;
      }
      case ECbmModuleId::kPsd: {
        const vector<CbmPsdDigi>* digiVec =
          boost::any_cast<const vector<CbmPsdDigi>*>(digiBranch->GetBranchContainer());
        assert(digiVec);
        ts.fData.fPsd.fDigis = *digiVec;
        break;
      }
      case ECbmModuleId::kBmon: {  //Bmon has Tof digis
        const vector<CbmBmonDigi>* digiVec =
          boost::any_cast<const vector<CbmBmonDigi>*>(digiBranch->GetBranchContainer());
        assert(digiVec);
        std::copy(digiVec->begin(), digiVec->end(), std::back_inserter(ts.fData.fTof.fDigis));
        break;
      }
      default: LOG(fatal) << GetName() << ": Unknown detector type!";
    }
  }
  return ts;
}
// ---------------------------------------------------------------------------


// -----   Execution   -------------------------------------------------------
void CbmTaskBuildEvents::Exec(Option_t*)
{

  // --- Timer and counters
  TStopwatch timerStep;
  TStopwatch timerTot;
  timerTot.Start();
  std::map<ECbmModuleId, size_t> numDigisTs;
  std::map<ECbmModuleId, size_t> numDigisEv;

  // --- Clear output vector
  fEvents->clear();

  // --- If the input is already CbmDigiTimeslice (from unpacking), use that directly
  if (fTimeslice) {
    timerStep.Start();
    *fEvents = cbm::algo::DigiEvent::ToCbmDigiEvents(
      (*fAlgo)(cbm::algo::DigiData(fTimeslice->fData), *fTriggers, std::nullopt).first);
    timerStep.Stop();
    fTimeBuildEvt += timerStep.RealTime();
    for (const auto& entry : fConfig->fWindows)
      numDigisTs[entry.first] = GetNumDigis(fTimeslice->fData, entry.first);
  }

  // --- If input is not DigiTimeslice: construct a transientDigiTimeslice from the data in CbmDigiManager
  else {
    timerStep.Start();
    CbmDigiTimeslice ts = FillTimeSlice();
    for (const auto& entry : fConfig->fWindows)
      numDigisTs[entry.first] = ts.fData.Size(entry.first);
    timerStep.Stop();
    fTimeFillTs += timerStep.RealTime();
    timerStep.Start();
    *fEvents =
      cbm::algo::DigiEvent::ToCbmDigiEvents((*fAlgo)(cbm::algo::DigiData(ts.fData), *fTriggers, std::nullopt).first);
    timerStep.Stop();
    fTimeBuildEvt += timerStep.RealTime();
  }

  // Apply event selector if desired
  if (fSelector) {
    timerStep.Start();
    auto noTrigger = [&](CbmDigiEvent& ev) { return !(*fSelector)(cbm::algo::DigiEvent(ev)); };
    auto removeIt  = std::remove_if(fEvents->begin(), fEvents->end(), noTrigger);
    fEvents->erase(removeIt, fEvents->end());
    timerStep.Stop();
    fTimeSelectorEvt += timerStep.RealTime();
  }

  // --- Timeslice statistics
  size_t numTriggers = fTriggers->size();
  size_t numEvents   = fEvents->size();
  for (const auto& entry : fConfig->fWindows) {
    for (auto& event : (*fEvents)) {
      numDigisEv[entry.first] += GetNumDigis(event.fData, entry.first);
    }
  }

  // --- Timeslice log
  timerTot.Stop();
  fTimeTot += timerTot.RealTime();
  stringstream logOut;
  logOut << setw(15) << left << GetName() << " [";
  logOut << fixed << setw(8) << setprecision(1) << right << timerTot.RealTime() * 1000. << " ms] ";
  logOut << "TS " << fNumTs << ", triggers " << numTriggers << ", events " << numEvents;

  for (const auto& entry : fConfig->fWindows) {
    auto system = entry.first;
    logOut << ", frac " << ToString(system) << " digis "
           << 100. * double(numDigisEv[system]) / double(numDigisTs[system]) << " %";
  }

  LOG(info) << logOut.str();

  // --- Run statistics
  fNumTs++;
  fNumTriggers += numTriggers;
  fNumEvents += numEvents;

  for (const auto& entry : fConfig->fWindows) {
    auto system = entry.first;
    fNumDigisTs[system] += numDigisTs[system];
    fNumDigisEv[system] += numDigisEv[system];
  }
}
// ----------------------------------------------------------------------------


// -----   End-of-timeslice action   ------------------------------------------
void CbmTaskBuildEvents::Finish()
{
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Timeslices           : " << fNumTs;
  LOG(info) << "Triggers             : " << fNumTriggers;
  LOG(info) << "Events               : " << fNumEvents;
  for (const auto& entry : fConfig->fWindows) {
    auto system = entry.first;
    LOG(info) << setw(4) << left << ToString(system) << " digis in TS     : " << fNumDigisTs[system];
    LOG(info) << setw(4) << left << ToString(system) << " digis in events : " << fNumDigisEv[system] << " = " << fixed
              << setprecision(2) << 100. * double(fNumDigisEv[system]) / double(fNumDigisTs[system]) << " %";
  }
  LOG(info) << "Time  / TS           : " << fixed << setprecision(2) << 1000. * fTimeTot / double(fNumTs) << " ms";
  LOG(info) << "Time fill TS         : " << fixed << setprecision(2) << 1000. * fTimeFillTs / double(fNumTs)
            << " ms = " << 100. * fTimeFillTs / fTimeTot << " %";
  LOG(info) << "Time build events    : " << fixed << setprecision(2) << 1000. * fTimeBuildEvt / double(fNumTs)
            << " ms = " << 100. * fTimeBuildEvt / fTimeTot << " %";
  LOG(info) << "Time selector events   : " << fixed << setprecision(2) << 1000. * fTimeSelectorEvt / double(fNumTs)
            << " ms = " << 100. * fTimeSelectorEvt / fTimeTot << " %";
  LOG(info) << "=====================================";
}
// ----------------------------------------------------------------------------


// -----  Number of digis in the timeslice   ----------------------------------
size_t CbmTaskBuildEvents::GetNumDigis(const CbmDigiData& data, ECbmModuleId system)
{
  size_t result = 0;
  switch (system) {
    case ECbmModuleId::kSts: result = data.fSts.fDigis.size(); break;
    case ECbmModuleId::kRich: result = data.fRich.fDigis.size(); break;
    case ECbmModuleId::kMuch: result = data.fMuch.fDigis.size(); break;
    case ECbmModuleId::kTrd: result = data.fTrd.fDigis.size(); break;
    case ECbmModuleId::kTrd2d: result = data.fTrd2d.fDigis.size(); break;
    case ECbmModuleId::kTof: result = data.fTof.fDigis.size(); break;
    case ECbmModuleId::kPsd: result = data.fPsd.fDigis.size(); break;
    case ECbmModuleId::kBmon: result = data.fBmon.fDigis.size(); break;
    default: result = 0; break;
  }
  return result;
}
// ----------------------------------------------------------------------------


// -----   Initialisation   ---------------------------------------------------
InitStatus CbmTaskBuildEvents::Init()
{
  // --- Get FairRootManager instance
  FairRootManager* ioman = FairRootManager::Instance();
  assert(ioman);

  LOG(info) << "==================================================";
  LOG(info) << GetName() << ": Initialising...";

  // --- Check input data
  // --- DigiTimeslice: Unpacked data from FLES
  fTimeslice = ioman->InitObjectAs<const CbmDigiTimeslice*>("DigiTimeslice.");
  if (fTimeslice) {
    LOG(info) << "--- Found branch DigiTimeslice.";
  }
  // --- DigiManager: Simulated digi data
  else {
    fDigiMan = CbmDigiManager::Instance();
    fDigiMan->Init();
    for (const auto& entry : fConfig->fWindows) {
      auto system = entry.first;
      if (!fDigiMan->IsPresent(system)) {
        LOG(warn) << GetName() << ": No digi branch for " << ToString(system);
      }
      LOG(info) << "--- Found digi branch for " << ToString(system);
    }
  }

  // --- Initialize diagnostics
  for (const auto& entry : fConfig->fWindows) {
    auto system         = entry.first;
    fNumDigisTs[system] = 0;
    fNumDigisEv[system] = 0;
  }

  // --- Get input data (triggers)
  fTriggers = ioman->InitObjectAs<std::vector<double> const*>("Trigger");
  if (!fTriggers) {
    LOG(fatal) << GetName() << ": No Trigger branch!" << endl;
    return kFATAL;
  }
  LOG(info) << "--- Found branch Trigger";

  // --- Register output array (CbmDigiEvent)
  if (ioman->GetObject("DigiEvent")) {
    LOG(fatal) << GetName() << ": Branch DigiEvent already exists!";
    return kFATAL;
  }
  fEvents = new vector<CbmDigiEvent>;
  ioman->RegisterAny("DigiEvent", fEvents, IsOutputBranchPersistent("DigiEvent"));
  if (!fEvents) {
    LOG(fatal) << GetName() << ": Output branch could not be created!";
    return kFATAL;
  }
  LOG(info) << "--- Registered branch DigiEvent";

  // --- Configure algorithm
  fAlgo.reset(new cbm::algo::evbuild::EventBuilder(*fConfig));
  LOG(info) << fAlgo->ToString();

  // --- Log selector
  if (fSelector)
    LOG(info) << fSelector->ToString();
  else
    LOG(info) << "--- No event selection";

  LOG(info) << "==================================================";

  return kSUCCESS;
}
// ----------------------------------------------------------------------------

ClassImp(CbmTaskBuildEvents)
