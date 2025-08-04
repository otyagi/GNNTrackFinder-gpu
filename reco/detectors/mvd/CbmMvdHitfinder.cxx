/* Copyright (C) 2014-2021 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Philipp Sitzmann [committer], Florian Uhlig */

// -------------------------------------------------------------------------
// -----                    CbmMvdHitfinder source file                -----
// -------------------------------------------------------------------------
#include "CbmMvdHitfinder.h"

#include "CbmDefs.h"                    // for ECbmModuleId
#include "CbmDigiManager.h"             // for CbmDigiManager
#include "CbmEvent.h"                   // for CbmEvent
#include "CbmMvdCluster.h"              // for CbmMvdCluster
#include "CbmMvdDetector.h"             // for CbmMvdDetector
#include "CbmMvdDetectorId.h"           // for CbmMvdDetector
#include "CbmMvdDigi.h"                 // for CbmMvdDigi
#include "CbmMvdSensor.h"               // for CbmMvdSensor
#include "CbmMvdSensorFindHitTask.h"    // for CbmMvdSensorFindHi...
#include "CbmMvdSensorHitfinderTask.h"  // for CbmMvdSensorHitfin...

#include <FairRootManager.h>  // for FairRootManager
#include <FairTask.h>         // for InitStatus, FairTask
#include <Logger.h>           // for Logger, LOG

#include <TClonesArray.h>  // for TClonesArray

#include <iomanip>   // for setprecision, setw
#include <iostream>  // for operator<<, endl
#include <map>       // for allocator, __map_i...
#include <sstream>   // for stringstream;
#include <utility>   // for pair

using std::endl;
using std::fixed;
using std::ios_base;
using std::right;
using std::setprecision;
using std::setw;
using std::stringstream;

// -----   Default constructor   ------------------------------------------
CbmMvdHitfinder::CbmMvdHitfinder()
  : FairTask("MVDHitfinder")
  , fDetector(nullptr)
  , fDigiMan(nullptr)
  , fInputCluster(nullptr)
  , fHits(nullptr)
  , fHitfinderPluginNr(0)
  , fUseClusterfinder(kFALSE)
  , fShowDebugHistos(kFALSE)
  , fTimer()
  , fmode(-1)
{
}
// -------------------------------------------------------------------------

// -----   Standard constructor   ------------------------------------------
CbmMvdHitfinder::CbmMvdHitfinder(const char* name, Int_t iVerbose)
  : FairTask(name, iVerbose)
  , fDetector(nullptr)
  , fDigiMan(nullptr)
  , fInputCluster(nullptr)
  , fHits(nullptr)
  , fHitfinderPluginNr(0)
  , fUseClusterfinder(kFALSE)
  , fShowDebugHistos(kFALSE)
  , fTimer()
  , fmode(-1)
{
}
// -------------------------------------------------------------------------

// -----   Standard constructor   ------------------------------------------
CbmMvdHitfinder::CbmMvdHitfinder(const char* name, Int_t mode, Int_t iVerbose)
  : FairTask(name, iVerbose)
  , fDetector(nullptr)
  , fDigiMan(nullptr)
  , fInputCluster(nullptr)
  , fHits(nullptr)
  , fHitfinderPluginNr(0)
  , fUseClusterfinder(kFALSE)
  , fShowDebugHistos(kFALSE)
  , fTimer()
  , fmode(mode)
{
  //    fmode = mode;
}
// -------------------------------------------------------------------------

// -----   Destructor   ----------------------------------------------------
CbmMvdHitfinder::~CbmMvdHitfinder()
{

  if (fHits) {
    fHits->Delete();
    delete fHits;
  }
}
// -----------------------------------------------------------------------------

