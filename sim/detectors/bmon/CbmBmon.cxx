/* Copyright (C) 2023 Facility for AntiProton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Eoin Clerkin [committer] */

#include "CbmBmon.h"

#include "CbmGeoBmon.h"
#include "CbmGeometryUtils.h"

#include "FairDetector.h"
#include "FairGeoInterface.h"
#include "FairGeoLoader.h"
#include "FairGeoNode.h"
#include "FairModule.h"
#include "FairRun.h"
#include "FairRuntimeDb.h"

#include "TFile.h"
#include "TGeoManager.h"
#include "TKey.h"
#include "TList.h"
#include "TObjArray.h"

CbmBmon::CbmBmon() : FairModule(), fCombiTrans(), fVolumeName("") {}
// CbmBmon::CbmBmon() : FairDetector(), fCombiTrans(), fVolumeName("") {}

CbmBmon::CbmBmon(const char* name, const char* title) : FairModule(name, title), fCombiTrans(), fVolumeName("") {}

// CbmBmon::CbmBmon(Bool_t active, const char* name)
/* CbmBmon::CbmBmon(const char* name, const char* Title)
  : FairDetector(name, fActive, ToIntegralType(ECbmModuleId::kBmon))
  , fStatusIn()
  , fStatusOut()
  , fEloss(0.)
  , fAddressMap()
  , fSetup(NULL)
  , fCombiTrans(NULL)
  , fProcessNeutrals(kFALSE)
{
} */

CbmBmon::~CbmBmon() {}

void CbmBmon::ConstructRootGeometry(TGeoMatrix*)
{
  if (Cbm::GeometryUtils::IsNewGeometryFile(fgeoName)) {
    LOG(info) << "Importing BMON geometry from ROOT file " << fgeoName.Data();
    Cbm::GeometryUtils::ImportRootGeometry(fgeoName, this, fCombiTrans);
  }
  else {
    LOG(info) << "Constructing BMON geometry from ROOT file " << fgeoName.Data();
    FairModule::ConstructGeometry();
    // FairModule::ConstructRootGeometry();
    // FairDetector::ConstructRootGeometry();
  }
}


void CbmBmon::ConstructGeometry()
{
  TString fileName = GetGeometryFileName();
  if (fileName.EndsWith(".root")) {
    // if (Cbm::GeometryUtils::IsNewGeometryFile(fgeoName)) {

    std::cout << "\tfgeoName=" << fgeoName << "\n\tdata=" << fgeoName.Data() << std::endl;


    std::cout << "\tfileName = " << fileName.Data() << std::endl;


    LOG(info) << "Importing BMON geometry from ROOT file " << fgeoName.Data();
    Cbm::GeometryUtils::ImportRootGeometry(fgeoName, this, fCombiTrans);
    // }
  }
  else
    LOG(fatal) << "Geometry format of BMON file " << fileName.Data() << " not supported.";
}

ClassImp(CbmBmon)
