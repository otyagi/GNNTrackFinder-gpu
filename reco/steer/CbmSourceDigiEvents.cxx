/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese[committer] */

#include "CbmSourceDigiEvents.h"

#include <FairRootManager.h>
#include <Logger.h>

#include <utility>


// -----   Constructor   ------------------------------------------------------
CbmSourceDigiEvents::CbmSourceDigiEvents(const char* fileName) : fInputFileName(fileName) {}
// ----------------------------------------------------------------------------


// -----   Close   ------------------------------------------------------------
void CbmSourceDigiEvents::Close()
{
  LOG(info) << "Source: Closing after " << fNumTs << "timeslices with " << fNumEvents << "events.";
}
// ----------------------------------------------------------------------------


// -----   Initialisation   ---------------------------------------------------
Bool_t CbmSourceDigiEvents::Init()
{

  // Create input archive
  fArchive = std::make_unique<cbm::algo::RecoResultsInputArchive>(fInputFileName);
  LOG(info) << "Source: Reading from input archive " << fInputFileName;
  auto desc = fArchive->descriptor();
  LOG(info) << "      - Time created: " << desc.time_created();
  LOG(info) << "      - Host name   : " << desc.hostname();
  LOG(info) << "      - User name   : " << desc.username();

  // Create and register the DigiEvent tree branch
  FairRootManager* ioman = FairRootManager::Instance();
  assert(ioman);
  if (ioman->GetObject("DigiEvent")) {
    LOG(fatal) << "Source: Branch DigiEvent already exists!";
    return kFALSE;
  }
  fEvents = new std::vector<CbmDigiEvent>();
  ioman->RegisterAny("DigiEvent", fEvents, true);
  LOG(info) << "Source: Registered branch DigiEvent at " << fEvents;

  return kTRUE;
}
// ----------------------------------------------------------------------------


// -----   Read one time slice from archive   ---------------------------------
Int_t CbmSourceDigiEvents::ReadEvent(UInt_t)
{

  // Clear output event vector
  fEvents->clear();

  // Get next timeslice data from archive. Stop run if end of archive is reached.
  auto results = fArchive->get();
  if (fArchive->eos()) {
    LOG(info) << "Source: End of input archive; terminating run";
    return 1;
  }

  // Move event data from input archive to ROOT tree
  if (results == nullptr) {
    LOG(error) << "Source: Failed to read RecoResults from archive";
    return 1;
  }
  size_t numEvents = results->DigiEvents().size();
  LOG(info);
  LOG(info) << "Source: Reading TS " << fNumTs << ", index " << results->TsIndex() << ", start "
            << results->TsStartTime() << ", events " << numEvents;
  std::move(results->DigiEvents().begin(), results->DigiEvents().end(), std::back_inserter(*fEvents));
  assert(fEvents->size() == numEvents);

  // Update counters
  fNumTs++;
  fNumEvents += numEvents;

  return 0;
}
// ----------------------------------------------------------------------------


ClassImp(CbmSourceDigiEvents)
