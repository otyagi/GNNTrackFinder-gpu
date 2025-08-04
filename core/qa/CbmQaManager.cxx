/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaManager.h
/// \brief  Manager task for other QA taska (implementation)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  17.10.2023

#include "CbmQaManager.h"

#include "CbmQaTask.h"
#include "Logger.h"

#include <iomanip>

// ---------------------------------------------------------------------------------------------------------------------
//
CbmQaManager::CbmQaManager(int verbose) : FairTask("CbmQaManager", verbose) {}


// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaManager::Finish()
{
  using std::left;
  using std::right;
  using std::setw;

  // Check QA-tasks
  for (auto* task : *(this->GetListOfTasks())) {
    auto* pQaTask = dynamic_cast<CbmQaTask*>(task);
    if (pQaTask) {
      LOG(info) << "Checking the task " << pQaTask->GetName();
      pQaTask->Check();
      pQaTask->ReadCheckListFromConfig();
    }
  }

  // Process check flags
  LOG(info) << fName << " check-list:";
  fStatus = true;
  for (auto* task : *(this->GetListOfTasks())) {
    auto* pQaTask = dynamic_cast<CbmQaTask*>(task);
    if (pQaTask) {
      LOG(info) << "Check list for the task " << pQaTask->GetName();
      const auto& mCheckList = pQaTask->GetCheckList();
      for (const auto& [entryName, flags] : mCheckList) {
        LOG(info) << '\t' << left << setw(40) << entryName << right << setw(10)
                  << (flags.fResult ? "\e[1;32mpassed\e[0m" : "\e[1;31mfailed\e[0m")
                  << (flags.fStatus ? "         " : " IGNORED ") << flags.fMsg;
        if (flags.fStatus) {
          fStatus &= flags.fResult;
        }
      }
    }
  }

  // Process histogram benchmarking
  if (fpBenchmarkInput.get()) {
    for (auto* task : *(this->GetListOfTasks())) {
      auto* pQaTask = dynamic_cast<CbmQaTask*>(task);
      if (pQaTask) {
        LOG(info) << "Histograms benchmark for the task " << pQaTask->GetName();
        pQaTask->CompareQaObjects();
      }
    }
    if (fpBenchmarkOutput.get()) {
      fpBenchmarkOutput->Close();
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmQaManager::Init()
{
  // Apply configuration file to all underlying tasks
  if (fsConfigName.Length()) {
    LOG(info) << "CbmQaManager: using configuration file " << fsConfigName;
    for (auto* task : *(this->GetListOfTasks())) {
      auto* pQaTask = dynamic_cast<CbmQaTask*>(task);
      if (pQaTask) {
        pQaTask->SetConfigName(fsConfigName.Data());
        pQaTask->SetVersionTag(fsVersionTag);
        pQaTask->SetDefaultTag(fsDefaultTag);
        if (fpBenchmarkInput.get() != nullptr) {
          pQaTask->SetCheckFile(fpBenchmarkInput);
        }
        if (fpBenchmarkOutput.get() != nullptr) {
          pQaTask->SetCompareOutput(fpBenchmarkOutput);
        }
      }
    }
  }
  return kSUCCESS;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaManager::OpenBenchmarkInput(const TString& path)
{
  if (path.Length()) {
    auto pFile = std::make_shared<TFile>(path, "READONLY");
    if (pFile->IsOpen()) {
      fpBenchmarkInput = std::move(pFile);
      LOG(info) << fName << ": opening benchmark input file " << fpBenchmarkInput->GetName();
    }
    else {
      LOG(error) << fName << ": benchmark input file " << path << " was not opened";
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaManager::OpenBenchmarkOutput(const TString& path)
{
  if (path.Length()) {
    auto pFile = std::make_shared<TFile>(path, "RECREATE");
    if (pFile->IsOpen()) {
      fpBenchmarkOutput = std::move(pFile);
      LOG(info) << fName << ": opening benchmark output file " << fpBenchmarkOutput->GetName();
    }
    else {
      LOG(error) << fName << ": benchmark output file " << path << " was not opened";
    }
  }
}
