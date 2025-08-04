/* Copyright (C) 2010-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Pascal Raisig, Alexandru Bercuci */

#include "CbmTaskTrdHitFinderParWrite.h"

#include "CbmTrdParAsic.h"
#include "CbmTrdParFasp.h"
#include "CbmTrdParModAsic.h"
#include "CbmTrdParModDigi.h"
#include "CbmTrdParModGeo.h"
#include "CbmTrdParSetAsic.h"
#include "CbmTrdParSetDigi.h"
#include "CbmTrdParSetGeo.h"
#include "TGeoPhysicalNode.h"
#include "TVector3.h"
#include "trd/Hitfind2DSetup.h"
#include "trd/HitfindSetup.h"
#include "yaml/Yaml.h"

#include <FairRootManager.h>
#include <FairRunAna.h>
#include <FairRuntimeDb.h>
#include <Logger.h>

#include <iomanip>
#include <iostream>
#include <vector>

using std::fixed;
using std::left;
using std::right;
using std::setprecision;
using std::setw;
using std::stringstream;

//_____________________________________________________________________
CbmTaskTrdHitFinderParWrite::CbmTaskTrdHitFinderParWrite() : FairTask("TrdClusterFinder", 1) {}

//_____________________________________________________________________
void CbmTaskTrdHitFinderParWrite::SetParContainers()
{
  FairRuntimeDb* rtdb = FairRun::Instance()->GetRuntimeDb();
  fAsicPar            = static_cast<CbmTrdParSetAsic*>(rtdb->getContainer("CbmTrdParSetAsic"));
  fDigiPar            = static_cast<CbmTrdParSetDigi*>(rtdb->getContainer("CbmTrdParSetDigi"));
  fGeoPar             = static_cast<CbmTrdParSetGeo*>(rtdb->getContainer("CbmTrdParSetGeo"));
}

