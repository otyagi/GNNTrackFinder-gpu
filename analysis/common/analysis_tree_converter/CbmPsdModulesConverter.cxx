/* Copyright (C) 2020-2021 Physikalisches Institut, Eberhard Karls Universität Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Viktor Klochkov [committer] */

#include "CbmPsdModulesConverter.h"

#include "AnalysisTree/Detector.hpp"
#include "CbmDefs.h"
#include "CbmEvent.h"
#include "CbmPsdHit.h"
#include "FairRootManager.h"
#include "TClonesArray.h"

#include <AnalysisTree/TaskManager.hpp>
#include <cassert>
#include <vector>

ClassImp(CbmPsdModulesConverter);

void CbmPsdModulesConverter::Init()
{
  assert(!out_branch_.empty());
  auto* ioman = FairRootManager::Instance();
  assert(ioman != nullptr);
  cbm_psd_hits_ = (TClonesArray*) ioman->GetObject("PsdHit");

  AnalysisTree::BranchConfig psd_branch(out_branch_, AnalysisTree::DetType::kModule);

  auto* man = AnalysisTree::TaskManager::GetInstance();
  man->AddBranch(psd_modules_, psd_branch);
}


void CbmPsdModulesConverter::ProcessData(CbmEvent* event)
{
  assert(cbm_psd_hits_);
  psd_modules_->ClearChannels();

  CbmPsdHit* hit{nullptr};

  auto* data_header  = AnalysisTree::TaskManager::GetInstance()->GetDataHeader();
  auto* config       = AnalysisTree::TaskManager::GetInstance()->GetConfig();
  const auto& branch = config->GetBranchConfig(out_branch_);

  const int n_psd_modules = data_header->GetModulePositions(0).GetNumberOfChannels();

  psd_modules_->Reserve(n_psd_modules);
  for (int i = 0; i < n_psd_modules; ++i) {
    psd_modules_->AddChannel(branch);
  }

  const int nPsdHits = event ? event->GetNofData(ECbmDataType::kPsdHit) : cbm_psd_hits_->GetEntriesFast();
  if (nPsdHits <= 0) {
    LOG(warn) << "No PSD hits!";
    return;
  }

  for (int i = 0; i < nPsdHits; ++i) {
    hit = (CbmPsdHit*) cbm_psd_hits_->At(i);
    if (hit == nullptr) continue;
    auto& module = psd_modules_->Channel(i);
    module.SetNumber(hit->GetModuleID());
    module.SetSignal(hit->GetEdep());
  }
}

void CbmPsdModulesConverter::Finish() {}

CbmPsdModulesConverter::~CbmPsdModulesConverter() { delete psd_modules_; };
