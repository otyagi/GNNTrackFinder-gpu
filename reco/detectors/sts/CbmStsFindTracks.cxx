/* Copyright (C) 2005-2019 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Denis Bertini [committer], Florian Uhlig */

// -------------------------------------------------------------------------
// -----                   CbmStsFindTracks source file                -----
// -----                  Created 02/02/05  by V. Friese               -----
// -------------------------------------------------------------------------
// Includes from STS
#include "CbmStsFindTracks.h"

#include "CbmStsHit.h"
#include "CbmStsTrack.h"
#include "CbmStsTrackFinderIdeal.h"

// Includes from base
#include "FairField.h"
#include "FairRootManager.h"
#include "FairRunAna.h"
#include "FairRuntimeDb.h"

// Includes from ROOT
#include "TClonesArray.h"

// Includes from C++
#include <iomanip>
#include <iostream>


using std::fixed;
using std::left;
using std::right;
using std::setprecision;
using std::setw;

// -----   Default constructor   -------------------------------------------
CbmStsFindTracks::CbmStsFindTracks()
  : FairTask("STSFindTracks")
  , fUseMvd(kFALSE)
  , fGeoPar(NULL)
  , fDigiPar(NULL)
  ,
  //   fDigiScheme(new CbmStsDigiScheme()),
  fField(NULL)
  , fFinder(new CbmStsTrackFinderIdeal(1))
  , fMvdHits(NULL)
  , fStsHits(NULL)
  , fTracks(NULL)
  , fTimer()
  , fNEvents(0)
  , fNEventsFailed(0)
  , fTime(0.)
  , fNTracks(0.)
{
  fVerbose = 1;
}
// -------------------------------------------------------------------------


// -----   Standard constructor   ------------------------------------------
CbmStsFindTracks::CbmStsFindTracks(Int_t iVerbose, CbmStsTrackFinder* finder, Bool_t useMvd, const char* name)
  : FairTask(name, iVerbose)
  , fUseMvd(useMvd)
  , fGeoPar(NULL)
  , fDigiPar(NULL)
  ,
  //   fDigiScheme(new CbmStsDigiScheme()),
  fField(NULL)
  , fFinder(NULL)
  , fMvdHits(NULL)
  , fStsHits(NULL)
  , fTracks(NULL)
  , fTimer()
  , fNEvents(0)
  , fNEventsFailed(0)
  , fTime(0.)
  , fNTracks(0.)
{
  if (finder)
    fFinder = finder;
  else
    fFinder = new CbmStsTrackFinderIdeal(iVerbose);
}
// -------------------------------------------------------------------------


// -----   Destructor   ----------------------------------------------------
CbmStsFindTracks::~CbmStsFindTracks()
{
  // if ( fDigiScheme ) delete fDigiScheme;
  fTracks->Delete();
  if (fFinder) delete fFinder;
}
// -------------------------------------------------------------------------


// -----   Virtual public method Exec   ------------------------------------
void CbmStsFindTracks::Exec(Option_t* /*opt*/)
{

  fTimer.Start();

  fTracks->Delete();
  Int_t nTracks = fFinder->DoFind();
  fFinder->FillEloss();
  //  for (Int_t iTrack=0; iTrack<fTracks->GetEntriesFast(); iTrack++) {
  //    CbmStsTrack* track = (CbmStsTrack*) fTracks->At(iTrack);
  //    track->SortHits();
  //  }

  fTimer.Stop();

  // --- Event log
  LOG(info) << "+ " << setw(20) << GetName() << ": Event " << setw(6) << right << fNEvents << ", real time " << fixed
            << setprecision(6) << fTimer.RealTime() << " s, hits: " << fStsHits->GetEntriesFast()
            << ", tracks: " << nTracks;


  fNEvents++;
  fTime += fTimer.RealTime();
  fNTracks += Double_t(nTracks);
}
// -------------------------------------------------------------------------


