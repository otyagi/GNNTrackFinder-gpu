/* Copyright (C) 2014-2020 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Philipp Sitzmann [committer] */

// -------------------------------------------------------------------------
// -----                    CbmMvdClusterfinder source file                -----
// -------------------------------------------------------------------------
#include "CbmMvdClusterfinder.h"

#include "CbmDefs.h"                        // for ECbmModuleId
#include "CbmDigiManager.h"                 // for CbmDigiManager
#include "CbmEvent.h"                       // for CbmEvent
#include "CbmMvdDetector.h"                 // for CbmMvdDetector
#include "CbmMvdDigi.h"                     // for CbmMvdDigi
#include "CbmMvdSensor.h"                   // for CbmMvdSensor
#include "CbmMvdSensorClusterfinderTask.h"  // for CbmMvdSensorCluste...

#include <FairRootManager.h>  // for FairRootManager
#include <FairTask.h>         // for InitStatus, FairTask
#include <Logger.h>           // for Logger, LOG

#include <TClonesArray.h>  // for TClonesArray

#include <iomanip>   // for setprecision, setw
#include <iostream>  // for operator<<, endl
#include <map>       // for allocator, __map_i...
#include <sstream>   // for stringstream
#include <utility>   // for pair

using std::endl;
using std::fixed;
using std::left;
using std::right;
using std::setprecision;
using std::setw;
using std::stringstream;

// -----   Default constructor   ------------------------------------------
CbmMvdClusterfinder::CbmMvdClusterfinder()
  : FairTask("MVDClusterfinder")
  , fMode(0)
  , fShowDebugHistos(kFALSE)
  , fDetector(nullptr)
  , fDigiMan(nullptr)
  , fCluster(nullptr)
  , fClusterPluginNr()
  , fBranchName("")
  , fTimer()
{
}
// -------------------------------------------------------------------------

// -----   Standard constructor   ------------------------------------------
CbmMvdClusterfinder::CbmMvdClusterfinder(const char* name, Int_t iMode, Int_t iVerbose)
  : FairTask(name, iVerbose)
  , fMode(iMode)
  , fShowDebugHistos(kFALSE)
  , fDetector(nullptr)
  , fDigiMan(nullptr)
  , fCluster(nullptr)
  , fClusterPluginNr(0)
  , fBranchName("MvdDigi")
  , fTimer()
{
}
// -------------------------------------------------------------------------

// -----   Destructor   ----------------------------------------------------
CbmMvdClusterfinder::~CbmMvdClusterfinder()
{

  if (fCluster) {
    fCluster->Delete();
    delete fCluster;
  }
}
// -----------------------------------------------------------------------------

// -----   Exec   --------------------------------------------------------------
void CbmMvdClusterfinder::Exec(Option_t* /*opt*/)
{

  // --- Start timer
  fTimer.Start();
  LOG(debug) << "CbmMvdClusterfinder::Exec : Starting Exec ";
  fCluster->Delete();

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
      Int_t clustersBeforeEvent = fCluster->GetEntriesFast();
      LOG(debug) << "Number of clusters before executing the event: " << clustersBeforeEvent;
      ProcessData(event);
      // when running in event based mode add the MvdCluster to the event
      if (event) {
        for (Int_t iCluster = clustersBeforeEvent; iCluster < fCluster->GetEntriesFast(); ++iCluster) {
          event->AddData(ECbmDataType::kMvdCluster, iCluster);
        }
        LOG(debug) << "The current event contains " << event->GetNofData(ECbmDataType::kMvdCluster) << " MVD clusters";
      }
    }
  }

  fTimer.Stop();

  stringstream logOut;
  logOut << setw(20) << left << GetName() << " [";
  logOut << fixed << setw(8) << setprecision(1) << right << fTimer.RealTime() * 1000. << " ms] ";
  logOut << "TS " << fNofTs;
  if (fEvents) logOut << ", events " << fEvents->GetEntriesFast();
  logOut << ", digis " << fDigiMan->GetNofDigis(ECbmModuleId::kMvd);
  logOut << ", clusters " << fCluster->GetEntriesFast();
  LOG(info) << logOut.str();

  fNofTs++;
}

