/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "CbmAnaTreeSource.h"

#include "CbmAnaTreeContainer.h"

#include <FairLogger.h>
#include <FairRootManager.h>

#include <TBranch.h>
#include <TChain.h>
#include <TFile.h>

#include <AnalysisTree/Configuration.hpp>
#include <fstream>


CbmAnaTreeSource::CbmAnaTreeSource() : CbmAnaTreeSource("", "aTree") {}

CbmAnaTreeSource::CbmAnaTreeSource(TString inFile, TString treeName)
  : fNFiles(1)
  , fTreeName(treeName)
  , fChain(nullptr)
  , fFileName(nullptr)
  , fContainerReco(nullptr)
  , fContainerSim(nullptr)
{
  fFileName    = new TString[1];
  fFileName[0] = inFile;
}

CbmAnaTreeSource::~CbmAnaTreeSource()
{
  if (fFileName) delete[] fFileName;
}

Bool_t CbmAnaTreeSource::Init()
{
  LOG(debug) << "Init source ";
  FairRootManager* mngr = FairRootManager::Instance();
  fChain                = new TChain(fTreeName);  //new name of tree?
  TString first_file_name;
  if (fFileName[0].EndsWith(".root")) {
    for (int i = 0; i < fNFiles; i++) {
      LOG(debug) << "CbmAnaTree: opening single file" << fFileName[i];
      fChain->Add(fFileName[i]);
    }
    first_file_name = fFileName[0];
  }
  else {  // this is long list
    std::ifstream list;
    list.open(fFileName[0]);
    bool first = true;
    do {
      TString temp;
      list >> temp;
      if (temp.Length() > 1) {
        fChain->Add(temp);
        if (first) {
          first_file_name = temp;
          first           = false;
        }
      }
      else {
        break;
      }
      LOG(debug) << "Adding file " << temp << " to chain";
    } while (!list.eof());
    list.close();
  }
  LOG(debug) << "load container ";
  fContainerReco   = new CbmAnaTreeRecoSourceContainer();
  fContainerSim    = new CbmAnaTreeMcSourceContainer();
  Bool_t recoAvail = fContainerReco->ConnectToTree(fChain);
  Bool_t simAvail  = fContainerSim->ConnectToTree(fChain);
  if (recoAvail == kFALSE) {
    delete fContainerReco;
    fContainerReco = nullptr;
  }
  if (simAvail == kFALSE) {
    delete fContainerSim;
    fContainerSim = nullptr;
  }
  LOG(debug) << "load conf ";
  LoadConf(first_file_name);
  mngr->SetInChain(fChain, -1);
  if (fContainerReco) {
    LOG(debug) << "Loading reco data";
    mngr->Register("CbmAnaTreeSourceContainer.", "CbmAnaTreeSourceContainer", fContainerReco, kFALSE);
  }
  if (fContainerSim) {
    LOG(debug) << "Loading sim ana";
    mngr->Register("CbmAnaTreeMcSourceContainer.", "CbmAnaTreeMcSourceContainer", fContainerSim, kFALSE);
  }
  return kTRUE;
}

Int_t CbmAnaTreeSource::ReadEvent(UInt_t unsignedInt)
{
  fChain->GetEntry(unsignedInt);
  return 0;
}

void CbmAnaTreeSource::Close() {}

void CbmAnaTreeSource::LoadConf(TString name)
{
  /// Save old global file and folder pointer to avoid messing with FairRoot
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  if (fContainerReco) {
    fContainerReco->LoadFields(name);
  }
  if (fContainerSim) {
    fContainerSim->LoadFields(name);
  }

  /// Restore old global file and folder pointer to avoid messing with FairRoot
  gFile      = oldFile;
  gDirectory = oldDir;
}

void CbmAnaTreeSource::AddFile(TString file)
{
  TString* temp = fFileName;
  fFileName     = new TString[fNFiles + 1];
  for (int i = 0; i < fNFiles; i++) {
    fFileName[i] = temp[i];
  }
  delete[] temp;
  fFileName[fNFiles] = file;
  fNFiles            = fNFiles + 1;
}

Int_t CbmAnaTreeSource::CheckMaxEventNo(Int_t /*int1*/) { return fChain->GetEntries(); }
