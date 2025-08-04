/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmMCEventInterface.h"

#include "CbmAnaTreeContainer.h"
#include "HalCbmDetectorID.h"

#include <FairMCEventHeader.h>

#include <Hal/DataManager.h>
#include <Hal/Event.h>
#include <Hal/EventInterface.h>


HalCbmMCEventInterface::HalCbmMCEventInterface() {}

void HalCbmMCEventInterface::Register(Bool_t write)
{
  Hal::DataManager* manager = Hal::DataManager::Instance();
  switch (fFormatType) {
    case HalCbm::DataFormat::kAnalysisTree: {
      manager->Register("CbmAnaTreeMcSourceContainer.", "CbmAnaTreeMcSourceContainer.", fDataContainer, write);
    } break;
    case HalCbm::DataFormat::kDST: {
      manager->Register("CbmMCTrack", "Tracks", fCbmMCtracks, write);
      manager->Register("MCEventHeader", "Tracks", fEventHeader, write);
      if (fStsMatches) manager->Register("StsTrackMatch", "Tracks", fStsMatches, write);
      if (fTofMatches) manager->Register("TofHitMatch", "Tracks", fTofMatches, write);
      if (fTrdMatches) manager->Register("TrdTrackMatch", "Tracks", fTrdMatches, write);
      if (fRichMatches) manager->Register("RichRingMatch", "Tracks", fRichMatches, write);
      if (fMuchMatches) manager->Register("MuchTrackMatch", "Tracks", fMuchMatches, write);
    } break;
    case HalCbm::DataFormat::kUnknown: {
      // DO nothing
    } break;
  }
}

void HalCbmMCEventInterface::FillTrackInterface(Hal::TrackInterface* /*track*/, Int_t /*index*/) {}

Int_t HalCbmMCEventInterface::GetTotalTrackNo() const
{
  switch (fFormatType) {
    case HalCbm::DataFormat::kAnalysisTree: {
      return fDataContainer->GetParticles()->GetNumberOfChannels();
    } break;
    case HalCbm::DataFormat::kDST: {
      return fCbmMCtracks->GetEntriesFast();
    } break;
    case HalCbm::DataFormat::kUnknown: {
      // DO nothing
    } break;
  }
  return 0;
}

TObject* HalCbmMCEventInterface::GetRawTrackPointer(Int_t /*index*/) const { return nullptr; }

TLorentzVector HalCbmMCEventInterface::GetVertexError() const { return TLorentzVector(0, 0, 0, 0); }

TLorentzVector HalCbmMCEventInterface::GetVertex() const
{
  switch (fFormatType) {
    case HalCbm::DataFormat::kAnalysisTree: {
      AnalysisTree::EventHeader* header = fDataContainer->GetEventHeader();
      return TLorentzVector(header->GetVertexX(), header->GetVertexY(), header->GetVertexZ(), 0);
    } break;
    case HalCbm::DataFormat::kDST: {
      return TLorentzVector(fEventHeader->GetX(), fEventHeader->GetY(), fEventHeader->GetZ(), fEventHeader->GetT());
    } break;
    case HalCbm::DataFormat::kUnknown: {
      // DO nothing
    } break;
  }
  return TLorentzVector(0, 0, 0, 0);
}

HalCbmMCEventInterface::~HalCbmMCEventInterface() {}

void HalCbmMCEventInterface::ConnectToTreeInternal(eMode /*mode*/)
{
  Hal::DataManager* manager = Hal::DataManager::Instance();
  fDataContainer            = (CbmAnaTreeMcSourceContainer*) manager->GetObject("CbmAnaTreeMcSourceContainer.");
  if (fDataContainer) {
    fFormatType = HalCbm::DataFormat::kAnalysisTree;
  }
  else {
    fCbmMCtracks = (TClonesArray*) manager->GetObject("CbmMCTrack");
    fEventHeader = (FairMCEventHeader*) manager->GetObject("MCEventHeader");
    fStsMatches  = (TClonesArray*) manager->GetObject("StsTrackMatch");
    fTofMatches  = (TClonesArray*) manager->GetObject("TofHitMatch");
    fTrdMatches  = (TClonesArray*) manager->GetObject("TrdTrackMatch");
    fRichMatches = (TClonesArray*) manager->GetObject("RichRingMatch");
    fMuchMatches = (TClonesArray*) manager->GetObject("MuchTrackMatch");
  }
}
