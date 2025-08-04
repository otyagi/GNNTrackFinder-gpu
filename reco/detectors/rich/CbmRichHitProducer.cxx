/* Copyright (C) 2004-2021 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Boris Polichtchouk, Andrey Lebedev [committer], Volker Friese */

/**
 * \file CbmRichHitProducer.cxx
 *
 * \author B. Polichtchouk
 * \date 2004
 **/

#include "CbmRichHitProducer.h"

#include "CbmDigiManager.h"
#include "CbmEvent.h"
#include "CbmRichDetectorData.h"  // for CbmRichPmtData, CbmRichPixelData
#include "CbmRichDigi.h"
#include "CbmRichDigiMapManager.h"
#include "CbmRichGeoManager.h"
#include "CbmRichHit.h"

#include <FairRootManager.h>
#include <Logger.h>

#include <TClonesArray.h>
#include <TStopwatch.h>

#include <iomanip>
#include <iostream>

using namespace std;


CbmRichHitProducer::CbmRichHitProducer() : FairTask("CbmRichHitProducer") {}

CbmRichHitProducer::~CbmRichHitProducer()
{
  FairRootManager* manager = FairRootManager::Instance();
  manager->Write();
}

void CbmRichHitProducer::SetParContainers() {}

InitStatus CbmRichHitProducer::Init()
{
  FairRootManager* manager = FairRootManager::Instance();

  fCbmEvents = dynamic_cast<TClonesArray*>(manager->GetObject("CbmEvent"));
  if (!fCbmEvents) {
    LOG(info) << ": CbmEvent NOT found \n \n \n";
  }
  else {
    LOG(info) << ": CbmEvent found \n \n \n";
  }

  fDigiMan = CbmDigiManager::Instance();
  fDigiMan->Init();
  if (!fDigiMan->IsPresent(ECbmModuleId::kRich)) {
    LOG(fatal) << "CbmRichHitProducer::Init: No RichDigi array!";
  }

  fRichHits = new TClonesArray("CbmRichHit");
  manager->Register("RichHit", "RICH", fRichHits, IsOutputBranchPersistent("RichHit"));

  CbmRichDigiMapManager::GetInstance();

  return kSUCCESS;
}

void CbmRichHitProducer::Exec(Option_t* /*option*/)
{

  TStopwatch timer;
  timer.Start();
  Int_t nDigisAll  = fDigiMan->GetNofDigis(ECbmModuleId::kRich);
  Int_t nDigisUsed = 0;
  Int_t nHits      = 0;
  Int_t nEvents    = 0;
  Int_t result     = 0;
  fRichHits->Delete();

  // Time-slice processing
  if (!fCbmEvents) nDigisUsed = ProcessData(nullptr);

  // Event processing
  else {
    nEvents = fCbmEvents->GetEntriesFast();
    for (Int_t iEvent = 0; iEvent < nEvents; iEvent++) {
      CbmEvent* event = static_cast<CbmEvent*>(fCbmEvents->At(iEvent));
      result          = ProcessData(event);
      nDigisUsed += result;
    }
  }

  timer.Stop();
  nHits = fRichHits->GetEntriesFast();
  stringstream logOut;
  logOut << setw(20) << left << GetName() << " [";
  logOut << fixed << setw(8) << setprecision(1) << right << timer.RealTime() * 1000. << " ms] ";
  logOut << "TS " << fNofTs;
  if (fCbmEvents) logOut << ", events " << nEvents;
  logOut << ", digis " << nDigisUsed << " / " << nDigisAll << ", hits " << nHits;
  LOG(info) << logOut.str();
  fNofTs++;
  fNofEvents += nEvents;
  fNofDigisAll += nDigisAll;
  fNofDigisUsed += nDigisUsed;
  fNofHitsAll += nHits;
  fTime += timer.RealTime();
}

Int_t CbmRichHitProducer::ProcessData(CbmEvent* event)
{
  Int_t nDigis = 0;
  if (event) {
    Int_t nofDigis = static_cast<Int_t>(event->GetNofData(ECbmDataType::kRichDigi));
    LOG(debug) << GetName() << ": Event mode. Event # " << event->GetNumber() << ", digis: " << nofDigis;
    for (Int_t iDigi = 0; iDigi < nofDigis; iDigi++) {
      Int_t digiIndex = static_cast<Int_t>(event->GetIndex(ECbmDataType::kRichDigi, iDigi));
      ProcessDigi(event, digiIndex);
    }
    nDigis = nofDigis;
  }
  else {
    nDigis = fDigiMan->GetNofDigis(ECbmModuleId::kRich);
    for (Int_t iDigi = 0; iDigi < fDigiMan->GetNofDigis(ECbmModuleId::kRich); iDigi++) {
      ProcessDigi(event, iDigi);
    }
  }
  return nDigis;
}

void CbmRichHitProducer::ProcessDigi(CbmEvent* event, Int_t digiIndex)
{
  const CbmRichDigi* digi = fDigiMan->Get<CbmRichDigi>(digiIndex);
  if (!digi) return;
  if (digi->GetAddress() < 0) return;
  CbmRichPixelData* data = CbmRichDigiMapManager::GetInstance().GetPixelDataByAddress(digi->GetAddress());
  TVector3 posPoint;
  posPoint.SetXYZ(data->fX, data->fY, data->fZ);
  TVector3 detPoint;

  CbmRichGeoManager::GetInstance().RotatePoint(&posPoint, &detPoint, !fRotationNeeded);
  AddHit(event, detPoint, digi->GetTime(), digiIndex);
}


void CbmRichHitProducer::AddHit(CbmEvent* event, TVector3& posHit, Double_t time, Int_t index)
{
  Int_t nofHits = fRichHits->GetEntriesFast();
  new ((*fRichHits)[nofHits]) CbmRichHit();
  CbmRichHit* hit = static_cast<CbmRichHit*>(fRichHits->At(nofHits));
  hit->SetPosition(posHit);
  hit->SetDx(fHitError);
  hit->SetDy(fHitError);
  hit->SetRefId(index);
  hit->SetTime(time);

  if (event) {
    event->AddData(ECbmDataType::kRichHit, nofHits);
  }
}

void CbmRichHitProducer::Finish()
{
  fRichHits->Clear();
  std::cout << std::endl;
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Time slices     : " << fNofTs;
  LOG(info) << "Digis      / TS : " << fixed << setprecision(2) << Double_t(fNofDigisAll) / Double_t(fNofTs);
  LOG(info) << "Used digis / TS : " << fixed << setprecision(2) << Double_t(fNofDigisUsed) / Double_t(fNofTs);
  LOG(info) << "Time       / TS : " << fixed << setprecision(2) << 1000. * fTime / Double_t(fNofTs) << " ms";
  if (fCbmEvents) {
    LOG(info) << "Events          : " << fNofEvents;
    LOG(info) << "Events     / TS : " << fixed << setprecision(2) << fNofEvents / (Double_t) fNofTs;
    if (fNofEvents > 0) {
      LOG(info) << "Digis      / Ev : " << fixed << setprecision(2) << Double_t(fNofDigisAll) / Double_t(fNofEvents);
      LOG(info) << "Used digis / Ev : " << fixed << setprecision(2) << Double_t(fNofDigisUsed) / Double_t(fNofEvents);
      LOG(info) << "Time       / Ev : " << fixed << setprecision(2) << 1000. * fTime / Double_t(fNofEvents) << " ms";
    }
  }
  LOG(info) << "=====================================\n";
}


ClassImp(CbmRichHitProducer)
