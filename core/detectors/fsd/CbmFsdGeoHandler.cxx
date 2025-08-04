/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Lukas Chlad [committer] */

#include "CbmFsdGeoHandler.h"

#include "CbmFsdAddress.h"        // for CbmFsdAddress
#include "CbmFsdDetectorSpecs.h"  // for CbmFsdModuleSpecs

#include <Logger.h>  // for LOG, Logger

#include <TGeoBBox.h>     // for TGeoBBox
#include <TGeoManager.h>  // for TGeoManager, gGeoManager
#include <TGeoMatrix.h>   // for TGeoMatrix
#include <TGeoNode.h>     // for TGeoIterator, TGeoNode
#include <TGeoVolume.h>   // for TGeoVolume
#include <TVector3.h>     // for TVector3
#include <TVirtualMC.h>   // for TVirtualMC, gMC

#include <string>   // for operator<, stoul
#include <utility>  // for pair

#include <stddef.h>  // for size_t

using namespace std;

// constructor with initialization of maps
CbmFsdGeoHandler::CbmFsdGeoHandler() { InitMaps(); }

void CbmFsdGeoHandler::InitMaps()
{
  fUnitIdToSpecsMap.clear();
  fModuleIdToSpecsMap.clear();

  Int_t currentModuleId = -1;
  Int_t currentUnitId   = -1;

  TGeoIterator geoIterator(gGeoManager->GetTopNode()->GetVolume());

  TGeoNode* curNode;
  TGeoCombiTrans* unitToGlobalMatrix = nullptr;
  geoIterator.Reset();  // safety to reset to "cave" befor the loop starts
  while ((curNode = geoIterator())) {
    TString nodePath;
    geoIterator.GetPath(nodePath);
    if (!nodePath.Contains(fBranchStr)) {
      geoIterator.Skip();  // skip current branch when it is not FSD => should speed up
      continue;            // don't do anything for this branch
    }

    TString nodeName(curNode->GetName());
    if (nodeName.Contains(fUnitStr)) {
      currentUnitId = curNode->GetNumber();
      const TGeoMatrix* curUnitMatrix = geoIterator.GetCurrentMatrix();
      unitToGlobalMatrix              = new TGeoCombiTrans(*(curUnitMatrix));

      CbmFsdUnitSpecs* unitSpecs = new CbmFsdUnitSpecs();
      unitSpecs->fUnitId         = currentUnitId;
      unitSpecs->fUnitName       = static_cast<TString>(curNode->GetVolume()->GetName());
      unitSpecs->fNumModules     = curNode->GetNdaughters();
      fUnitIdToSpecsMap.insert(pair<Int_t, CbmFsdUnitSpecs*>(currentUnitId, unitSpecs));
    }
    if (nodeName.Contains(fModuleStr)) {
      currentModuleId             = curNode->GetNumber();


      TGeoMatrix* moduleToUnitMatrix = curNode->GetMatrix();
      TVector3 localModuleCoord(moduleToUnitMatrix->GetTranslation());
      TVector3 globalModuleCoord;
      if (!unitToGlobalMatrix) {
        LOG(fatal) << "No parent (unit) matrix initialized!!";
      }
      unitToGlobalMatrix->LocalToMaster(&localModuleCoord[0], &globalModuleCoord[0]);

      TGeoVolume* curScintillator = nullptr;
      for (int idn = 0; idn < curNode->GetNdaughters(); idn++) {
        TGeoNode* daughterNode = curNode->GetDaughter(idn);
        if (TString(daughterNode->GetName()).Contains(fActiveMatStr)) {
          curScintillator = daughterNode->GetVolume();
          break;
        }
      }
      const TGeoBBox* shape = (const TGeoBBox*) (curScintillator->GetShape());

      CbmFsdModuleSpecs* moduleSpecs = new CbmFsdModuleSpecs();
      moduleSpecs->fX                = globalModuleCoord[0];
      moduleSpecs->fY                = globalModuleCoord[1];
      moduleSpecs->fZ                = globalModuleCoord[2];
      moduleSpecs->dX                = shape->GetDX();
      moduleSpecs->dY                = shape->GetDY();
      moduleSpecs->dZ                = shape->GetDZ();
      moduleSpecs->fModuleId         = currentModuleId;
      moduleSpecs->fUnitId           = currentUnitId;
      fModuleIdToSpecsMap.insert(pair<Int_t, CbmFsdModuleSpecs*>(currentModuleId, moduleSpecs));
    }
  }

  LOG(info) << "CbmFsdGeoHandler has initialized maps";
  LOG(info) << "fUnitIdToSpecsMap.size() = " << fUnitIdToSpecsMap.size();
  LOG(info) << "fModuleIdToSpecsMap.size() = " << fModuleIdToSpecsMap.size();
}

