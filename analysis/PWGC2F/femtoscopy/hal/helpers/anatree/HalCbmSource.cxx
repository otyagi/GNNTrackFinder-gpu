/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmSource.h"

#include "CbmAnaTreeContainer.h"

#include <TBranch.h>
#include <TNamed.h>
#include <TObjArray.h>

#include <Hal/RootIOManager.h>

HalCbmSource::HalCbmSource(TString filename) : Source(filename) { fManager = new HalCbmATIOManager(filename); }

void HalCbmSource::AddFile(TString fileName) { fManager->AddFile(fileName); }

void HalCbmSource::AddFriend(TString friendName, Int_t level) { fManager->AddFriend(friendName, level); }

HalCbmSource::~HalCbmSource() { delete fManager; }

void HalCbmATIOManager::FillBranches()
{
  auto inchain     = GetInChain();
  fRecoContainer   = new CbmAnaTreeRecoSourceContainer();
  fSimContainer    = new CbmAnaTreeMcSourceContainer();
  Bool_t recoAvail = fRecoContainer->ConnectToTree(inchain[0]);
  Bool_t simAvail  = fSimContainer->ConnectToTree(inchain[0]);
  fRecoContainer->LoadFields(GetInputFileName());
  fSimContainer->LoadFields(GetInputFileName());
  int count = -1;
  for (auto chain : inchain) {
    count++;
    if (count == 0) {
      if (recoAvail) {
        AddBranch("CbmAnaTreeSourceContainer.", fRecoContainer, Hal::BranchInfo::EFlag::kInActive);
      }
      if (simAvail) {
        AddBranch("CbmAnaTreeMcSourceContainer.", fSimContainer, Hal::BranchInfo::EFlag::kInActive);
      }
      continue;  //skip first chain - we are looking for AT data
    }
    TObjArray* list_branch = chain->GetListOfBranches();
    for (int i = 0; i < list_branch->GetEntries(); i++) {
      TBranch* branch = (TBranch*) list_branch->At(i);
      TString name    = branch->GetName();
      TObject** obj   = new TObject*();
      PushTObject(obj);
      chain->SetBranchAddress(name, obj);
      AddBranch(branch->GetName(), obj[0], Hal::BranchInfo::EFlag::kInActive);
    }
  }
}

HalCbmATIOManager::~HalCbmATIOManager()
{
  if (fRecoContainer) delete fRecoContainer;
  if (fSimContainer) delete fSimContainer;
}
