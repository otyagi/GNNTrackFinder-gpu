/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmQaCheckerFileHandler.cxx
/// @brief  A handler class to process versions from similar files (implementation)
/// @author S. Zharko <s.zharko@gsi.de>
/// @since  08.02.2023

#include "CbmQaCheckerFileHandler.h"

#include "CbmQaCheckerHist1DHandler.h"
#include "CbmQaCheckerHist2DHandler.h"
#include "CbmQaCheckerProfile1DHandler.h"
#include "Logger.h"
#include "TDirectoryFile.h"
#include "TFile.h"
#include "TFolder.h"
#include "TH1.h"
#include "TH2.h"
#include "TNamed.h"
#include "TProfile.h"
#include "TROOT.h"

#include <boost/algorithm/string.hpp>

#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>


using cbm::qa::checker::ECmpInference;
using cbm::qa::checker::FileHandler;

// ---------------------------------------------------------------------------------------------------------------------
//
FileHandler::FileHandler(std::shared_ptr<ObjectDB>& pObjDB, int iDataset, int iFile)
  : fFileID(iFile)
  , fDatasetID(iDataset)
  , fpObjDB(pObjDB)
{
  ReOpenOutputFile();
  fpInputFiles = std::make_unique<TClonesArray>("TFile");

  for (int iVer = 0; iVer < fpObjDB->GetNofVersions(); ++iVer) {
    std::string filename = fpObjDB->GetInputFileName(iVer, fFileID, fDatasetID);
    auto* pInFile        = new ((*fpInputFiles)[iVer]) TFile(filename.data(), "READONLY");
    LOG(info) << "File: " << pInFile->GetName();
    LOG_IF(fatal, !pInFile->IsOpen()) << "FileHandler: file " << filename << " cannot be opened";
  }

  // Check registered folder
  LOG_IF(fatal, !fpObjDB.get()) << "FileHandler: attempt to register a null pointer for the object database";
  LOG_IF(fatal, !fpOutDir) << "FileHandler: attempt to register a null pointer for the output directory";
  LOG_IF(fatal, fDatasetID < 0) << "FileHandler: attempt to register undefined dataset index";
  LOG_IF(fatal, fFileID < 0) << "FileHandler: attempt to register undefined file index";
}