//_____________________________________________________________________
InitStatus CbmTaskTrdHitFinderParWrite::Init()
{
  // Get the full geometry information of the detector gas layers and store
  // them with the CbmTrdModuleRec. This information can then be used for
  // transformation calculations
  if (!fGeoPar->LoadAlignVolumes()) {
    LOG(error) << GetName() << "::Init: GEO info for modules unavailable !";
    return kFATAL;
  }
  if (fDigiPar->GetNrOfModules() != fGeoPar->GetNrOfModules()) {
    LOG(fatal) << "CbmTaskTrdHitFinderParWrite::Init() - Geometry and parameter files"
               << " have different number of modules.";
  }

  LOG(info) << "Creating TRD + TRD2D parameters";

  // Sets of module IDs
  std::vector<uint16_t> trdModules;
  std::vector<uint16_t> trd2DModules;

  // Store IDs of 1D and 2D modules
  for (auto entry : fDigiPar->GetModuleMap()) {
    const auto moduleId = entry.first;

    // Get ASIC parameters for this module
    const CbmTrdParModAsic* asicPar = static_cast<const CbmTrdParModAsic*>(fAsicPar->GetModulePar(moduleId));
    if (!asicPar) continue;

    if (asicPar->GetAsicType() == CbmTrdDigi::eCbmTrdAsicType::kFASP) {
      trd2DModules.push_back(entry.first);
    }
    else {
      trdModules.push_back(entry.first);
    }
  }

  // Create setup files for each type
  cbm::algo::trd::HitfindSetup setup;
  cbm::algo::trd::Hitfind2DSetup setup2D;
  setup.modules.resize(trdModules.size());
  setup2D.modules.resize(trd2DModules.size());

  // Fill 2D setup files
  for (size_t mod = 0; mod < trd2DModules.size(); mod++) {
    const uint16_t moduleId = trd2DModules[mod];

    // Get Geometry parameters for module
    CbmTrdParModGeo* pGeo = static_cast<CbmTrdParModGeo*>(fGeoPar->GetModulePar(moduleId));

    // Get ASIC parameters for this module
    const CbmTrdParModAsic* asicPar = static_cast<const CbmTrdParModAsic*>(fAsicPar->GetModulePar(moduleId));

    if (!asicPar) continue;
    const CbmTrdParModDigi* digiPar = static_cast<const CbmTrdParModDigi*>(fDigiPar->GetModulePar(moduleId));

    cbm::algo::trd::Hitfind2DSetup::Mod& module = setup2D.modules[mod];
    module.padSizeX                             = digiPar->GetPadSizeX(0);
    module.padSizeY                             = digiPar->GetPadSizeY(0);
    module.address                              = moduleId;
    module.rowPar.resize(digiPar->GetNofRows());

    const double* tra_ptr = pGeo->GetNode()->GetMatrix()->GetTranslation();
    const double* rot_ptr = pGeo->GetNode()->GetMatrix()->GetRotationMatrix();
    memcpy(module.translation.data(), tra_ptr, 3 * sizeof(double));
    memcpy(module.rotation.data(), rot_ptr, 9 * sizeof(double));

    for (int row = 0; row < digiPar->GetNofRows(); row++) {

      cbm::algo::trd::Hitfind2DSetup::Row& rowPar = module.rowPar[row];
      rowPar.padPar.resize(digiPar->GetNofColumns());

      int rowInSector  = 0;
      const int sector = digiPar->GetSectorRow(row, rowInSector);  // Second is ref. TO DO: Return std::pair instead.

      for (int col = 0; col < digiPar->GetNofColumns(); col++) {
        cbm::algo::trd::Hitfind2DSetup::Pad& padPar = rowPar.padPar[col];
        TVector3 pos, posErr;
        digiPar->GetPadPosition(sector, col, rowInSector, pos, posErr);
        const std::array<double, 3> pos_ptr    = {pos[0], pos[1], pos[2]};
        const std::array<double, 3> posErr_ptr = {posErr[0], posErr[1], posErr[2]};
        memcpy(padPar.position.data(), pos_ptr.data(), 3 * sizeof(double));
        memcpy(padPar.positionError.data(), posErr_ptr.data(), 3 * sizeof(double));

        const size_t chan = row * digiPar->GetNofColumns() + col;
        const CbmTrdParFaspChannel *daqFaspChT(nullptr), *daqFaspChR(nullptr);
        asicPar->GetFaspChannelPar(chan, daqFaspChT, daqFaspChR);
        if (!daqFaspChR) {
          padPar.chRMasked = false;  // TODO implement case for not installed
        }
        else {
          padPar.chRMasked = daqFaspChR->IsMasked();
        }
        if (!daqFaspChT) {
          padPar.chTMasked = false;  // TODO implement case for not installed
        }
        else {
          padPar.chTMasked = daqFaspChT->IsMasked();
        }
      }
    }
  }  // for (size_t mod; mod < trd2DModules.size(); mod++) {


  // Fill 1D setup files
  for (size_t mod = 0; mod < trdModules.size(); mod++) {
    const uint16_t moduleId = trdModules[mod];

    // Get Geometry parameters for module
    CbmTrdParModGeo* pGeo = static_cast<CbmTrdParModGeo*>(fGeoPar->GetModulePar(moduleId));

    // Get ASIC parameters for this module
    const CbmTrdParModAsic* asicPar = static_cast<const CbmTrdParModAsic*>(fAsicPar->GetModulePar(moduleId));

    if (!asicPar) continue;
    const CbmTrdParModDigi* digiPar = static_cast<const CbmTrdParModDigi*>(fDigiPar->GetModulePar(moduleId));

    cbm::algo::trd::HitfindSetup::Mod& module = setup.modules[mod];
    module.padSizeX                           = digiPar->GetPadSizeX(0);
    module.padSizeY                           = digiPar->GetPadSizeY(0);
    module.padSizeErrX                        = digiPar->GetPadSizeX(1);
    module.padSizeErrY                        = digiPar->GetPadSizeY(1);
    module.orientation                        = digiPar->GetOrientation();
    module.address                            = moduleId;
    module.rowPar.resize(digiPar->GetNofRows());

    const double* tra_ptr = pGeo->GetNode()->GetMatrix()->GetTranslation();
    const double* rot_ptr = pGeo->GetNode()->GetMatrix()->GetRotationMatrix();
    memcpy(module.translation.data(), tra_ptr, 3 * sizeof(double));
    memcpy(module.rotation.data(), rot_ptr, 9 * sizeof(double));

    for (int row = 0; row < digiPar->GetNofRows(); row++) {

      cbm::algo::trd::HitfindSetup::Row& rowPar = module.rowPar[row];
      rowPar.padPar.resize(digiPar->GetNofColumns());

      int rowInSector  = 0;
      const int sector = digiPar->GetSectorRow(row, rowInSector);  // Second is ref. TO DO: Return std::pair instead.

      for (int col = 0; col < digiPar->GetNofColumns(); col++) {
        cbm::algo::trd::HitfindSetup::Pad& padPar = rowPar.padPar[col];
        TVector3 pos, posErr;
        digiPar->GetPadPosition(sector, col, rowInSector, pos, posErr);
        const std::array<double, 3> pos_ptr    = {pos[0], pos[1], pos[2]};
        const std::array<double, 3> posErr_ptr = {posErr[0], posErr[1], posErr[2]};
        memcpy(padPar.position.data(), pos_ptr.data(), 3 * sizeof(double));
        memcpy(padPar.positionError.data(), posErr_ptr.data(), 3 * sizeof(double));
      }
    }
  }  // for (size_t mod; mod < trdModules.size(); mod++) {


  /* Write Yaml files */

  cbm::algo::yaml::Dump dump;
  std::ofstream fout("TrdHitfinderPar.yaml");
  fout << dump(setup);
  fout.close();

  std::ofstream f2Dout("TrdHitfinder2DPar.yaml");
  f2Dout << dump(setup2D);
  f2Dout.close();

  return kSUCCESS;
}

ClassImp(CbmTaskTrdHitFinderParWrite)
