/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Lukas Chlad [committer] */

#include "CbmFsdHitProducer.h"

#include "CbmDigiManager.h"
#include "CbmEvent.h"
#include "CbmFsdAddress.h"
#include "CbmFsdDetectorSpecs.h"
#include "CbmFsdDigi.h"
#include "CbmFsdGeoHandler.h"
#include "CbmFsdHit.h"

#include <FairRootManager.h>
#include <Logger.h>

#include <TClonesArray.h>
#include <TStopwatch.h>
#include <TVector3.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <tuple>

using std::cout;
using std::endl;
using std::fixed;
using std::left;
using std::pair;
using std::right;
using std::setprecision;
using std::setw;
using std::stringstream;


// -----   Default constructor   -------------------------------------------
CbmFsdHitProducer::CbmFsdHitProducer() : FairTask("FsdHitProducer") {}
// -------------------------------------------------------------------------


// -----   Destructor   ----------------------------------------------------
CbmFsdHitProducer::~CbmFsdHitProducer()
{
  if (fHitArray) {
    fHitArray->Delete();
    delete fHitArray;
  }
}
// -------------------------------------------------------------------------


// -----   Public method Init   --------------------------------------------
InitStatus CbmFsdHitProducer::Init()
{
  std::cout << std::endl << std::endl;
  LOG(info) << "=======   " << GetName() << ": Init   =====================";

  // --- Initialize the mapping between loaded Geometry and ModuleData
  CbmFsdGeoHandler::GetInstance().InitMaps();  // singleton first instance initialization of geometry to maps

  // --- Get RootManager
  FairRootManager* ioman = FairRootManager::Instance();
  if (!ioman) {
    LOG(fatal) << "-W- CbmFsdHitProducer::Init: RootManager not instantised!";  //FLORIAN & SELIM
    return kFATAL;
  }

  // --- Get event array, if present
  fEvents = dynamic_cast<TClonesArray*>(ioman->GetObject("Event"));
  if (fEvents)
    LOG(info) << GetName() << ": found Event branch, run event-by-event";
  else {
    fEvents = dynamic_cast<TClonesArray*>(ioman->GetObject("CbmEvent"));
    if (fEvents) LOG(info) << GetName() << ": found CbmEvent branch, run event-by-event";
  }
  if (!fEvents) {
    LOG(info) << GetName() << ": no event branch found; run in time-based mode";
    LOG(warn) << "*** Note that the current FSD hit producer is not suited for time-based reconstruction!";
    LOG(warn) << "*** Unless you have run the simulation event-by-event, it will not produce sensible results.";
  }

  // --- Initialise digi manager
  fDigiMan = CbmDigiManager::Instance();
  fDigiMan->Init();
  if (!fDigiMan->IsPresent(ECbmModuleId::kFsd)) {
    LOG(error) << GetName() << ": No FsdDigi input array present; "
               << "task will be inactive.";
    return kERROR;
  }
  LOG(info) << GetName() << ": found FsdDigi branch";

  // --- Create and register output array
  fHitArray = new TClonesArray("CbmFsdHit", 1000);
  ioman->Register("FsdHit", "FSD", fHitArray, IsOutputBranchPersistent("FsdHit"));
  fHitArray->Dump();
  LOG(info) << GetName() << ": registered branch FsdHit";
  LOG(info) << GetName() << ": intialisation successfull";
  LOG(info) << "======================================================\n";

  return kSUCCESS;
}
// -------------------------------------------------------------------------


// -----   Public method Exec   --------------------------------------------
void CbmFsdHitProducer::Exec(Option_t* /*opt*/)
{

  // --- Timer
  TStopwatch timer;
  timer.Start();

  // --- Reset output array
  Reset();

  // --- Local variables
  pair<Int_t, Int_t> result;
  Int_t nEvents   = 0;
  Int_t nDigis    = 0;
  Int_t nHits     = 0;
  Int_t nDigisAll = fDigiMan->GetNofDigis(ECbmModuleId::kFsd);
  CbmEvent* event = nullptr;

  // --- Time-slice mode: process entire digi array
  if (!fEvents) {
    result = ProcessData(nullptr);
    nDigis = result.first;
    nHits  = result.second;
  }

  // --- Event mode: loop over events and process digis within
  else {
    assert(fEvents);
    nEvents = fEvents->GetEntriesFast();
    for (Int_t iEvent = 0; iEvent < nEvents; iEvent++) {
      event  = static_cast<CbmEvent*>(fEvents->At(iEvent));
      result = ProcessData(event);
      LOG(debug) << GetName() << ": Event " << iEvent << " / " << nEvents << ", digis " << result.first << ", hits "
                 << result.second;
      nDigis += result.first;
      nHits += result.second;
    }  //# events
  }    //? event mode

  // --- Timeslice log and statistics
  timer.Stop();
  stringstream logOut;
  logOut << setw(20) << left << GetName() << " [";
  logOut << fixed << setw(8) << setprecision(1) << right << timer.RealTime() * 1000. << " ms] ";
  logOut << "TS " << fNofTs;
  if (fEvents) logOut << ", events " << nEvents;
  logOut << ", digis " << nDigis << " / " << nDigisAll;
  logOut << ", hits " << nHits;
  LOG(info) << logOut.str();
  fNofTs++;
  fNofEvents += nEvents;
  fNofDigis += nDigis;
  fNofHits += nHits;
  fTimeTot += timer.RealTime();
}
// -------------------------------------------------------------------------


