/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "CbmTaskInspectDigiEvents.h"

#include <FairRootManager.h>
#include <Logger.h>


// -----   Constructor   -----------------------------------------------------
CbmTaskInspectDigiEvents::CbmTaskInspectDigiEvents() : FairTask("InspectDigiEvents") {}
// ---------------------------------------------------------------------------


// -----   Destructor   ------------------------------------------------------
CbmTaskInspectDigiEvents::~CbmTaskInspectDigiEvents() {}
// ---------------------------------------------------------------------------


// -----   Execution   -------------------------------------------------------
void CbmTaskInspectDigiEvents::Exec(Option_t*)
{

  // --- Inspect event vector
  LOG(info) << GetName() << ": timeslice " << fNumTs << " with " << fEvents->size() << " events"
            << (fEvents->size() > 10 ? " (showing the first 10 only)" : "");
  size_t numEventsInTs = 0;
  for (auto& event : *fEvents) {
    size_t numBmon  = event.fData.fBmon.Size();
    size_t numSts   = event.fData.fSts.Size();
    size_t numMuch  = event.fData.fMuch.Size();
    size_t numRich  = event.fData.fRich.Size();
    size_t numTrd1d = event.fData.fTrd.Size();
    size_t numTrd2d = event.fData.fTrd2d.Size();
    size_t numTof   = event.fData.fTof.Size();
    size_t numFsd   = event.fData.fFsd.Size();
    LOG(info) << "  Event " << event.fNumber << ", time " << event.fTime << ", digis: bmon " << numBmon << "  sts "
              << numSts << "  much " << numMuch << "  rich " << numRich << "  trd1d " << numTrd1d << "  trd2d "
              << numTrd2d << "  tof " << numTof << " fsd " << numFsd;
    numEventsInTs++;
    if (numEventsInTs > 9) break;
  }

  // --- Run statistics
  fNumTs++;
  fNumEvents += fEvents->size();
}
// ----------------------------------------------------------------------------


// -----   End-of-run action   ------------------------------------------------
void CbmTaskInspectDigiEvents::Finish()
{
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Timeslices : " << fNumTs;
  LOG(info) << "Events     : " << fNumEvents;
  LOG(info) << "=====================================";
}
// ----------------------------------------------------------------------------


// -----   Initialisation   ---------------------------------------------------
InitStatus CbmTaskInspectDigiEvents::Init()
{
  // --- Get FairRootManager instance
  FairRootManager* ioman = FairRootManager::Instance();
  assert(ioman);

  LOG(info) << "==================================================";
  LOG(info) << GetName() << ": Initialising...";

  // --- Input data
  fEvents = ioman->InitObjectAs<const std::vector<CbmDigiEvent>*>("DigiEvent");
  if (!fEvents) {
    LOG(error) << GetName() << ": No input branch DigiEvent!";
    return kFATAL;
  }
  LOG(info) << "--- Found branch DigiEvent at " << fEvents;

  LOG(info) << "==================================================";
  return kSUCCESS;
}
// ----------------------------------------------------------------------------


ClassImp(CbmTaskInspectDigiEvents)