// -----   Private virtual method SetParContainers   -----------------------
void CbmStsFindTracks::SetParContainers()
{

  // Get run and runtime database
  FairRunAna* run = FairRunAna::Instance();
  if (!run) LOG(fatal) << GetName() << " SetParContainers: No analysis run";

  FairRuntimeDb* db = run->GetRuntimeDb();
  if (!db) LOG(fatal) << GetName() << " SetParContainers: No runtime database";

  // Get STS geometry parameter container
  fGeoPar = (CbmGeoStsPar*) db->getContainer("CbmGeoStsPar");

  // Get STS digitisation parameter container
  fDigiPar = (CbmStsDigiPar*) db->getContainer("CbmStsDigiPar");
}
// -------------------------------------------------------------------------


// -----   Private virtual method Init  ------------------------------------
InitStatus CbmStsFindTracks::Init()
{

  LOG(info) << "---------------------------------------------";
  LOG(info) << " Initialising " << GetName() << " ....";

  // Get input hit arrays
  FairRootManager* ioman = FairRootManager::Instance();
  if (!ioman) LOG(fatal) << GetName() << " Init: No FairRootManager";
  fStsHits = (TClonesArray*) ioman->GetObject("StsHit");
  if (!fStsHits) {
    LOG(error) << GetName() << "::Init: No StsHit array!";
    return kERROR;
  }
  if (fUseMvd) {
    fMvdHits = (TClonesArray*) ioman->GetObject("MvdHit");
    if (!fMvdHits) {
      LOG(warning) << GetName() << " Init: No MVD hits available!";
      LOG(warning) << GetName() << " Init:     Running track finding without MVD hits";
    }
  }

  // Create and register output array STSTrack
  fTracks = new TClonesArray("CbmStsTrack", 100);
  ioman->Register("StsTrack", "STS", fTracks, IsOutputBranchPersistent("StsTrack"));

  // Build digitisation scheme
  /*
  if ( fDigiScheme->Init(fGeoPar, fDigiPar) ) {
    if      (fVerbose == 1 || fVerbose == 2) fDigiScheme->Print(kFALSE);
    else if (fVerbose >  2) fDigiScheme->Print(kTRUE);
    cout << "-I- "
	 << "STS digitisation scheme succesfully initialised" << endl;
    cout << "    Stations: " << fDigiScheme->GetNStations()
	 << ", Sectors: " << fDigiScheme->GetNSectors() << ", Channels: "
	 << fDigiScheme->GetNChannels() << endl;
  }
	 */

  // Check for Track finder
  if (!fFinder) {
    LOG(error) << GetName() << "::Init: No track finder selected!";
    return kERROR;
  }
  LOG(info) << GetName() << " Track finder engine " << fFinder->GetName() << " selected";

  // Set members of track finder and verbosity and initialise track finder
  //fFinder->SetDigiScheme(fDigiScheme);
  fFinder->SetField(fField);
  fFinder->SetMvdHitArray(fMvdHits);
  fFinder->SetStsHitArray(fStsHits);
  fFinder->SetTrackArray(fTracks);
  fFinder->SetVerbose(fVerbose);
  fFinder->Init();

  // Screen output
  LOG(info) << GetName() << " intialised ";
  LOG(info) << "---------------------------------------------";

  return kSUCCESS;
}
// -------------------------------------------------------------------------


// -----   Virtual private method Finish   ---------------------------------
void CbmStsFindTracks::Finish()
{

  fFinder->Finish();

  LOG(info);
  LOG(info) << "============================================================";
  LOG(info) << "=====   " << GetName() << ": Run summary ";
  LOG(info) << "===== ";
  LOG(info) << "===== Good events   : " << setw(6) << fNEvents;
  LOG(info) << "===== Failed events : " << setw(6) << fNEventsFailed;
  LOG(info) << "===== Average time  : " << setprecision(4) << setw(8) << right << fTime / Double_t(fNEvents) << " s";
  LOG(info) << "===== ";
  LOG(info) << "===== Found tracks per event  : " << fixed << setprecision(0) << fNTracks / Double_t(fNEvents);
  LOG(info) << "============================================================";
}
// -------------------------------------------------------------------------


ClassImp(CbmStsFindTracks)
