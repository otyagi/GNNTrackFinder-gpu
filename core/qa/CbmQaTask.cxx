/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaTask.cxx
/// \brief  A base class for CBM QA task logic (implementation)
/// \author S.Zharko <s.zharko@lx-pool.gsi.de>
/// \data   12.01.2023


#include "CbmQaTask.h"

#include "FairRootFileSink.h"
#include "FairRootManager.h"
#include "FairRunAna.h"
#include "TAxis.h"
#include "TCanvas.h"
#include "TClonesArray.h"
#include "TF1.h"
#include "TPad.h"
#include "TPaveText.h"
#include "TString.h"

#include <array>
#include <limits>
#include <regex>

ClassImp(CbmQaTask);

// ---------------------------------------------------------------------------------------------------------------------
//
CbmQaTask::CbmQaTask(const char* name, int verbose, bool isMCUsed, ECbmRecoMode recoMode)
  : FairTask(name, verbose)
  , CbmQaIO(name)
  , fbUseMC(isMCUsed)
  , fRecoMode(recoMode)
{
  CbmQaIO::SetRootFolderName(name);
  fStoringMode = CbmQaIO::EStoringMode::kSUBDIR;  // mode of objects arrangement in the output file
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaTask::Exec(Option_t* /*option*/)
{
  if (fpBrEvents) {
    int nEvents = fpBrEvents->GetEntriesFast();
    for (int iEvt = 0; iEvt < nEvents; ++iEvt) {
      fpCurrentEvent = static_cast<CbmEvent*>(fpBrEvents->At(iEvt));
      assert(fpCurrentEvent);
      this->ExecQa();
      fNofEvents.SetVal(fNofEvents.GetVal() + 1);
    }
    fpCurrentEvent = nullptr;
  }
  else {
    this->ExecQa();
    fNofEvents.SetVal(fNofEvents.GetVal() + 1);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaTask::Finish()
{
  // Processes canvases in the end of the run
  this->CreateSummary();

  // Write the root folder to sinker
  auto* pSink = FairRootManager::Instance()->GetSink();
  LOG_IF(fatal, !pSink) << fName << ": output sink is undefined";
  if (dynamic_cast<FairRootFileSink*>(pSink)) {
    auto* pRootSink = static_cast<FairRootFileSink*>(pSink);
    auto* pRootFile = pRootSink->GetRootFile();
    if (!pRootFile->FindObjectAny("nEvents")) {
      pRootFile->cd();
      fNofEvents.Write();
    }
    this->WriteToFile(pRootSink->GetRootFile());
  }
  else {
    LOG(warn) << fName << ": objects cannot be written into online sink (not implemented yet)";
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmQaTask::Init()
{
  LOG_IF(info, fVerbose > 0) << fName << ": initializing task ...";
  InitStatus res = kSUCCESS;

  // ----- Clear map of the histograms (note)
  DeInitBase();

  // ----- Initialize task
  LOG_IF(info, fVerbose > 1) << fName << ": initializing histograms";
  res = std::max(res, this->InitQa());

  // ----- Initialize event branch
  if (ECbmRecoMode::EventByEvent == fRecoMode) {
    fpBrEvents = dynamic_cast<TClonesArray*>(FairRootManager::Instance()->GetObject("CbmEvent"));
    if (fpBrEvents) {
      LOG_IF(info, fVerbose > 1) << fName << ": the routine will run on reconstructed events";
    }
    else {
      LOG_IF(info, fVerbose > 1) << fName << ": the routine will run on timeslices, the CbmEvent branch is undefined";
      fRecoMode = ECbmRecoMode::Timeslice;
    }
  }
  else {
    LOG_IF(info, fVerbose > 1) << fName << ": the routine will run on timeslices";
  }

  fNofEvents.SetVal(0);

  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmQaTask::ReInit()
{
  return Init();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaTask::DeInitBase()
{
  // De-initialize basic data members
  // ...

  // De-initialize particular QA-task implementation
  DeInit();
}


// ---------------------------------------------------------------------------------------------------------------------
//
bool CbmQaTask::CheckRange(TH1* h, double meanMax, double rmsMin, double rmsMax)
{
  // Checks ranges for mean and standard deviation
  // \return  False, if variable exits the range

  double mean    = h->GetMean();
  double meanErr = h->GetMeanError();

  double rms    = h->GetStdDev();
  double rmsErr = h->GetStdDevError();

  bool res = true;
  if (h->GetEntries() >= 10) {  // ask for some statistics, otherwise errors are not properly estimated
    res = CheckRange(TString("Mean for ") + h->GetTitle(), mean, meanErr, -meanMax, meanMax) && res;
    res = CheckRange(TString("RMS for ") + h->GetTitle(), rms, rmsErr, rmsMin, rmsMax) && res;
  }
  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool CbmQaTask::CompareQaObjects()
{
  YAML::Node tagNode = fConfigNode["check_histograms"];


  struct HashTString {
    std::size_t operator()(const TString& str) const { return std::hash<std::string>()(str.Data()); };
  };

  // Fill map of ROOT objects, which are going to be checked
  std::unordered_map<TString, ObjectComparisonConfig, HashTString> mCompareList;
  // TODO: 28.12.2023 SZh: Add a possibility to define a pack of histograms (in a similar way to check flags)
  for (const auto& histNode : tagNode) {
    try {
      auto histName = fsRootFolderName + "/" + histNode["name"].as<std::string>().c_str();
      ObjectComparisonConfig entry;
      if (const auto& subNode = histNode["point_to_point"]) {
        entry.fPoint = subNode["use"].as<uint8_t>();
      }
      if (const auto& subNode = histNode["ratio"]) {
        entry.fRatio    = subNode["use"].as<uint8_t>();
        entry.fRatioMin = subNode["min"].as<double>();
        entry.fRatioMax = subNode["max"].as<double>();
      }
      if (const auto& subNode = histNode["chi2Test"]) {
        entry.fStat       = subNode["use"].as<uint8_t>();
        entry.fChi2NdfMax = subNode["chi2_ndf_max"].as<double>();
      }
      if (const auto& subNode = histNode["canvas"]) {
        entry.fCanvOpt = "";
        if (subNode["comp"].as<bool>(false)) {
          entry.fCanvOpt += "c";
        }
        if (subNode["ratio"].as<bool>(false)) {
          entry.fCanvOpt += "r";
        }
        if (subNode["diff"].as<bool>(false)) {
          entry.fCanvOpt += "d";
        }
      }
      mCompareList[histName] = entry;
    }
    catch (const YAML::InvalidNode& exc) {
      LOG(warn) << fName << "::CompareQaObjects(): attempt to access key which does not exist in the configuration"
                << " file (message from YAML exception: " << exc.what() << ')';
    }
  }

  // Process comparison
  // Look up the objects in the object list and search for the name in the map

  bool bCheckResult = true;
  for (auto& [pObject, sPath] : (*fpvObjList)) {
    if (pObject) {

      TString objName    = sPath + "/" + pObject->GetName();
      auto configEntryIt = mCompareList.find(objName);
      if (configEntryIt == mCompareList.end()) {
        continue;
      }
      const auto& configEntry = configEntryIt->second;

      // Get default object
      auto* pObjCheck = fpBenchmarkInput->Get(objName);
      if (!pObjCheck) {
        LOG(warn) << fName << "::CompareQaObjects(): ROOT object " << objName << " is not found in the check file";
        continue;
      }

      // Call the comparison function depending on the object type
      if (dynamic_cast<TProfile*>(pObject)) {
        bCheckResult &= CompareTwoObjects<TProfile>(pObject, pObjCheck, objName, configEntry);
      }
      else if (dynamic_cast<TH2*>(pObject)) {
        bCheckResult &= CompareTwoObjects<TH2>(pObject, pObjCheck, objName, configEntry);
      }
      else if (dynamic_cast<TH1*>(pObject)) {
        bCheckResult &= CompareTwoObjects<TH1>(pObject, pObjCheck, objName, configEntry);
      }
    }
  }
  return bCheckResult;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaTask::PutSetupNameOnPad(double xMin, double yMin, double xMax, double yMax)
{
  if (gPad) {
    gPad->cd();
    auto* paveText = new TPaveText(xMin, yMin, xMax, yMax, "NDC");
    paveText->AddText(fsSetupName.c_str());
    paveText->SetBorderSize(0);
    paveText->SetTextSize(0.04);
    paveText->SetFillStyle(0);
    paveText->Draw("same");
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaTask::ReadCheckListFromConfig()
{
  YAML::Node tagNode = fConfigNode["check_list"];
  if (tagNode) {
    std::vector<char> vbConfigKeyUsed;
    std::vector<char> vbCheckKeyProcessed(fmCheckList.size(), false);  // Flag to avoid checking the check entry twice
    vbConfigKeyUsed.reserve(tagNode.size());
    std::regex rexInt("%d");
    // First loop: read all the entries, containing special characters ("%d")
    for (auto it = tagNode.begin(); it != tagNode.end(); ++it) {
      std::string configEntry = it->first.as<std::string>();
      if (static_cast<int>(configEntry.find_last_of("%d")) != -1) {
        vbConfigKeyUsed.push_back(true);
        bool bCheckStatus = it->second.as<bool>();  // use-flag, stored in the configuration
        std::regex keyRex = std::regex(std::regex_replace(configEntry, rexInt, "([0-9]+)"));
        int iCheckEntry   = 0;
        for (auto& [checkEntry, checkFlags] : fmCheckList) {
          if (!vbCheckKeyProcessed[iCheckEntry] && std::regex_match(checkEntry, keyRex)) {
            vbCheckKeyProcessed[iCheckEntry] = true;
            checkFlags.fStatus               = bCheckStatus;
          }
          ++iCheckEntry;
        }
      }
      else {
        vbConfigKeyUsed.push_back(false);
      }
    }
    // Second loop: read all other entries
    int iConfigEntry = 0;
    for (auto it = tagNode.begin(); it != tagNode.end(); ++it) {
      if (!vbConfigKeyUsed[iConfigEntry]) {
        std::string configEntry = it->first.as<std::string>();
        auto checkListIt        = fmCheckList.find(configEntry);
        if (checkListIt != fmCheckList.end()) {
          checkListIt->second.fStatus = it->second.as<bool>();
        }
        else {
          LOG(warn) << fName << "::ReadCheckListFromConfig(): config contains unknown entry " << configEntry;
        }
      }
      ++iConfigEntry;
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaTask::StoreCheckResult(const std::string& tag, bool result, const std::string& msg)
{
  fmCheckList[tag] = CheckFlags{msg, result, false};
}
