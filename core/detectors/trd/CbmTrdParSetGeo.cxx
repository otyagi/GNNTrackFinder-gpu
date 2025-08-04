/* Copyright (C) 2018-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci, Florian Uhlig [committer] */

#include "CbmTrdParSetGeo.h"

#include "CbmTrdGeoHandler.h"  // for CbmTrdGeoHandler
#include "CbmTrdParModGeo.h"   // for CbmTrdParModGeo

#include <Logger.h>  // for LOG, Logger

#include <TGenericClassInfo.h>  // for TGenericClassInfo
#include <TGeoManager.h>        // for TGeoManager, gGeoManager
#include <TGeoNode.h>           // for TGeoNode
#include <TObjArray.h>          // for TObjArray
#include <TString.h>            // for operator+, TString, Form, TString::kI...

#include <map>      // for map, map<>::const_iterator, operator!=
#include <utility>  // for pair

#include <stdio.h>  // for printf

//________________________________________________________________________________________
CbmTrdParSetGeo::CbmTrdParSetGeo(const char* name, const char* title, const char* context)
  : CbmTrdParSet(name, title, context)
{
  LOG(debug) << "Constructor of CbmTrdParSetGeo";
  Init();
  LOG(debug) << "Constructor of CbmTrdParSetGeo --- finished";
}

//________________________________________________________________________________________
CbmTrdParSetGeo::~CbmTrdParSetGeo(void) {}

//________________________________________________________________________________________
bool CbmTrdParSetGeo::Init()
{
  CbmTrdGeoHandler geo;
  TGeoNode* topNode = gGeoManager->GetTopNode();
  TObjArray* nodes  = topNode->GetNodes();
  if (!nodes) {
    LOG(fatal) << "CbmTrdParSetGeo::Init: nodes is null!";
    return false;
  }

  CbmTrdParModGeo* pGeo(nullptr);
  for (Int_t iNode = 0; iNode < nodes->GetEntriesFast(); iNode++) {
    TGeoNode* node = static_cast<TGeoNode*>(nodes->At(iNode));
    if (!TString(node->GetName()).Contains("trd", TString::kIgnoreCase))
      continue;  // trd_vXXy top node, e.g. trd_v13a, trd_v14b
    TGeoNode* station = node;

    TObjArray* layers = station->GetNodes();
    for (Int_t iLayer = 0; iLayer < layers->GetEntriesFast(); iLayer++) {
      TGeoNode* layer = static_cast<TGeoNode*>(layers->At(iLayer));
      if (!TString(layer->GetName()).Contains("layer", TString::kIgnoreCase)) continue;  // only layers

      TObjArray* modules = layer->GetNodes();
      for (Int_t iModule = 0; iModule < modules->GetEntriesFast(); iModule++) {
        TGeoNode* module = static_cast<TGeoNode*>(modules->At(iModule));
        TString path     = TString("/") + topNode->GetName() + "/" + station->GetName() + "/" + layer->GetName() + "/"
                       + module->GetName();
        TObjArray* parts = module->GetNodes();
        for (Int_t iPart = 0; iPart < parts->GetEntriesFast(); iPart++) {
          TGeoNode* part = static_cast<TGeoNode*>(parts->At(iPart));
          // Put together the full path to the interesting volume, which
          // is needed to navigate with the geomanager to this volume.
          // Extract the geometry information (size, global position)
          // from this volume.
          if (TString(part->GetName()).Contains("gas", TString::kIgnoreCase)) {
            // legacy naming convention for the active volume
            path += Form("/%s", part->GetName());
          }
          else if (TString(part->GetName()).Contains("Volume", TString::kIgnoreCase)) {
            path += Form("/%s/gas_0", part->GetName());
          }
          else
            continue;  // only active gas volume
          LOG(debug) << "Adding detector with path " << path.Data();
          // Generate a physical node which has all needed information
          gGeoManager->cd(path.Data());
          Int_t address = geo.GetModuleAddress();
          pGeo          = new CbmTrdParModGeo(Form("TRD_%d", address), path.Data());
          pGeo->SetModuleId(address);
          addParam(pGeo);
        }
      }
    }
  }
  return true;
}

//________________________________________________________________________________________
bool CbmTrdParSetGeo::LoadAlignVolumes()
{
  if (!GetNrOfModules()) {
    LOG(error) << "CbmTrdParSetGeo::LoadAlignVolumes: No modules initialized. Nothing to do.";
    return false;
  }

  for (auto mod : fModuleMap)
    if (!((CbmTrdParModGeo*) mod.second)->SetNode()) return false;

  return true;
}

//________________________________________________________________________________________
void CbmTrdParSetGeo::Print(Option_t*) const
{
  std::map<Int_t, CbmTrdParMod*>::const_iterator imod = fModuleMap.begin();
  CbmTrdParModGeo* mod(nullptr);
  while (imod != fModuleMap.end()) {
    mod = (CbmTrdParModGeo*) imod->second;
    printf("Module %4d [%p]:\n", imod->first, (void*) mod);
    printf("  %s[%s]\n", mod->GetName(), mod->GetPath());
    Double_t xyz[3];
    mod->GetXYZ(xyz);
    printf("  X[%.2f] Y[%.2f] Z[%.2f] DX[%.2f] DY[%.2f] DZ[%.2f]\n", xyz[0], xyz[1], xyz[2], mod->GetDX(), mod->GetDY(),
           mod->GetDZ());
    imod++;
  }
}

// //________________________________________________________________________________________
// void CbmTrdParSetGeo::putParams(FairParamList* l)
// {
//   if (!l) return;
//   TObjArray *snodes(nullptr), *pnodes(nullptr);
//   //for()
//   l->addObject("FairGeoNodes Sensitive List", snodes);
//   l->addObject("FairGeoNodes Passive List", pnodes);
// }
//
// //________________________________________________________________________________________
// Bool_t CbmTrdParSetGeo::getParams(FairParamList* l)
// {
//   if (!l) return kFALSE;
//   TObjArray *snodes(nullptr), *pnodes(nullptr);
//   if (!l->fillObject("FairGeoNodes Sensitive List", snodes)) return kFALSE;
//   if (!l->fillObject("FairGeoNodes Passive List", pnodes)) return kFALSE;
//
//   //CbmTrdParModGeo *geo(nullptr);
//   for (Int_t i=0; i < snodes->GetEntriesFast(); i++){
// //     fModPar[moduleId[i]] = new CbmTrdParModGeo(GetName());
// //     fModPar
//
//     ((FairGeoNode*)(*snodes)[i])->print();
//   }
//   return kTRUE;
// }

ClassImp(CbmTrdParSetGeo)