void CbmMvdClusterfinder::ProcessData(CbmEvent* event)
{

  Int_t nDigis = (event ? event->GetNofData(ECbmDataType::kMvdDigi) : fDigiMan->GetNofDigis(ECbmModuleId::kMvd));

  if (nDigis > 0) {
    if (fVerbose) LOG(debug) << "//----------------------------------------//";
    if (fVerbose) LOG(debug) << "Send Input";

    Int_t nTargetPlugin = fDetector->DetectPlugin(200);

    LOG(debug) << "CbmMvdClusterfinder::Exec - nDigis= " << nDigis;

    // Since the digis are distributed to several sensor tasks which creates
    // several new arrays, the array index can't be used to link the proper
    // digi information in the tasks to be used inside the task to link the
    // correct MC information
    // Add the index in the original array to to the digi itself
    for (Int_t iDigi = 0; iDigi < nDigis; iDigi++) {
      Int_t digiIndex  = (event ? event->GetIndex(ECbmDataType::kMvdDigi, iDigi) : iDigi);
      CbmMvdDigi* digi = new CbmMvdDigi(*(fDigiMan->Get<CbmMvdDigi>(digiIndex)));
      digi->SetRefId(digiIndex);

      fDetector->SendInputToSensorPlugin(digi->GetDetectorId(), nTargetPlugin, static_cast<TObject*>(digi));
    }

    LOG(debug) << "CbmMvdClusterfinder::Exec : Communicating with Plugin: " << nTargetPlugin;

    //fDetector->SendInputDigis(fDigiMan);
    if (fVerbose) LOG(debug) << "Execute ClusterPlugin Nr. " << fClusterPluginNr;
    fDetector->Exec(nTargetPlugin);
    if (fVerbose) LOG(debug) << "End Chain";
    if (fVerbose) LOG(debug) << "Start writing Cluster";

    // Get the data from all sensors
    fDetector->GetOutputArray(nTargetPlugin, fCluster);

    //fDetector->GetMatchArray (nTargetPlugin, fTmpMatch);
    //fCluster->AbsorbObjects(fDetector->GetOutputCluster(), 0, fDetector->GetOutputCluster()->GetEntriesFast() - 1);
  }
}
// -----------------------------------------------------------------------------

// -----   Init   --------------------------------------------------------------
InitStatus CbmMvdClusterfinder::Init()
{
  LOG(info) << GetName() << ": Initialisation...";

  // **********  RootManager
  FairRootManager* ioman = FairRootManager::Instance();
  if (!ioman) {
    LOG(error) << GetName() << "::Init: No FairRootManager!";
    return kFATAL;
  }

  // **********  Get input array
  fDigiMan = CbmDigiManager::Instance();
  fDigiMan->Init();
  if (!fDigiMan->IsPresent(ECbmModuleId::kMvd)) {
    LOG(error) << "No MvdDigi branch found. There was no MVD in the "
                  "simulation. Switch this task off";
    return kERROR;
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
  fCluster = new TClonesArray("CbmMvdCluster", 10000);
  ioman->Register("MvdCluster", "Mvd Clusters", fCluster, IsOutputBranchPersistent("MvdCluster"));

  fDetector = CbmMvdDetector::Instance();

  if (fDetector->GetSensorArraySize() > 1) {
    if (fVerbose) LOG(info) << "succesfully loaded Geometry from file";
  }
  else {
    LOG(fatal) << "Geometry couldn't be loaded from file. No MVD digitizer available.";
  }

  // Add the cluster finder plugin to all sensors
  std::map<int, CbmMvdSensor*>& sensorMap = fDetector->GetSensorMap();
  UInt_t plugincount                      = fDetector->GetPluginCount();

  for (auto itr = sensorMap.begin(); itr != sensorMap.end(); itr++) {
    CbmMvdSensorClusterfinderTask* clusterTask = new CbmMvdSensorClusterfinderTask();

    itr->second->AddPlugin(clusterTask);
    itr->second->SetClusterPlugin(plugincount);
  }
  fDetector->SetSensorArrayFilled(kTRUE);
  fDetector->SetPluginCount(plugincount + 1);
  fClusterPluginNr = (UInt_t)(fDetector->GetPluginArraySize());

  if (fShowDebugHistos) fDetector->ShowDebugHistos();
  fDetector->Init();


  // Screen output
  LOG(info) << GetName() << " initialised with parameters: ";
  //PrintParameters();


  return kSUCCESS;
}

// -----   Virtual public method Reinit   ----------------------------------
InitStatus CbmMvdClusterfinder::ReInit() { return kSUCCESS; }
// -------------------------------------------------------------------------


// -----   Virtual method Finish   -----------------------------------------
void CbmMvdClusterfinder::Finish()
{
  fDetector->Finish();
  PrintParameters();
}
// -------------------------------------------------------------------------

// -----   Private method Reset   ------------------------------------------
void CbmMvdClusterfinder::Reset() { fCluster->Delete(); }
// -------------------------------------------------------------------------

// -----   Private method GetMvdGeometry   ---------------------------------
void CbmMvdClusterfinder::GetMvdGeometry() {}
// -------------------------------------------------------------------------

// -----   Private method PrintParameters   --------------------------------
void CbmMvdClusterfinder::PrintParameters() const { LOG(info) << ParametersToString(); }

// -----   Private method ParametersToString   -----------------------------
std::string CbmMvdClusterfinder::ParametersToString() const
{
  std::stringstream ss;
  ss << "============================================================" << endl;
  ss << "============== Parameters Clusterfinder ====================" << endl;
  ss << "============================================================" << endl;
  ss << "=============== End Task ===================================" << endl;
  return ss.str();
}
// -------------------------------------------------------------------------


ClassImp(CbmMvdClusterfinder);
