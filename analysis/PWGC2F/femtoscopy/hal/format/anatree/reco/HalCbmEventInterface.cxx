/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmEventInterface.h"

#include "CbmAnaTreeSource.h"
#include "CbmGlobalTrack.h"
#include "CbmStsTrack.h"
#include "CbmTofTrack.h"
#include "CbmVertex.h"
#include "FairRootManager.h"

#include <RtypesCore.h>
#include <TObjArray.h>

#include <Hal/DataManager.h>
#include <Hal/EventInterfaceAdvanced.h>
#include <Hal/RootIOManager.h>
#include <stddef.h>

HalCbmEventInterface::HalCbmEventInterface() {}

void HalCbmEventInterface::ConnectToTreeInternal(eMode /*mode*/)
{
  Hal::DataManager* manager = Hal::DataManager::Instance();
  fDataContainer            = (CbmAnaTreeRecoSourceContainer*) manager->GetObject("CbmAnaTreeSourceContainer.");
  auto branchList           = manager->GetBranchNameList();
  for (auto name : branchList) {
    std::cout << name << std::endl;
  }
  manager->GetIOManagerInfo();
  if (fDataContainer == nullptr) {
    fCbmVertex    = (CbmVertex*) manager->GetObject("PrimaryVertex.");
    fGlobalTracks = (TClonesArray*) manager->GetObject("GlobalTrack");
    fStsTracks    = (TClonesArray*) manager->GetObject("StsTrack");
    fTofHits      = (TClonesArray*) manager->GetObject("TofHit");
    fTrdTracks    = (TClonesArray*) manager->GetObject("TrdTrack");
    fRichRings    = (TClonesArray*) manager->GetObject("RichRing");
    fMuchTracks   = (TClonesArray*) manager->GetObject("MuchTrack");
    if (fCbmVertex && fGlobalTracks)
      fFormatType = HalCbm::DataFormat::kDST;
    else
      fFormatType = HalCbm::DataFormat::kUnknown;
  }
  else {
    fFormatType = HalCbm::DataFormat::kAnalysisTree;
  }
}

void HalCbmEventInterface::Register(Bool_t write)
{
  Hal::DataManager* manager = Hal::DataManager::Instance();
  switch (fFormatType) {
    case HalCbm::DataFormat::kAnalysisTree: {
      manager->Register("CbmAnaTreeSourceContainer.", "CbmAnaTreeSourceContainer.", fDataContainer, write);
    } break;
    case HalCbm::DataFormat::kDST: {
      manager->Register("PrimaryVertex.", "PrimaryVertex", fCbmVertex, write);
      manager->Register("GlobalTrack", "Tracks", fGlobalTracks, write);

      if (fStsTracks) manager->Register("StsTrack", "Tracks", fStsTracks, write);
      if (fTofHits) manager->Register("TofHit", "Tracks", fTofHits, write);
      if (fTrdTracks) manager->Register("TrdTrack", "Tracks", fTrdTracks, write);
      if (fRichRings) manager->Register("RichRing", "Tracks", fRichRings, write);
      if (fMuchTracks) manager->Register("MuchTrack", "Tracks", fMuchTracks, write);
    } break;
    default: break;
  }
}

Int_t HalCbmEventInterface::GetTotalTrackNo() const
{
  switch (fFormatType) {
    case HalCbm::DataFormat::kAnalysisTree: {
      return fDataContainer->GetVtxTracks()->GetNumberOfChannels();
    } break;
    case HalCbm::DataFormat::kDST: {
      return fGlobalTracks->GetEntriesFast();
    } break;
    default: break;
  }
  return 0;
}

TObject* HalCbmEventInterface::GetRawTrackPointer(Int_t /*index*/) const { return nullptr; }

TLorentzVector HalCbmEventInterface::GetVertex() const
{
  switch (fFormatType) {
    case HalCbm::DataFormat::kAnalysisTree: {
      AnalysisTree::EventHeader* header = fDataContainer->GetEventHeader();
      return TLorentzVector(header->GetVertexX(), header->GetVertexY(), header->GetVertexZ(), 0);
    } break;
    case HalCbm::DataFormat::kDST: {
      TLorentzVector vec(fCbmVertex->GetX(), fCbmVertex->GetY(), fCbmVertex->GetZ(), 0);
      return vec;
    } break;
    default: return TLorentzVector(0, 0, 0, 0); break;
  }
}

HalCbmEventInterface::~HalCbmEventInterface() {}

void HalCbmEventInterface::UpdateDst(HalCbmMCEventInterface* /*ie*/) {}

void HalCbmEventInterface::UpdateAnaTree(HalCbmMCEventInterface* /*ie*/) {}
