/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmKfTarget.h
/// \brief  Target property initialization and access in CBM (source)
/// \since  02.09.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

// TODO: Move this class somewhere in the cbmroot/core

#include "CbmKfTarget.h"

#include "Logger.h"
#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoTube.h"
#include "TGeoVolume.h"

#include <iomanip>

using cbm::kf::Target;

Target* Target::fpInstance{nullptr};
std::mutex Target::fMutex;

// ---------------------------------------------------------------------------------------------------------------------
//
Target* Target::Instance()
{
  std::lock_guard<std::mutex> lock(fMutex);
  if (!fpInstance) {
    fpInstance = new Target();
    fpInstance->Init();
  }
  return fpInstance;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Target::FindTargetNode(TString& targetPath, TGeoNode*& targetNode)
{
  if (!targetNode) {  // init at the top of the tree
    targetNode = gGeoManager->GetTopNode();
    targetPath = "/" + TString(targetNode->GetName());
  }

  if (TString(targetNode->GetName()).Contains("target")) {
    return;
  }

  for (Int_t iNode = 0; iNode < targetNode->GetNdaughters(); ++iNode) {
    TGeoNode* newNode = targetNode->GetDaughter(iNode);
    TString newPath   = targetPath + "/" + newNode->GetName();
    FindTargetNode(newPath, newNode);
    if (newNode) {
      targetPath = newPath;
      targetNode = newNode;
      return;
    }
  }
  targetPath = "";
  targetNode = nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Target::Init()
{
  if (!gGeoManager) {
    throw std::logic_error("cbm::kf::Target: the TGeoManager instance is not initialized on this step. Please be "
                           "ensured to call the Target::Init() after the TGeoManager initialization.");
  }

  TString targetPath;
  TGeoNode* pTargetNode{nullptr};
  FindTargetNode(targetPath, pTargetNode);

  if (!pTargetNode) {
    throw std::runtime_error("cbm::kf::Target: the target node is not found in the setup");
  }

  Double_t local[3]  = {0., 0., 0.};  // target centre, local c.s.
  Double_t global[3] = {0};           // target centre, global c.s.
  gGeoManager->cd(targetPath);
  gGeoManager->GetCurrentMatrix()->LocalToMaster(local, global);
  fX = global[0];
  fY = global[1];
  fZ = global[2];

  if (const auto* pTube = dynamic_cast<TGeoTube*>(pTargetNode->GetVolume()->GetShape())) {
    fDz   = pTube->GetDz();
    fRmax = pTube->GetRmax();
  }
  else {
    throw std::logic_error("cbm::kf::Target: target is supposed to be a Tube, but it is not. Please, "
                           "provide a proper handling of the new target shape (return it's reference central point "
                           "and half of its thickness)");
  }
}
