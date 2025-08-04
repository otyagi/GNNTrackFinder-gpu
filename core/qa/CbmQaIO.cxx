/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmQaIO.cxx
/// @brief  Module for ROOT objects IO interface (implementation)
/// @author S.Zharko <s.zharko@gsi.de>
/// @since  29.03.2023

#include "CbmQaIO.h"

#include "CbmQaCanvas.h"
#include "CbmQaUtil.h"
#include "TFile.h"
#include "TPaveStats.h"

// ---------------------------------------------------------------------------------------------------------------------
//
CbmQaIO::CbmQaIO(TString prefixName, std::shared_ptr<ObjList_t> pObjList) : fsPrefix(prefixName), fpvObjList(pObjList)
{
  if (!fsPrefix.IsNull()) {
    //fsPrefix += "_";
  }
  if (!fpvObjList.get()) {
    fpvObjList = std::make_shared<ObjList_t>();
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
CbmQaIO::~CbmQaIO() {}


// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaIO::SetTH1Properties(TH1* pHist) const
{
  // Set default histo properties
  //pHist->GetYaxis()->SetLabelOffset(0.95);

  TPaveStats* stats = cbm::qa::util::GetHistStats(pHist);
  assert(stats);
  stats->SetY1NDC(0.7);
  stats->SetOptStat(111110);
  stats->SetOptFit(100001);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaIO::SetTH2Properties(TH2* pHist) const
{
  // Set default histo properties

  pHist->SetOption("colz");
  pHist->SetStats(false);
  //pHist->GetYaxis()->SetLabelOffset(0.95);
  /*
  TPaveStats* stats = cbm::qa::util::GetHistStats(pHist);
  assert(stats);
  stats->SetOptStat(10);
  stats->SetOptFit(0);
  */
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaIO::SetTProfile2DProperties(TProfile2D* pHist) const
{
  // Set default histo properties

  pHist->GetYaxis()->SetLabelOffset(0.95);
  pHist->SetOption("colz");
  pHist->SetStats(false);
  /*
  TPaveStats* stats = cbm::qa::util::GetHistStats(pHist);
  assert(stats);
  stats->SetOptStat(10);
  stats->SetOptFit(0);
  */
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaIO::SetCanvasProperties(TCanvas* pCanv) const
{
  constexpr double left   = 0.18;
  constexpr double bottom = 0.15;
  constexpr double right  = 0.10;
  constexpr double top    = 0.10;
  pCanv->SetMargin(left, right, bottom, top);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaIO::SetConfigName(const char* path)
{
  fsConfigName = path;
  try {
    fConfigNode = YAML::LoadFile(path)["qa"][fsPrefix.Data()];
  }
  catch (const YAML::BadFile& exc) {
    std::stringstream msg;
    msg << "configuration file for QA \"" << path << "\" does not exist";
    throw std::runtime_error(msg.str());
  }
  catch (const YAML::ParserException& exc) {
    std::stringstream msg;
    msg << "configuration file for QA \"" << path << "\" is improperly formatted";
    throw std::runtime_error(msg.str());
  }
  LOG(info) << fsPrefix << ": configuration file is set to " << path;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaIO::WriteToFile(TFile* pOutFile) const
{
  pOutFile->cd();
  for (auto& [pObject, sPath] : (*fpvObjList)) {
    if (!pOutFile->GetDirectory(sPath)) {
      pOutFile->mkdir(sPath);
    }
    pOutFile->cd(sPath);
    if (pObject) {
      pObject->Write();
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmQaIO::MakeQaDirectory(TString sDirectory)
{
  // Add parent directory
  if (fsRootFolderName.Length() != 0) {
    sDirectory = fsRootFolderName + "/" + sDirectory;
  }

  // Register the object in the list
  fpvObjList->push_back(std::make_pair(nullptr, sDirectory));
}
