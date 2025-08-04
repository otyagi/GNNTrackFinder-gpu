/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese[committer] */

#include "CbmSourceDigiTimeslice.h"

#include "CbmTimeSlice.h"
#include "CbmTsEventHeader.h"

#include <FairRootManager.h>
#include <FairRun.h>
#include <Logger.h>

#include <utility>


// -----   Constructor   ------------------------------------------------------
CbmSourceDigiTimeslice::CbmSourceDigiTimeslice(const char* fileName) : fInputFileName(fileName) {}
// ----------------------------------------------------------------------------


// -----   Close   ------------------------------------------------------------
void CbmSourceDigiTimeslice::Close()
{
  LOG(info) << "Source: Closing after " << fNumTs << " timeslices";
  // Clear output vectors
  fBmonDigis->clear();
  fStsDigis->clear();
  fMuchDigis->clear();
  fTrdDigis->clear();
  fTofDigis->clear();
  fRichDigis->clear();
}
// ----------------------------------------------------------------------------


// -----   Initialisation   ---------------------------------------------------
template<typename TVecobj>
Bool_t CbmSourceDigiTimeslice::RegisterVector(FairRootManager* ioman, std::vector<TVecobj>*& vec)
{
  if (ioman->GetObject(TVecobj::GetBranchName())) {
    LOG(fatal) << "Source: Branch " << TVecobj::GetBranchName() << " already exists!";
    return kFALSE;
  }

  ioman->RegisterAny(TVecobj::GetBranchName(), vec, kTRUE);
  LOG(info) << "Source: Registered branch " << TVecobj::GetBranchName() << " at " << vec;

  return kTRUE;
}

Bool_t CbmSourceDigiTimeslice::Init()
{
  // Create input archive
  fArchive = std::make_unique<cbm::algo::RecoResultsInputArchive>(fInputFileName);
  LOG(info) << "Source: Reading from input archive " << fInputFileName;
  auto desc = fArchive->descriptor();
  LOG(info) << "      - Time created: " << desc.time_created();
  LOG(info) << "      - Host name   : " << desc.hostname();
  LOG(info) << "      - User name   : " << desc.username();

  // Create and register the Digi tree branches
  FairRootManager* ioman = FairRootManager::Instance();
  assert(ioman);

  if (!(fTsEventHeader = dynamic_cast<CbmTsEventHeader*>(FairRun::Instance()->GetEventHeader()))) {
    LOG(fatal) << "CbmSourceDigiTimeslice::Init() no CbmTsEventHeader was added to the run. Without it, we can not "
                  "store the UTC of the "
                  "Timeslices correctly. Hence, this causes a fatal. Please add it in the steering macro to the Run.";
    return kFALSE;
  }

  // TimeSlice. branch initialization
  if (ioman->GetObject("TimeSlice.")) {
    LOG(fatal) << "Source: Branch TimeSlice. already exists!";
    return kFALSE;
  }
  else {
    // NOTE: the max time of timeslice is 1.28e8, taken from CbmRecoUnpack.cxx
    fTimeslice = new CbmTimeSlice(0., 1.28e8 + 1.28e6);
    ioman->Register("TimeSlice.", "DAQ", fTimeslice, kTRUE);
  }

  fBmonDigis = new std::vector<CbmBmonDigi>();
  if (kFALSE == RegisterVector<CbmBmonDigi>(ioman, fBmonDigis)) {
    return kFALSE;
  }


  fStsDigis = new std::vector<CbmStsDigi>();
  if (kFALSE == RegisterVector<CbmStsDigi>(ioman, fStsDigis)) {
    return kFALSE;
  }


  fMuchDigis = new std::vector<CbmMuchDigi>();
  if (kFALSE == RegisterVector<CbmMuchDigi>(ioman, fMuchDigis)) {
    return kFALSE;
  }


  fTrdDigis = new std::vector<CbmTrdDigi>();
  if (kFALSE == RegisterVector<CbmTrdDigi>(ioman, fTrdDigis)) {
    return kFALSE;
  }


  fTofDigis = new std::vector<CbmTofDigi>();
  if (kFALSE == RegisterVector<CbmTofDigi>(ioman, fTofDigis)) {
    return kFALSE;
  }


  fRichDigis = new std::vector<CbmRichDigi>();
  if (kFALSE == RegisterVector<CbmRichDigi>(ioman, fRichDigis)) {
    return kFALSE;
  }


  return kTRUE;
}
// ----------------------------------------------------------------------------


// -----   Read one time slice from archive   ---------------------------------
Int_t CbmSourceDigiTimeslice::ReadEvent(UInt_t)
{

  // Clear output vectors
  fBmonDigis->clear();
  fStsDigis->clear();
  fMuchDigis->clear();
  fTrdDigis->clear();
  fTofDigis->clear();
  fRichDigis->clear();


  // Get next timeslice data from archive. Stop run if end of archive is reached.
  auto results = fArchive->get();
  if (fArchive->eos()) {
    LOG(info) << "Source: End of input archive; terminating run";
    return 1;
  }

  // Move event data from input archive to ROOT tree
  if (results == nullptr) {
    LOG(error) << "Source: Failed to read RecoResults from archive";
    return 1;
  }
  LOG(info) << "Source: Reading TS " << fNumTs << ", index " << results->TsIndex() << ", start "
            << results->TsStartTime() << ", contains Digis: \n"                                      //
            << "STS = " << results->StsDigis().size() << " MUCH = " << results->MuchDigis().size()   //
            << " TOF = " << results->TofDigis().size() << " BMON = " << results->BmonDigis().size()  //
            << " TRD = " << results->TrdDigis().size() << " TRD2D=" << results->Trd2dDigis().size()  //
            << " RICH=" << results->RichDigis().size();
  /// FIXME: Add PSD and FSD to the output of RawDigis in algo/global and reco/app
  // << " PSD=" << results->StsDigis().size() << " FSD=" << results->StsDigis().size();

  fTsEventHeader->SetTsIndex(results->TsIndex());
  fTsEventHeader->SetTsStartTime(results->TsStartTime());

  fTimeslice->SetStartTime(results->TsStartTime());

  std::move(results->BmonDigis().begin(), results->BmonDigis().end(), std::back_inserter(*fBmonDigis));
  std::move(results->StsDigis().begin(), results->StsDigis().end(), std::back_inserter(*fStsDigis));
  std::move(results->MuchDigis().begin(), results->MuchDigis().end(), std::back_inserter(*fMuchDigis));
  std::move(results->Trd2dDigis().begin(), results->Trd2dDigis().end(), std::back_inserter(*fTrdDigis));
  std::move(results->TrdDigis().begin(), results->TrdDigis().end(), std::back_inserter(*fTrdDigis));
  std::move(results->TofDigis().begin(), results->TofDigis().end(), std::back_inserter(*fTofDigis));
  std::move(results->RichDigis().begin(), results->RichDigis().end(), std::back_inserter(*fRichDigis));

  // Time-sort the TRD vector as we merged TRD1D and TRD2D (needed for compatibility with legacy unpackers)
  Timesort(fTrdDigis);

  // Update counters
  fNumTs++;

  return 0;
}
// ----------------------------------------------------------------------------


ClassImp(CbmSourceDigiTimeslice)