// -----   End-of-timeslice action   ---------------------------------------
void CbmFsdHitProducer::Finish()
{

  std::cout << std::endl;
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Time slices     : " << fNofTs;
  LOG(info) << "Digis / TS      : " << fixed << setprecision(2) << Double_t(fNofDigis) / Double_t(fNofTs);
  LOG(info) << "Hits  / TS      : " << fixed << setprecision(2) << Double_t(fNofHits) / Double_t(fNofTs);
  LOG(info) << "Time  / TS      : " << fixed << setprecision(2) << 1000. * fTimeTot / Double_t(fNofTs) << " ms";
  if (fEvents) {
    LOG(info) << "Events          : " << fNofEvents;
    LOG(info) << "Events / TS     : " << fixed << setprecision(2) << Double_t(fNofEvents) / Double_t(fNofTs);
    LOG(info) << "Digis  / event  : " << fixed << setprecision(2) << Double_t(fNofDigis) / Double_t(fNofEvents);
    LOG(info) << "Hits   / event  : " << fixed << setprecision(2) << Double_t(fNofHits) / Double_t(fNofEvents);
  }
  LOG(info) << "=====================================\n";
}
// -------------------------------------------------------------------------


// -----   Data processing   -----------------------------------------------
pair<Int_t, Int_t> CbmFsdHitProducer::ProcessData(CbmEvent* event)
{

  // --- Local variables
  std::map<uint32_t, std::tuple<Double_t, Double_t, Int_t>>
    edeptimemap;  // sum of energy deposition, earliest time & digi index per address, key is digi address
  Int_t nHits            = 0;
  const CbmFsdDigi* digi = nullptr;
  Int_t nDigis = (event ? event->GetNofData(ECbmDataType::kFsdDigi) : fDigiMan->GetNofDigis(ECbmModuleId::kFsd));

  // --- Loop over FsdDigis. Distribute energy deposition into the modules.
  Int_t digiIndex = -1;
  for (Int_t iDigi = 0; iDigi < nDigis; iDigi++) {

    digiIndex = (event ? event->GetIndex(ECbmDataType::kFsdDigi, iDigi) : iDigi);
    digi      = fDigiMan->Get<const CbmFsdDigi>(digiIndex);
    assert(digi);

    uint32_t address   = digi->GetAddress();  // this contains more information than just module id
    Double_t eDep      = (Double_t) digi->GetEdep();
    Double_t digiTime  = (Double_t) digi->GetTime();
    auto insert_result = edeptimemap.insert(std::make_pair(address, std::make_tuple(eDep, digiTime, digiIndex)));
    if (!insert_result.second) {  // entry was here before
      std::get<0>((*insert_result.first).second) = eDep + std::get<0>((*insert_result.first).second);
      if (digiTime < std::get<1>((*insert_result.first).second)) {  // earlier digi => overwrite
        std::get<1>((*insert_result.first).second) = digiTime;
        std::get<2>((*insert_result.first).second) = digiIndex;
      }
    }  // ? exist entry
  }    // # digis

  // --- Create hits, one per activated module
  UInt_t hitIndex = -1;
  for (auto entry : edeptimemap) {
    uint32_t address               = entry.first;
    Double_t sumEdep               = std::get<0>(entry.second);
    Double_t fastestDigiTime       = std::get<1>(entry.second);
    Int_t fastestDigiIndex         = std::get<2>(entry.second);
    CbmFsdModuleSpecs* fsdModSpecs = CbmFsdGeoHandler::GetInstance().GetModuleSpecsById(
      CbmFsdAddress::GetElementId(address, static_cast<int32_t>(CbmFsdAddress::Level::Module)));
    if (!fsdModSpecs) {
      LOG(warning) << "FSD module data for address: " << address << " (unitId: "
                   << CbmFsdAddress::GetElementId(address, static_cast<int32_t>(CbmFsdAddress::Level::Unit))
                   << " ,moduleId: "
                   << CbmFsdAddress::GetElementId(address, static_cast<int32_t>(CbmFsdAddress::Level::Module))
                   << " ) could not be found! => skip";
      continue;
    }
    TVector3 moduleCenter;
    moduleCenter.SetXYZ(fsdModSpecs->fX, fsdModSpecs->fY, fsdModSpecs->fZ);
    TVector3 moduleDimensions;
    moduleDimensions.SetXYZ(fsdModSpecs->dX, fsdModSpecs->dY, fsdModSpecs->dZ);

    hitIndex = fHitArray->GetEntriesFast();
    new ((*fHitArray)[hitIndex]) CbmFsdHit(static_cast<int32_t>(address), moduleCenter, moduleDimensions,
                                           static_cast<int32_t>(fastestDigiIndex), fastestDigiTime, sumEdep);
    if (event) event->AddData(ECbmDataType::kFsdHit, hitIndex);
    nHits++;
  }

  return std::make_pair(nDigis, nHits);
}
// -------------------------------------------------------------------------


// -----   Reset: clear output array   -------------------------------------
void CbmFsdHitProducer::Reset()
{
  if (fHitArray) fHitArray->Delete();
}
// -------------------------------------------------------------------------


ClassImp(CbmFsdHitProducer)