// -----   Exec   --------------------------------------------------------------
void CbmMvdHitfinder::Exec(Option_t* /*opt*/)
{

  using namespace std;

  fHits->Clear();
  fTimer.Start();

  if (fDigiMan->IsPresent(ECbmModuleId::kMvd) || fInputCluster) {  //checks if data sources are available
    if (fVerbose) LOG(debug) << "//----------------------------------------//";
    if (!fUseClusterfinder) {
      LOG(fatal) << GetName() << "- Mode without cluster finder is currently not supported ";
    }
  }


  LOG(debug) << "CbmMvdClusterfinder::Exec : Starting Exec ";

  if (fEventMode == ECbmRecoMode::Timeslice) {
    ProcessData(nullptr);
  }
  else {
    assert(fEvents);
    Int_t nEvents = fEvents->GetEntriesFast();
    LOG(debug) << setw(20) << left << GetName() << ": Processing time slice " << fNofTs << " with " << nEvents
               << (nEvents == 1 ? " event" : " events");
    for (Int_t iEvent = 0; iEvent < nEvents; iEvent++) {
      CbmEvent* event = dynamic_cast<CbmEvent*>(fEvents->At(iEvent));
      assert(event);
      Int_t hitsBeforeEvent = fHits->GetEntriesFast();
      LOG(debug) << "Number of hits before executing the event: " << hitsBeforeEvent;
      ProcessData(event);
      // when running in event based mode add the MvdCluster to the event
      if (event) {
        for (Int_t iHit = hitsBeforeEvent; iHit < fHits->GetEntriesFast(); ++iHit) {
          event->AddData(ECbmDataType::kMvdHit, iHit);
        }
        LOG(debug) << "The current event contains " << event->GetNofData(ECbmDataType::kMvdHit) << " MVD hits";
      }
    }
  }

  fTimer.Stop();
  stringstream logOut;
  logOut << setw(20) << left << GetName() << " [";
  logOut << fixed << setw(8) << setprecision(1) << right << fTimer.RealTime() * 1000. << " ms] ";
  logOut << "TS " << fNofTs;
  if (fEvents) logOut << ", events " << fEvents->GetEntriesFast();
  logOut << ", clusters " << fInputCluster->GetEntriesFast();
  logOut << ", hits " << fHits->GetEntriesFast();
  LOG(info) << logOut.str();

  fNofTs++;
}


void CbmMvdHitfinder::ProcessData(CbmEvent* event)
{

  Int_t nCluster = (event ? event->GetNofData(ECbmDataType::kMvdCluster) : fInputCluster->GetEntriesFast());

  if (nCluster > 0) {
    if (fVerbose) LOG(debug) << "//----------------------------------------//";
    if (fVerbose) LOG(debug) << "Send Input";

    Int_t nTargetPlugin = fDetector->DetectPlugin(fMyPluginID);

    LOG(debug) << "CbmMvdHitfinder::Exec - nDigis= " << nCluster;

    // Since the clusters are distributed to several sensor tasks which creates
    // several new arrays, the array index can't be used to link the proper
    // digi information in the tasks to be used inside the task to link the
    // correct MC information
    // Add the index in the original array to to the digi itself
    for (Int_t iCluster = 0; iCluster < nCluster; ++iCluster) {
      CbmMvdDetectorId tmp;
      Int_t clusterIndex     = (event ? event->GetIndex(ECbmDataType::kMvdCluster, iCluster) : iCluster);
      CbmMvdCluster* cluster = static_cast<CbmMvdCluster*>(fInputCluster->At(clusterIndex));
      cluster->SetRefId(clusterIndex);

      fDetector->SendInputToSensorPlugin(tmp.DetectorId(cluster->GetSensorNr()), nTargetPlugin,
                                         static_cast<TObject*>(cluster));
    }
    if (fVerbose) LOG(debug) << "Execute HitfinderPlugin Nr. " << fHitfinderPluginNr;


    fDetector->Exec(fHitfinderPluginNr);
    if (fVerbose) LOG(debug) << "End Chain";
    if (fVerbose) LOG(debug) << "Start writing Hits";
    fDetector->GetOutputArray(nTargetPlugin, fHits);

    //fDetector->GetMatchArray (nTargetPlugin, fTmpMatch);
    //fHits->AbsorbObjects(fDetector->GetOutputHits(), 0, fDetector->GetOutputHits()->GetEntriesFast() - 1);
  }
}
// -----------------------------------------------------------------------------