CbmFsdModuleSpecs* CbmFsdGeoHandler::GetModuleSpecsById(Int_t modId)
{
  std::map<Int_t, CbmFsdModuleSpecs*>::iterator it;
  it = fModuleIdToSpecsMap.find(modId);
  if (it == fModuleIdToSpecsMap.end()) return nullptr;
  return it->second;
}

CbmFsdUnitSpecs* CbmFsdGeoHandler::GetUnitSpecsById(Int_t unitId)
{
  std::map<Int_t, CbmFsdUnitSpecs*>::iterator it;
  it = fUnitIdToSpecsMap.find(unitId);
  if (it == fUnitIdToSpecsMap.end()) return nullptr;
  return it->second;
}

int32_t CbmFsdGeoHandler::GetAddress(TString geoPath) const
{
  Int_t moduleID = GetCopyNumberByKey(geoPath, fModuleStr);
  Int_t unitID   = GetCopyNumberByKey(geoPath, fUnitStr);

  return CbmFsdAddress::GetAddress(unitID, moduleID);
}

int32_t CbmFsdGeoHandler::GetCurrentAddress(TVirtualMC* vmc) const
{
  Int_t moduleID = -1;
  Int_t unitID   = -1;

  Int_t upstreamOffset = 0;
  while (((TString) vmc->CurrentVolOffName(upstreamOffset)).Length() > 0) {
    if (((TString) vmc->CurrentVolOffName(upstreamOffset)).Contains(fUnitStr)) {
      vmc->CurrentVolOffID(upstreamOffset, unitID);
    }
    if (((TString) vmc->CurrentVolOffName(upstreamOffset)).Contains(fModuleStr)) {
      vmc->CurrentVolOffID(upstreamOffset, moduleID);
    }
    // if module and unit information was found one can
    // exit the loop
    if (moduleID > -1 && unitID > -1) { break; }
    upstreamOffset++;
  }

  return CbmFsdAddress::GetAddress(unitID, moduleID);
}

int32_t CbmFsdGeoHandler::GetCurrentAddress(TGeoManager* geoMan) const
{
  Int_t moduleID = -1;
  Int_t unitID   = -1;

  Int_t upstreamOffset = 0;
  while (upstreamOffset <= geoMan->GetLevel()) {
    TGeoNode* node = geoMan->GetMother(upstreamOffset);
    if (((TString) node->GetVolume()->GetName()).Contains(fUnitStr)) unitID = node->GetNumber();
    if (((TString) node->GetVolume()->GetName()).Contains(fModuleStr)) moduleID = node->GetNumber();
    upstreamOffset++;
  }

  return CbmFsdAddress::GetAddress(unitID, moduleID);
}

Int_t CbmFsdGeoHandler::GetCopyNumberByKey(TString geoPath, TString key) const
{
  Int_t copyNum = -1;

  // sanity checks
  if (!geoPath.Contains(fBranchStr)) { LOG(warning) << __func__ << ": In geoPath " << fBranchStr << " was not found!"; }
  else if (!geoPath.Contains(key)) {
    LOG(warning) << __func__ << ": In geoPath " << key << " was not found!";
  }
  else {
    Ssiz_t keyStart     = geoPath.Index(key);
    TString keyName     = geoPath(keyStart, geoPath.Index("/", keyStart) - keyStart);
    Ssiz_t copyNumStart = keyName.Last('_') + 1;
    TString copyNumStr  = keyName(copyNumStart, keyName.Length() - copyNumStart);
    if (!copyNumStr.IsDigit()) {
      LOG(warning) << __func__ << ": Expected numerical part from " << geoPath << " using key " << key << " is "
                   << copyNumStr << " which does not contain only digits!";
    }
    else {
      copyNum = copyNumStr.Atoi();
    }
  }

  return copyNum;
}
