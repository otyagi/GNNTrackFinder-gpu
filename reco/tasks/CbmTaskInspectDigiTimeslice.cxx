/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "CbmTaskInspectDigiTimeslice.h"

#include "CbmStsAddress.h"

#include <FairRootManager.h>
#include <Logger.h>


// -----   Constructor   -----------------------------------------------------
CbmTaskInspectDigiTimeslice::CbmTaskInspectDigiTimeslice() : FairTask("InspectOnlineRawDigis") {}
// ---------------------------------------------------------------------------


// -----   Destructor   ------------------------------------------------------
CbmTaskInspectDigiTimeslice::~CbmTaskInspectDigiTimeslice() {}
// ---------------------------------------------------------------------------


// -----   Execution   -------------------------------------------------------
void CbmTaskInspectDigiTimeslice::Exec(Option_t*)
{

  // --- Inspect digi data
  LOG(info) << GetName() << ": timeslice " << fNumTs << " with " << fBmonDigis->size() << " BMon digis "
            << fStsDigis->size() << " STS digis " << fMuchDigis->size() << " MUCH digis " << fTrdDigis->size()
            << " TRD digis " << fTofDigis->size() << " TOF digis " << fRichDigis->size() << " RICH digis ";

  // --- Run statistics
  fNumTs++;
}
// ----------------------------------------------------------------------------


// -----   End-of-run action   ------------------------------------------------
void CbmTaskInspectDigiTimeslice::Finish()
{
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Timeslices : " << fNumTs;
  LOG(info) << "=====================================";
}
// ----------------------------------------------------------------------------


// -----   Initialisation   ---------------------------------------------------
template<typename TVecobj>
const std::vector<TVecobj>* CbmTaskInspectDigiTimeslice::InitInput(FairRootManager* ioman)
{
  const std::vector<TVecobj>* vec = ioman->InitObjectAs<const std::vector<TVecobj>*>(TVecobj::GetBranchName());
  if (nullptr == vec) {
    LOG(error) << GetName() << ": No input branch " << TVecobj::GetBranchName() << " !";
    return nullptr;
  }
  LOG(info) << "--- Found branch " << TVecobj::GetBranchName() << " at " << vec;
  return vec;
}

InitStatus CbmTaskInspectDigiTimeslice::Init()
{
  // --- Get FairRootManager instance
  FairRootManager* ioman = FairRootManager::Instance();
  assert(ioman);

  LOG(info) << "==================================================";
  LOG(info) << GetName() << ": Initialising...";

  // --- Input data

  fBmonDigis = InitInput<CbmBmonDigi>(ioman);
  if (!fBmonDigis) {
    return kFATAL;
  }

  fStsDigis = InitInput<CbmStsDigi>(ioman);
  if (!fStsDigis) {
    return kFATAL;
  }

  fMuchDigis = InitInput<CbmMuchDigi>(ioman);
  if (!fMuchDigis) {
    return kFATAL;
  }

  fTrdDigis = InitInput<CbmTrdDigi>(ioman);
  if (!fTrdDigis) {
    return kFATAL;
  }

  fTofDigis = InitInput<CbmTofDigi>(ioman);
  if (!fTofDigis) {
    return kFATAL;
  }

  fRichDigis = InitInput<CbmRichDigi>(ioman);
  if (!fStsDigis) {
    return kFATAL;
  }

  LOG(info) << "==================================================";
  return kSUCCESS;
}
// ----------------------------------------------------------------------------


ClassImp(CbmTaskInspectDigiTimeslice)
