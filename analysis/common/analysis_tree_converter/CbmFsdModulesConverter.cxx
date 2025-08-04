/* Copyright (C) 2020-2021 Physikalisches Institut, Eberhard Karls Universität Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Viktor Klochkov [committer] */

#include "CbmFsdModulesConverter.h"

#include "AnalysisTree/Detector.hpp"
#include "CbmDefs.h"
#include "CbmEvent.h"
#include "CbmFsdHit.h"
#include "FairRootManager.h"
#include "TClonesArray.h"

#include <AnalysisTree/TaskManager.hpp>
#include <cassert>
#include <vector>

ClassImp(CbmFsdModulesConverter);

void CbmFsdModulesConverter::Init()
{
  assert(!out_branch_.empty());
  auto* ioman = FairRootManager::Instance();
  assert(ioman != nullptr);
  cbm_fsd_hits_ = (TClonesArray*) ioman->GetObject("FsdHit");

  AnalysisTree::BranchConfig fsd_branch(out_branch_, AnalysisTree::DetType::kModule);

  auto* man = AnalysisTree::TaskManager::GetInstance();
  man->AddBranch(fsd_modules_, fsd_branch);
}


void CbmFsdModulesConverter::ProcessData(CbmEvent* event)
{
  assert(cbm_fsd_hits_);
  fsd_modules_->ClearChannels();

  CbmFsdHit* hit{nullptr};

  auto* data_header  = AnalysisTree::TaskManager::GetInstance()->GetDataHeader();
  auto* config       = AnalysisTree::TaskManager::GetInstance()->GetConfig();
  const auto& branch = config->GetBranchConfig(out_branch_);

  const int n_fsd_modules = data_header->GetModulePositions(0).GetNumberOfChannels();

  fsd_modules_->Reserve(n_fsd_modules);
  for (int i = 0; i < n_fsd_modules; ++i) {
    fsd_modules_->AddChannel(branch);
  }

  const int nFsdHits = event ? event->GetNofData(ECbmDataType::kFsdHit) : cbm_fsd_hits_->GetEntriesFast();
  if (nFsdHits <= 0) {
    LOG(warn) << "No FSD hits!";
    return;
  }

  for (int i = 0; i < nFsdHits; ++i) {
    hit = (CbmFsdHit*) cbm_fsd_hits_->At(i);
    if (hit == nullptr) continue;
    auto& module = fsd_modules_->Channel(hit->GetModuleId());
    module.SetNumber(i);
    module.SetSignal(hit->GetEdep());
  }
}

void CbmFsdModulesConverter::Finish() {}

CbmFsdModulesConverter::~CbmFsdModulesConverter() { delete fsd_modules_; };
