/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmQaCheckerCore.cxx
/// @brief  Core class of the QA checking framework (implementation)
/// @author S. Zharko <s.zharko@gsi.de>
/// @since  06.02.2023

#include "CbmQaCheckerCore.h"

#include "CbmQaCheckerFileHandler.h"
#include "Logger.h"
#include "TClonesArray.h"
#include "TFile.h"
//#include <boost/filesystem.hpp>
#include <regex>

using cbm::qa::checker::Core;

// ---------------------------------------------------------------------------------------------------------------------
//
Core::Core()
{
  // Define object names data base
  fpObjDB = std::make_shared<ObjectDB>();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Core::AddVersion(const char* version, const char* pathName) { fpObjDB->AddVersion(version, pathName); }

// ---------------------------------------------------------------------------------------------------------------------
//
void Core::AddDataset(const char* dataset) { fpObjDB->AddDataset(dataset); }

// ---------------------------------------------------------------------------------------------------------------------
//
void Core::RegisterOutFile(const char* filename)
{
  LOG(info) << "Core: Registering output file: " << filename;
  fpObjDB->SetOutputPath(filename);
}

// ---------------------------------------------------------------------------------------------------------------------
//
int Core::Process(Option_t* opt)
{
  // ----- Init the object database
  fpObjDB->Init();

  PrepareOutputFile();

  int nDatasets = fpObjDB->GetNofDatasets();
  int nFiles    = fpObjDB->GetNofFiles();
  int nVersions = fpObjDB->GetNofVersions();

  std::vector<ECmpInference> cmpSummary(nVersions * nFiles * nDatasets, ECmpInference::StronglyEqual);

  // ----- Process datasets and files
  for (int iDS = 0; iDS < nDatasets; ++iDS) {
    for (int iFile = 0; iFile < nFiles; ++iFile) {
      // Create and process a file handler
      auto pFileHandler = std::make_unique<FileHandler>(fpObjDB, iDS, iFile);
      auto cmpRes       = pFileHandler->Process(opt);
      for (int iVer = 0; iVer < nVersions; ++iVer) {
        cmpSummary[nVersions * (iDS * nFiles + iFile) + iVer] = cmpRes[iVer];
      }
    }  // iFile
  }    // iDS

  ECmpInference res = ECmpInference::StronglyEqual;
  LOG(info) << "Summary:";
  for (int iDS{0}; iDS < nDatasets; ++iDS) {
    LOG(info) << "\tDataset: " << fpObjDB->GetDataset(iDS);
    for (int iFile{0}; iFile < nFiles; ++iFile) {
      LOG(info) << "\t\tFile: " << fpObjDB->GetFileLabel(iDS);
      for (int iVer{0}; iVer < nVersions; ++iVer) {
        auto versionInference = cmpSummary[nVersions * (iDS * nFiles + iFile) + iVer];
        LOG(info) << "\t\t\tVersion: " << fpObjDB->GetVersionLabel(iVer)
                  << " (path: " << fpObjDB->GetInputFileName(iVer, iFile, iDS) << "): " << ToString(versionInference);
        res = std::max(versionInference, res);
      }
    }
  }
  return static_cast<int>(res);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Core::SetFromYAML(const char* configName) { fpObjDB->ReadFromYAML(configName); }

// ---------------------------------------------------------------------------------------------------------------------
//
void Core::PrepareOutputFile()
{
  TFile outFile(fpObjDB->GetOutputPath().c_str(), "RECREATE");
  for (int iDS = 0; iDS < fpObjDB->GetNofDatasets(); ++iDS) {
    auto* pDSDir = outFile.mkdir(fpObjDB->GetDataset(iDS).c_str());
    for (int iFile = 0; iFile < fpObjDB->GetNofFiles(); ++iFile) {
      pDSDir->mkdir(fpObjDB->GetFileLabel(iFile).c_str());
    }
  }
  outFile.Close();
}