// -----   Init   --------------------------------------------------------------
InitStatus CbmMvdHitfinder::Init()
{

  using namespace std;

  LOG(info) << GetName() << ": Initialisation...";

  // **********  RootManager
  FairRootManager* ioman = FairRootManager::Instance();
  if (!ioman) {
    LOG(error) << GetName() << "::Init: No FairRootManager!";
    return kFATAL;
  }

  // **********  Get input arrays
  if (!fUseClusterfinder) {
    LOG(fatal) << GetName() << " - Mode without cluster finder is currently not supported ";
    fDigiMan = CbmDigiManager::Instance();
    fDigiMan->Init();
    if (!fDigiMan->IsPresent(ECbmModuleId::kMvd)) {
      LOG(error) << "No MvdDigi branch found. There was no MVD in the "
                    "simulation. Switch this task off";
      return kERROR;
    }
  }
  else {
    fInputCluster = (TClonesArray*) ioman->GetObject("MvdCluster");
    if (!fInputCluster) {
      LOG(error) << "No MvdCluster branch found. There was no MVD in the "
                    "simulation. Switch this task off";
      return kERROR;
    }
  }

  // --- In event mode: get input array (CbmEvent)
  if (fEventMode == ECbmRecoMode::EventByEvent) {
    LOG(info) << GetName() << ": Using event-by-event mode";
    fEvents = dynamic_cast<TClonesArray*>(ioman->GetObject("CbmEvent"));
    if (nullptr == fEvents) {
      LOG(warn) << GetName() << ": Event mode selected but no event array found!";
      return kFATAL;
    }  //? Event branch not present
  }    //? Event mode
  else {
    LOG(info) << GetName() << ": Using time-based mode";
  }

  // **********  Register output array
  fHits = new TClonesArray("CbmMvdHit", 1000);
  ioman->Register("MvdHit", "Mvd Hits", fHits, IsOutputBranchPersistent("MvdHit"));

  fDetector = CbmMvdDetector::Instance();


  // Add the hit finder plugin to all sensors
  std::map<int, CbmMvdSensor*>& sensorMap = fDetector->GetSensorMap();
  UInt_t plugincount                      = fDetector->GetPluginCount();

  /*
  if (!fUseClusterfinder) {

    for (auto itr = sensorMap.begin(); itr != sensorMap.end(); itr++) {
      CbmMvdSensorFindHitTask* hitfinderTask = new CbmMvdSensorFindHitTask();

      itr->second->AddPlugin(hitfinderTask);
      itr->second->SetHitPlugin(plugincount);
      fMyPluginID=400;

    }

  else {*/
  for (auto itr = sensorMap.begin(); itr != sensorMap.end(); itr++) {
    CbmMvdSensorHitfinderTask* hitfinderTask = new CbmMvdSensorHitfinderTask();

    itr->second->AddPlugin(hitfinderTask);
    itr->second->SetHitPlugin(plugincount);
    fMyPluginID = 300;
  }
  //}

  fDetector->SetSensorArrayFilled(kTRUE);
  fDetector->SetPluginCount(plugincount + 1);
  fHitfinderPluginNr = (UInt_t)(fDetector->GetPluginArraySize());

  if (fShowDebugHistos) fDetector->ShowDebugHistos();
  fDetector->Init();


  // Screen output
  LOG(info) << GetName() << " initialised with parameters: ";
  //PrintParameters();

  return kSUCCESS;
}

// -----   Virtual public method Reinit   ----------------------------------
InitStatus CbmMvdHitfinder::ReInit() { return kSUCCESS; }
// -------------------------------------------------------------------------


// -----   Virtual method Finish   -----------------------------------------
void CbmMvdHitfinder::Finish() { PrintParameters(); }
// -------------------------------------------------------------------------


// -----   Private method Reset   ------------------------------------------
void CbmMvdHitfinder::Reset() { fHits->Delete(); }
// -------------------------------------------------------------------------

// -----   Private method GetMvdGeometry   ---------------------------------
void CbmMvdHitfinder::GetMvdGeometry() {}
// -------------------------------------------------------------------------

// -----   Private method PrintParameters   --------------------------------
void CbmMvdHitfinder::PrintParameters() const { LOG(info) << ParametersToString(); }

// -----   Private method ParametersToString   -----------------------------
std::string CbmMvdHitfinder::ParametersToString() const
{
  std::stringstream ss;
  ss.setf(std::ios_base::fixed, std::ios_base::floatfield);
  ss << "============================================================" << endl;
  ss << "============== Parameters MvdHitfinder =====================" << endl;
  ss << "============================================================" << endl;
  ss << "=============== End Task ===================================" << endl;
  return ss.str();
}
// -------------------------------------------------------------------------

ClassImp(CbmMvdHitfinder);
