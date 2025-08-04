/* Copyright (C) 2020-2021 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Viktor Klochkov [committer] */

#include "CbmConverterManager.h"

#include "AnalysisTree/DataHeader.hpp"
#include "AnalysisTree/TaskManager.hpp"
#include "CbmConverterTask.h"
#include "CbmDefs.h"
#include "CbmEvent.h"
#include "TClonesArray.h"
#include "TGeoBBox.h"
#include "TGeoManager.h"
#include "TString.h"

#include <iostream>

ClassImp(CbmConverterManager);

InitStatus CbmConverterManager::Init()
{
  task_manager_->Init();
  FillDataHeader();
  InitEvent();
  return kSUCCESS;
}

void CbmConverterManager::AddTask(CbmConverterTask* task)
{
  tasks_.emplace_back(task);
  task_manager_->AddTask(reinterpret_cast<AnalysisTree::Task*>(task));
}

void CbmConverterManager::ProcessData(CbmEvent* event)
{
  index_map_.clear();

  for (auto* task : tasks_) {
    task->SetIndexesMap(&index_map_);
    task->ProcessData(event);
    index_map_.insert(std::make_pair(task->GetOutputBranchName(), task->GetOutIndexesMap()));
  }
  task_manager_->FillOutput();
}

void CbmConverterManager::Exec(Option_t* /*opt*/)
{
  if (events_ != nullptr) {
    auto n_events = events_->GetEntriesFast();
    for (int i_event = 0; i_event < n_events; ++i_event) {
      auto* event = (CbmEvent*) events_->At(i_event);
      ProcessData(event);
    }
  }
  else {
    LOG(info) << "Event based mode\n";
    ProcessData(nullptr);
  }
}


void CbmConverterManager::Finish()
{
  TDirectory* curr   = gDirectory;  // TODO check why this is needed
  TFile* currentFile = gFile;

  task_manager_->Finish();

  gFile      = currentFile;
  gDirectory = curr;
}

void CbmConverterManager::FillDataHeader()
{
  // Force user to write data info //TODO is there a way to read it from a file automatically?
  assert(!system_.empty() && beam_mom_);

  auto* data_header = new AnalysisTree::DataHeader();

  std::cout << "ReadDataHeader" << std::endl;
  data_header->SetSystem(system_);
  data_header->SetBeamMomentum(beam_mom_);
  data_header->SetTimeSliceLength(ts_length_);

  auto& specDet_mod_pos    = data_header->AddDetector();
  TString specDet_names[2] = {(TString) ToString(ECbmModuleId::kPsd), (TString) ToString(ECbmModuleId::kFsd)};
  const char* module_name  = "module";  // PSD and FSD modules always contains "module"

  TVector3 frontFaceGlobal;
  Int_t nSpecDetModules  = 0;
  TString specDetNameTag = "";

  TGeoIterator geoIterator(gGeoManager->GetTopNode()->GetVolume());
  TGeoNode* curNode;
  geoIterator.Reset();  // safety to reset to "cave" before the loop starts
  while ((curNode = geoIterator())) {
    TString nodePath;
    geoIterator.GetPath(nodePath);
    if (!(nodePath.Contains(specDet_names[0], TString::kIgnoreCase)
          || nodePath.Contains(specDet_names[1], TString::kIgnoreCase))) {
      geoIterator.Skip();  // skip current branch when it is not FSD => should speed up
      continue;            // don't do anything for this branch
    }

    TString nodeName(curNode->GetName());

    // spectator detector as a whole
    if (nodeName.Contains(specDet_names[0], TString::kIgnoreCase)
        || nodeName.Contains(specDet_names[1], TString::kIgnoreCase)) {
      specDetNameTag = nodeName;

      auto specDetGeoMatrix = curNode->GetMatrix();
      auto specDetBox       = (TGeoBBox*) curNode->GetVolume()->GetShape();
      TVector3 frontFaceLocal(0, 0, -specDetBox->GetDZ());
      specDetGeoMatrix->LocalToMaster(&frontFaceLocal[0], &frontFaceGlobal[0]);
    }

    // modules of spectator detector
    if (nodeName.Contains(module_name)) {
      nSpecDetModules++;

      auto geoMatrix = curNode->GetMatrix();
      TVector3 translation(geoMatrix->GetTranslation());
      double x  = translation.X();
      double y  = translation.Y();

      auto* module = specDet_mod_pos.AddChannel();
      module->SetPosition(x, y, frontFaceGlobal[2]);
      LOG(info) << "Module " << nSpecDetModules << " : " << Form("(%.3f, %.3f)", x, y) << " id "
                << curNode->GetNumber();
    }
  }

  LOG(info) << "Detector " << specDetNameTag << " with " << nSpecDetModules << " modules";

  task_manager_->SetOutputDataHeader(data_header);
}
CbmConverterManager::~CbmConverterManager() = default;