// ---------------------------------------------------------------------------------------------------------------------
//
FileHandler::~FileHandler()
{
  // ----- Clean pointers
  fpInputFiles->Delete();
  fpInputFiles = nullptr;
  fpOutputFile->Close();
  fpOutputFile = nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
//
TDirectory* FileHandler::CreateNestedDirectory(const std::string& path)
{
  fpOutDir->mkdir(path.data());
  return fpOutDir->GetDirectory(path.data());
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::vector<ECmpInference> FileHandler::Process(Option_t* opt)
{
  std::vector<ECmpInference> bCmpResult;
  int nObjects  = fpObjDB->GetNofObjects(fFileID);
  int nVersions = fpObjDB->GetNofVersions();
  bCmpResult.resize(nVersions, ECmpInference::StronglyEqual);

  // ----- Initial checks
  if (!nObjects) {
    LOG(warn) << "FileHandler: No objects were passed to file \"" << nObjects << ". Skipping file";
    return bCmpResult;
  }

  // ----- Option parsing
  std::string sOption = opt;
  for (auto& ch : sOption) {
    ch = std::tolower(ch);
  }
  bool bSuppressCanvases = sOption.find("b") != std::string::npos;
  bool bForceCanvases    = sOption.find("c") != std::string::npos;
  bool bCmpExact         = sOption.find("e") != std::string::npos;
  bool bCmpChi2          = sOption.find("s") != std::string::npos;
  bool bCmpRatio         = sOption.find("u") != std::string::npos;

  LOG(info) << "FileHandler: processing objects: ...";
  std::vector<TNamed*> vpObjects(nVersions, nullptr);  // vector to keep object different versions
  for (int iObj = 0; iObj < nObjects; ++iObj) {
    bool skipObj = false;
    for (int iVer = 0; iVer < nVersions; ++iVer) {
      auto* pInputFile = static_cast<TFile*>(fpInputFiles->At(iVer));
      vpObjects[iVer]  = pInputFile->Get<TNamed>(fpObjDB->GetObject(fFileID, iObj).c_str());
      if (!vpObjects[iVer]) {
        //LOG(warn) << "FileHandler: object " << fpObjDB->GetObject(fFileID, iObj) << " is undefined for version "
        //          << fpObjDB->GetVersionLabel(iVer) << ". This object will be skipped";
        skipObj = true;
      }
    }  // iVer
    if (skipObj) {
      continue;
    }

    //
    if (iObj != 0 && nObjects % 500 == 0) {
      ReOpenOutputFile();
      LOG(info) << "FileHandler: object " << iObj << " / " << nObjects;
    }

    // Create an instance of an object handler
    std::unique_ptr<ObjectHandler> pObjHandler = nullptr;
    //LOG(info) << "FileHandler: processing object \"" << vpObjects[0]->GetName() << '\"';
    if (dynamic_cast<TProfile*>(vpObjects[0])) {
      pObjHandler = std::make_unique<Profile1DHandler>(iObj, fFileID, fDatasetID);
    }
    else if (dynamic_cast<TH2*>(vpObjects[0])) {
      pObjHandler = std::make_unique<Hist2DHandler>(iObj, fFileID, fDatasetID);
    }
    else if (dynamic_cast<TH1*>(vpObjects[0])) {
      pObjHandler = std::make_unique<Hist1DHandler>(iObj, fFileID, fDatasetID);
    }
    else {
      LOG(warn) << "FileHandler: Object " << fpObjDB->GetObject(fFileID, iObj) << " has a type \""
                << vpObjects[0]->ClassName()
                << "\", which is unknown to the cbm::qa::checker framework, it will be skipped";
      continue;
    }
    if (bCmpExact) {
      pObjHandler->SetComparisonMethod(ECmpMethod::Exact);  // Compare point-by-point (exact equality)
    }
    if (bCmpChi2) {
      pObjHandler->SetComparisonMethod(ECmpMethod::Chi2);  // Compare with chi2 test
    }
    if (bCmpRatio) {
      pObjHandler->SetComparisonMethod(ECmpMethod::Ratio);
    }
    pObjHandler->SetObjectDB(fpObjDB);
    pObjHandler->SetOutputDirectory(CreateNestedDirectory((fpObjDB->GetObject(fFileID, iObj))));
    pObjHandler->AddObjects(vpObjects);
    auto cmpResultObject = pObjHandler->CompareWithDefault();
    for (int iVer = 0; iVer < nVersions; ++iVer) {
      bCmpResult[iVer] = std::max(cmpResultObject[iVer], bCmpResult[iVer]);
    }

    if (!bSuppressCanvases
        && (bForceCanvases || std::any_of(cmpResultObject.begin(), cmpResultObject.end(), [](auto i) {
              return i != ECmpInference::StronglyEqual;
            }))) {
      pObjHandler->CreateCanvases(opt);
    }
    pObjHandler->Write();
    gFile->Flush();

    // Clean memory
    for (auto* pObj : vpObjects) {
      if (pObj) {
        delete pObj;
        pObj = nullptr;
      }
    }
  }  // iObj
  LOG(info) << "FileHandler: processing objects: done";
  return bCmpResult;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void FileHandler::ReOpenOutputFile()
{
  if (fpOutputFile.get()) {
    fpOutputFile->Flush();
    fpOutputFile->Close();
    fpOutputFile = nullptr;
    fpOutDir     = nullptr;
  }
  fpOutputFile    = std::make_unique<TFile>(fpObjDB->GetOutputPath().c_str(), "UPDATE");
  TString dirName = fpObjDB->GetDataset(fDatasetID) + "/" + fpObjDB->GetFileLabel(fFileID);
  fpOutDir        = fpOutputFile->Get<TDirectory>(dirName);
}
