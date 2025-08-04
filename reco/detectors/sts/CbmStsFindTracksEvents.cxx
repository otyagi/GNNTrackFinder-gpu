/* Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

/** @file CbmFindTracksEvent.h
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 16.06.2014
 ** @date 23.10.2016
 **/


// Includes from STS
#include "CbmStsFindTracksEvents.h"

#include "CbmEvent.h"
#include "CbmStsHit.h"
#include "CbmStsTrack.h"
#include "CbmStsTrackFinderIdeal.h"

#include <cassert>

// Includes from base
#include "FairField.h"
#include "FairRootManager.h"
#include "FairRunAna.h"
#include "FairRuntimeDb.h"

#include <Logger.h>

// Includes from ROOT
#include "TClonesArray.h"

// Includes from C++
#include <iomanip>
#include <iostream>

using std::cout;
using std::endl;
using std::fixed;
using std::left;
using std::pair;
using std::right;
using std::setprecision;
using std::setw;
using std::stringstream;

// -----   Standard constructor   ------------------------------------------
CbmStsFindTracksEvents::CbmStsFindTracksEvents(CbmStsTrackFinder* finder, Bool_t useMvd)
  : FairTask("StsFindTracksEvents")
  , fUseMvd(useMvd)
  , fFinder(finder)
  , fEvents(NULL)
  , fMvdHits(NULL)
  , fStsHits(NULL)
  , fTracks(NULL)
  , fTimer()
  , fNofEvents(0)
  , fNofHits(0.)
  , fNofTracks(0.)
  , fTime(0.)
{
  if (!finder) fFinder = new CbmStsTrackFinderIdeal();
}
// -------------------------------------------------------------------------


// -----   Destructor   ----------------------------------------------------
CbmStsFindTracksEvents::~CbmStsFindTracksEvents()
{
  fTracks->Delete();
  if (fFinder) delete fFinder;
}
// -------------------------------------------------------------------------


// -----   Task execution   ------------------------------------------------
void CbmStsFindTracksEvents::Exec(Option_t* /*opt*/)
{

  // --- Local variables
  Long64_t nEvents = 0;
  Long64_t nHits   = 0;
  Long64_t nTracks = 0;
  pair<UInt_t, UInt_t> result;
  TStopwatch timer;
  timer.Start();

  // --- Clear output array
  fTracks->Delete();

  // --- Event loop (from event objects)
  if (fEvents) {
    nEvents = fEvents->GetEntriesFast();
    LOG(debug) << GetName() << ": reading time slice with " << nEvents << " events ";
    for (Int_t iEvent = 0; iEvent < nEvents; iEvent++) {
      CbmEvent* event = static_cast<CbmEvent*>(fEvents->At(iEvent));
      result          = ProcessEvent(event);
      nHits += result.first;
      nTracks += result.second;
    }  //# events
  }    //? event branch present

  else {  // Timeslice reconstruction without events
    result  = ProcessEvent(nullptr);
    nHits   = result.first;
    nTracks = result.second;
  }

  // --- Timeslice log and statistics
  timer.Stop();
  stringstream logOut;
  logOut << setw(20) << left << GetName() << " [";
  logOut << fixed << setw(8) << setprecision(1) << right << timer.RealTime() * 1000. << " ms] ";
  logOut << "TS " << fNofTs;
  if (fEvents) logOut << ", events " << nEvents;
  logOut << ", hits " << nHits << ", tracks " << nTracks;
  LOG(info) << logOut.str();
  fNofTs++;
  fNofEvents += nEvents;
  fNofHits += nHits;
  fNofTracks += nTracks;
  fTime += timer.RealTime();
}
// -------------------------------------------------------------------------


// -----   Initialisation   ------------------------------------------------
InitStatus CbmStsFindTracksEvents::Init()
{

  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": initialising";

  // I/O manager
  FairRootManager* ioman = FairRootManager::Instance();
  assert(ioman);

  // --- Get input array (Events)
  fEvents = dynamic_cast<TClonesArray*>(ioman->GetObject("CbmEvent"));
  if (nullptr == fEvents) {
    LOG(warn) << GetName() << ": No event array! Will process entire tree.";
  }

  // --- Get input array (StsHits)
  fStsHits = (TClonesArray*) ioman->GetObject("StsHit");
  assert(fStsHits);

  // Array of MvdHits
  if (fUseMvd) {
    fMvdHits = (TClonesArray*) ioman->GetObject("MvdHit");
    if (fMvdHits == nullptr) {
      LOG(error) << GetName() << ": Use of MVD hits selected, but no hit branch present! "
                 << "Tracking will be done without MVD hits.";
    }
    else
      LOG(info) << GetName() << ": including MVD hits in tracking";
  }

  // Create and register output array for StsTracks
  fTracks = new TClonesArray("CbmStsTrack", 100);
  ioman->Register("StsTrack", "STS", fTracks, IsOutputBranchPersistent("StsTrack"));

  // Check for Track finder
  if (!fFinder) {
    LOG(fatal) << GetName() << ": no track finding engine selected!";
    return kERROR;
  }
  LOG(info) << GetName() << ": Use track finder " << fFinder->GetName();

  // Set members of track finder and initialise it
  fFinder->SetMvdHitArray(fMvdHits);
  fFinder->SetStsHitArray(fStsHits);
  fFinder->SetTrackArray(fTracks);
  fFinder->Init();

  // Screen output
  LOG(info) << GetName() << ": successfully initialised.";
  LOG(info) << "=====================================\n";

  return kSUCCESS;
}
// -------------------------------------------------------------------------


// -----   End-of-run action   ---------------------------------------------
void CbmStsFindTracksEvents::Finish()
{
  std::cout << std::endl;
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Time slices      : " << fNofTs;
  LOG(info) << "Hits   / TS      : " << fixed << setprecision(2) << Double_t(fNofHits) / Double_t(fNofTs);
  LOG(info) << "Tracks / TS      : " << fixed << setprecision(2) << Double_t(fNofTracks) / Double_t(fNofTs);
  LOG(info) << "Hits per track   : " << fNofHits / fNofTracks;
  LOG(info) << "Time per TS      : " << 1000. * fTime / Double_t(fNofTs) << " ms ";
  if (fEvents) {
    LOG(info) << "Events processed : " << fNofEvents;
    LOG(info) << "Hits / event     : " << fNofHits / Double_t(fNofEvents);
    LOG(info) << "Tracks / event   : " << fNofTracks / Double_t(fNofEvents);
  }
  LOG(info) << "=====================================";
}
// -------------------------------------------------------------------------


// ------   Process one event   --------------------------------------------
pair<UInt_t, UInt_t> CbmStsFindTracksEvents::ProcessEvent(CbmEvent* event)
{

  // --- Call track finder
  fTimer.Start();
  Int_t nTracks = fFinder->FindTracks(event);
  fFinder->FillEloss();
  fTimer.Stop();

  // --- Event log
  Int_t eventNumber = (event ? event->GetNumber() : fNofEvents);
  Int_t nHits       = (event ? event->GetNofData(ECbmDataType::kStsHit) : fStsHits->GetEntriesFast());
  LOG(debug) << "+ " << setw(20) << GetName() << ": Event " << setw(6) << right << eventNumber << ", real time "
             << fixed << setprecision(6) << fTimer.RealTime() << " s, hits: " << nHits << ", tracks: " << nTracks;

  return std::make_pair(nHits, nTracks);
}
// -------------------------------------------------------------------------


ClassImp(CbmStsFindTracksEvents)
