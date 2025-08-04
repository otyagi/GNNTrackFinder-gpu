/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "CbmUnigenSource.h"

#include "UEvent.h"
#include "UParticle.h"

#include <FairLogger.h>
#include <FairRootManager.h>

#include <TBranch.h>

#include <fstream>
#include <iostream>


CbmUnigenSource::CbmUnigenSource() : fUnigenChain(nullptr), fEvent(nullptr), fPrintTreeInfo(kFALSE) {}

CbmUnigenSource::CbmUnigenSource(TString inFile) : fUnigenChain(nullptr), fEvent(nullptr), fPrintTreeInfo(kFALSE)
{
  fFileName.push_back(inFile);
}

CbmUnigenSource::~CbmUnigenSource()
{
  //if (fUnigenChain) delete fUnigenChain;
  //is  owned by FairRoot ?
}

Bool_t CbmUnigenSource::Init()
{
  FairRootManager* mngr = FairRootManager::Instance();
  fUnigenChain          = new TChain("events");
  if (fFileName[0].EndsWith(".root")) {
    for (auto i : fFileName) {
      LOG(debug) << "NicaUnigenSource: opening single file" << i;
      fUnigenChain->Add(i);
    }
  }
  else {  // this is long list
    std::ifstream list;
    list.open(fFileName[0]);
    do {
      TString temp;
      list >> temp;
      if (temp.Length() > 1) {
        fUnigenChain->Add(temp);
      }
      else {
        break;
      }
      LOG(debug) << "Adding file " << temp << " to chain";
    } while (!list.eof());
    list.close();
  }

  fEvent = new UEvent();
  if (fUnigenChain->GetBranch("event")) {
    if (fPrintTreeInfo) fUnigenChain->Print();
    fUnigenChain->SetBranchStatus("event", 1);
    fUnigenChain->SetBranchAddress("event", &fEvent);
  }
  else {
    if (fPrintTreeInfo) std::cout << "Event read II" << std::endl;
    fUnigenChain->SetBranchStatus("UEvent.", 1);
    fUnigenChain->SetBranchAddress("UEvent.", &fEvent);
  }
  mngr->SetInChain(fUnigenChain, -1);
  mngr->Register("UEvent.", "UEvent", (TNamed*) fEvent, kFALSE);
  return kTRUE;
}

Int_t CbmUnigenSource::ReadEvent(UInt_t unsignedInt)
{
  //std::cout<<"READING EVENT " <<unsignedInt<<std::endl;
  fUnigenChain->GetEntry(unsignedInt);
  // std::cout<<"xxx"<<std::endl;
  return 0;
}

void CbmUnigenSource::Close() {}

void CbmUnigenSource::Boost(Double_t vx, Double_t vy, Double_t vz)
{
  for (int i = 0; i < fEvent->GetNpa(); i++) {
    UParticle* p       = fEvent->GetParticle(i);
    TLorentzVector mom = p->GetMomentum();
    TLorentzVector pos = p->GetPosition();
    mom.Boost(vx, vy, vz);
    pos.Boost(vx, vy, vz);
    p->SetMomentum(mom);
    p->SetPosition(pos);
  }
}

Int_t CbmUnigenSource::CheckMaxEventNo(Int_t /*int1*/) { return fUnigenChain->GetEntries(); }

Bool_t CbmUnigenSource::SpecifyRunId() { return kFALSE; }
