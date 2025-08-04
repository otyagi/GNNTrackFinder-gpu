/* Copyright (C) 2020-2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer], Dominik Smith, Alexandru Bercuci */

#include "CbmAlgoBuildRawEvents.h"

/// CBM headers
#include "CbmBmonDigi.h"
#include "CbmEvent.h"
#include "CbmFsdDigi.h"
#include "CbmMuchBeamTimeDigi.h"
#include "CbmMuchDigi.h"
#include "CbmPsdDigi.h"
#include "CbmRichDigi.h"
#include "CbmStsDigi.h"
#include "CbmTofDigi.h"
#include "CbmTrdAddress.h"
#include "CbmTrdDigi.h"
#include "TimesliceMetaData.h"

/// FAIRROOT headers
#include <FairRootManager.h>
#include <FairRunOnline.h>
#include <Logger.h>

/// FAIRSOFT headers (geant, boost, ...)
#include <TCanvas.h>
#include <TClonesArray.h>
#include <TDirectoryFile.h>
#include <TH1.h>
#include <TH2.h>
#include <THttpServer.h>
#include <TProfile.h>
#include <TStopwatch.h>

/// C/C++ headers
#include <algorithm>

#define VERBOSE 0

template<>
void CbmAlgoBuildRawEvents::LoopOnSeeds<Double_t>();

Bool_t CbmAlgoBuildRawEvents::InitAlgo()
{
  LOG(info) << "CbmAlgoBuildRawEvents::InitAlgo => Starting sequence";

  if (fbGetTimings) {
    fTimer = new TStopwatch;
    fTimer->Start();
  }

  /// Check if reference detector is set and seed data are available,
  /// otherwise look for explicit seed times
  if (fRefDet.detId == ECbmModuleId::kNotExist) {
    if (fSeedTimes == nullptr) {
      LOG(fatal) << "No reference detector set and no seed times supplied, stopping there!";
    }
  }
  else {
    if (fSeedTimes != nullptr) {
      LOG(fatal) << "Cannot have explicit seed times and reference detector, stopping there!";
    }
    if (kFALSE == CheckDataAvailable(fRefDet)) {
      LOG(fatal) << "Reference detector set but no digi input found, stopping there!";
    }
  }

  /// Check if data for detectors in selection list are available
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    if (kFALSE == CheckDataAvailable(*det)) {
      LOG(fatal) << "No digi input for one of selection detector, stopping there!";
    }
  }

  /// Access the TS metadata to know TS start time if needed
  if (fbUseTsMetaData) {
    if (!fTimeSliceMetaDataArray) {
      LOG(fatal) << "No TS metadata input found"
                 << " => Please check in the unpacking macro if the following line was "
                    "present!"
                 << std::endl
                 << "source->SetWriteOutputFlag(kTRUE);  // For writing TS metadata";
    }
  }
  if (fbFillHistos) {
    CreateHistograms();
  }
  if (fTimer != nullptr) {
    fTimer->Stop();
    Double_t rtime = fTimer->RealTime();
    Double_t ctime = fTimer->CpuTime();
    LOG(info) << "CbmAlgoBuildRawEvents::Init(): Real time " << rtime << " s, CPU time " << ctime << " s";
  }

  LOG(info) << "CbmAlgoBuildRawEvents::InitAlgo => Done";
  return kTRUE;
}

void CbmAlgoBuildRawEvents::Finish()
{
  if (fbGetTimings) {
    PrintTimings();
  }
}

void CbmAlgoBuildRawEvents::PrintTimings()
{
  if (fTimer == nullptr) {
    LOG(fatal) << "Trying to print timings but timer not set";
  }
  else {
    Double_t rtime = fTimer->RealTime();
    Double_t ctime = fTimer->CpuTime();
    LOG(info) << "CbmAlgoBuildRawEvents: Real time " << rtime << " s, CPU time " << ctime << " s";
  }
}

void CbmAlgoBuildRawEvents::ClearEventVector()
{
  /// Need to delete the object the pointer points to first
  int counter = 0;
  for (CbmEvent* event : fEventVector) {
    LOG(debug) << "Event " << counter << " has " << event->GetNofData() << " digis";
    delete event;
    counter++;
  }
  fEventVector.clear();
}

void CbmAlgoBuildRawEvents::ProcessTs()
{
  LOG_IF(info, fuNrTs % 1000 == 0) << "Begin of TS " << fuNrTs;
  TStopwatch timerTs;
  timerTs.Start();

  if (fTimer != nullptr) {
    fTimer->Start(kFALSE);
  }
  InitTs();
  InitSeedWindow();
  BuildEvents();

  /// Store last event with trigger if not done by other seed
  if (nullptr != fCurrentEvent) {
    /// TODO: store start time of current event ?
    //        fCurrentEvent->SetStartTime( fPrevTime ); // Replace Seed time with time of first digi in event?
    fCurrentEvent->SetEndTime(fdPrevEvtEndTime);
    if (fbBmonInUse) {
      /// Bmon used either as Seed or selected detector
      SetBmonEventTime(fCurrentEvent);
    }
    fEventVector.push_back(fCurrentEvent);
    fuCurEv++;

    /// Prevent building over TS edge
    fCurrentEvent = nullptr;
  }

  if (fbFillHistos) {
    timerTs.Stop();
    fhCpuTimePerTs->Fill(fuNrTs, timerTs.CpuTime() * 1000.);
    fhRealTimePerTs->Fill(fuNrTs, timerTs.RealTime() * 1000.);
    timerTs.Start();
  }

  LOG(debug) << "Found " << fEventVector.size() << " triggered events";
  if (fbFillHistos) {
    FillHistos();
  }
  if (fTimer != nullptr) {
    fTimer->Stop();
  }

  if (fbFillHistos) {
    timerTs.Stop();
    fhCpuTimePerTsHist->Fill(fuNrTs, timerTs.CpuTime() * 1000.);
    fhRealTimePerTsHist->Fill(fuNrTs, timerTs.RealTime() * 1000.);
  }

  fuNrTs++;
}

void CbmAlgoBuildRawEvents::InitTs()
{
  /// Reset TS based variables (analysis per TS = no building over the border)
  fuCurEv = 0;

  /// Reference detector
  if (fRefDet.detId != ECbmModuleId::kNotExist) {
    fRefDet.fuStartIndex = 0;
    fRefDet.fuEndIndex   = 0;
  }
  /// Loop on detectors in selection list
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    (*det).fuStartIndex = 0;
    (*det).fuEndIndex   = 0;
  }
}

void CbmAlgoBuildRawEvents::InitSeedWindow()
{
  /// Access the TS metadata if needed to know TS start time and overlap size
  Double_t dTsStartTime  = fdTsStartTime;
  Double_t dOverlapStart = fdTsStartTime + fdTsLength;
  Double_t dOverlapSize  = fdTsOverLength;

  if (fbUseTsMetaData) {
    const TimesliceMetaData* pTsMetaData = dynamic_cast<TimesliceMetaData*>(fTimeSliceMetaDataArray->At(0));
    if (nullptr == pTsMetaData)
      LOG(fatal) << Form("CbmAlgoBuildRawEvents::LoopOnSeeds => "
                         "No TS metadata found for TS %6u.",
                         fuNrTs);
    dTsStartTime  = pTsMetaData->GetStartTime();
    dOverlapStart = pTsMetaData->GetOverlapStartTime();
    dOverlapSize  = pTsMetaData->GetOverlapDuration();
  }

  /// Print warning in first TS if time window borders out of potential overlap
  if ((0.0 < fdEarliestTimeWinBeg && dOverlapSize < fdLatestTimeWinEnd) || (dOverlapSize < fdWidestTimeWinRange)) {
    LOG(warning) << "CbmAlgoBuildRawEvents::LoopOnSeeds => "
                 << Form("Event window not fitting in TS overlap, risk of "
                         "incomplete events: %f %f %f %f",
                         fdEarliestTimeWinBeg, fdLatestTimeWinEnd, fdWidestTimeWinRange, dOverlapSize);
  }  // if end of event window does not fit in overlap for a seed at edge of TS core

  /// Define an acceptance window for the seeds in order to use the overlap
  /// part of the TS to avoid incomplete events
  if (fbIgnoreTsOverlap) {
    fdSeedWindowBeg = dTsStartTime;
    fdSeedWindowEnd = dOverlapStart;
  }
  else {
    fdSeedWindowBeg = dTsStartTime + (0.0 < fdEarliestTimeWinBeg ? 0.0 : -fdEarliestTimeWinBeg);
    fdSeedWindowEnd = dOverlapStart + (0.0 < fdEarliestTimeWinBeg ? 0.0 : -fdEarliestTimeWinBeg);
  }
}

void CbmAlgoBuildRawEvents::BuildEvents()
{
  /// Call LoopOnSeed with proper template argument
  switch (fRefDet.detId) {
    case ECbmModuleId::kSts: {
      LoopOnSeeds<CbmStsDigi>();
      break;
    }
    case ECbmModuleId::kMuch: {
      if (fbUseMuchBeamtimeDigi) {
        LoopOnSeeds<CbmMuchBeamTimeDigi>();
      }
      else {
        LoopOnSeeds<CbmMuchDigi>();
      }
      break;
    }
    case ECbmModuleId::kTrd2d:  // Same data storage as trd 1d
    case ECbmModuleId::kTrd: {
      LoopOnSeeds<CbmTrdDigi>();
      break;
    }
    case ECbmModuleId::kTof: {
      LoopOnSeeds<CbmTofDigi>();
      break;
    }
    case ECbmModuleId::kRich: {
      LoopOnSeeds<CbmRichDigi>();
      break;
    }
    case ECbmModuleId::kPsd: {
      LoopOnSeeds<CbmPsdDigi>();
      break;
    }
    case ECbmModuleId::kFsd: {
      LoopOnSeeds<CbmFsdDigi>();
      break;
    }
    case ECbmModuleId::kBmon: {
      LoopOnSeeds<CbmBmonDigi>();
      break;
    }
    case ECbmModuleId::kNotExist: {  //explicit seed times
      LoopOnSeeds<Double_t>();
      break;
    }
    default: {
      LOG(fatal) << "CbmAlgoBuildRawEvents::BuildEvents => "
                 << "Trying to search event seeds with unsupported det: " << fRefDet.sName;
      break;
    }
  }
}

template<>
void CbmAlgoBuildRawEvents::LoopOnSeeds<Double_t>()
{
  if (ECbmModuleId::kNotExist == fRefDet.detId) {
    const UInt_t uNbSeeds = fSeedTimes->size();
    /// Loop on size of vector
    for (UInt_t uSeed = 0; uSeed < uNbSeeds; ++uSeed) {
      LOG(debug) << Form("Checking seed %6u / %6u", uSeed, uNbSeeds);
      Double_t dTime = fSeedTimes->at(uSeed);

      /// Check if seed in acceptance window (is this needed here?)
      if (dTime < fdSeedWindowBeg) {
        LOG(debug4) << Form("CbmAlgoBuildRawEvents :: reject for time %f < %f\n", dTime, fdSeedWindowBeg);
        continue;
      }
      else if (fdSeedWindowEnd < dTime) {
        break;
      }
      /// Check Seed and build event if needed
      CheckSeed(dTime, uSeed);
    }
  }
  else {
    LOG(fatal) << "Trying to read explicit seeds while reference detector is set.";
  }
}

template<class DigiSeed>
void CbmAlgoBuildRawEvents::LoopOnSeeds()
{
  const UInt_t uNbRefDigis = GetNofDigis(fRefDet.detId);
  /// Loop on size of vector
  for (UInt_t uDigi = 0; uDigi < uNbRefDigis; ++uDigi) {
    LOG(debug) << Form("Checking seed %6u / %6u", uDigi, uNbRefDigis);
    const DigiSeed* pDigi = GetDigi<DigiSeed>(uDigi);
    // hack for mCBM2024 data and Bmon station selection
    if (fRefDet.detId == ECbmModuleId::kBmon && !filterBmon(pDigi->GetAddress())) continue;

    const Double_t dTime = pDigi->GetTime();
    //printf("time = %f %d %d\n", dTime, CbmTofAddress::GetChannelSide(add), CbmTofAddress::GetChannelId(add));

    /// Check if seed in acceptance window
    if (dTime < fdSeedWindowBeg) {
      LOG(debug4) << Form("CbmAlgoBuildRawEvents :: reject for time %f < %f\n", dTime, fdSeedWindowBeg);
      continue;
    }
    else if (fdSeedWindowEnd < dTime) {
      break;
    }
    /// Check Seed and build event if needed
    CheckSeed(dTime, uDigi);
  }
}

Double_t CbmAlgoBuildRawEvents::GetSeedTimeWinRange()
{
  if (ECbmModuleId::kNotExist != fRefDet.detId) {
    return fRefDet.GetTimeWinRange();
  }
  else {
    return fdSeedTimeWinEnd - fdSeedTimeWinBeg;
  }
}

void CbmAlgoBuildRawEvents::CheckSeed(Double_t dSeedTime, UInt_t uSeedDigiIdx)
{
  /// If previous event valid and event overlap not allowed, check if we are in overlap
  /// and react accordingly

  LOG(debug4) << Form("CbmAlgoBuildRawEvents :: CheckSeed(%f, %d)\n", dSeedTime, uSeedDigiIdx);
  if (nullptr != fCurrentEvent
      && (EOverlapModeRaw::AllowOverlap != fOverMode || dSeedTime - fdPrevEvtTime < GetSeedTimeWinRange())
      && dSeedTime - fdPrevEvtTime < fdWidestTimeWinRange) {
    /// Within overlap range
    switch (fOverMode) {
      case EOverlapModeRaw::NoOverlap: {
        /// No overlap allowed => reject
        LOG(debug1) << "Reject seed due to overlap";
        return;
        break;
      }
      case EOverlapModeRaw::MergeOverlap: {
        /// Merge overlap mode => do nothing and go on filling current event
        break;
      }
      case EOverlapModeRaw::AllowOverlap: {
        /// In allow overlap mode => reject only if reference det is in overlap
        /// to avoid cloning events due to single seed cluster
        LOG(debug1) << "Reject seed because part of cluster of previous one";
        return;
        break;
      }
      default: break;
    }
  }  // if( prev Event exists and mode forbiden overlap present )
  else {
    /// Out of overlap range or in overlap allowed mode
    /// => store previous event if not empty and create new one
    if (nullptr != fCurrentEvent) {
      /// TODO: store start time of current event ?
      //        fCurrentEvent->SetStartTime( fPrevTime ); // Replace Seed time with time of first digi in event?
      fCurrentEvent->SetEndTime(fdPrevEvtEndTime);
      if (fbBmonInUse) {
        /// Bmon used either as Seed or selected detector
        SetBmonEventTime(fCurrentEvent);
      }
      fEventVector.push_back(fCurrentEvent);

      fuCurEv++;
    }
    if (fbBmonInUse) {
      /// Bmon used either as Seed or selected detector
      fCurrentEvent = new CbmEvent(fuCurEv, -1, 0.);
    }
    else {
      fCurrentEvent = new CbmEvent(fuCurEv, dSeedTime, 0.);
    }
  }  // else of if( prev Event exists and mode forbiden overlap present )

  if (fRefDet.detId != ECbmModuleId::kNotExist) {
    /// If window open for reference detector, search for other reference Digis matching it
    /// Otherwise only add the current seed
    if (fRefDet.fdTimeWinBeg < fRefDet.fdTimeWinEnd) {
      SearchMatches(dSeedTime, fRefDet);
      /// Also add the seed if the window starts after the seed
      if (0 < fRefDet.fdTimeWinBeg) {
        AddDigiToEvent(fRefDet, uSeedDigiIdx);
      }
    }
    else {
      AddDigiToEvent(fRefDet, uSeedDigiIdx);
    }
  }


  /// Check if this reference detector passes the trigger conditions to "exit early"
  /// then check for each detectors if it also passes
  /// => Replaces the call to CheckTriggerCondition after a complete loop
  if (kTRUE == CheckTriggerConditions(fCurrentEvent, fRefDet)) {

    /// Search for matches for each detector in selection list
    bool bAllTriggersOk = true;
    for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
      SearchMatches(dSeedTime, *det);

      /// Check if this det pass the trigger conditions to "exit early"
      if (kFALSE == CheckTriggerConditions(fCurrentEvent, *det)) {  //
        bAllTriggersOk = false;
        break;
      }
    }
    if (bAllTriggersOk) {
      fdPrevEvtTime = dSeedTime;

      /// In case of NoOverlap or MergeOverlap, we can and should start checking the next window
      /// from end of current window in order to save CPU and avoid duplicating
      if (EOverlapModeRaw::NoOverlap == fOverMode || EOverlapModeRaw::MergeOverlap == fOverMode) {
        /// Update reference detector
        if (fRefDet.detId != ECbmModuleId::kNotExist) {
          fRefDet.fuStartIndex = fRefDet.fuEndIndex;
        }
        /// Loop on selection detectors
        for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
          (*det).fuStartIndex = (*det).fuEndIndex;
        }
      }
      // LOG(info) << Form("Accept seed %9.0f due to Selection Trigger requirements", dSeedTime);
    }
    else {
      // LOG(info) << Form("Reject seed %9.0f due to Selection Trigger requirements", dSeedTime);
      LOG(debug1) << "Reject seed due to Trigger requirements";
      delete fCurrentEvent;
      fCurrentEvent = nullptr;  /// delete does NOT set a pointer to nullptr...
    }
  }
  else {
    // LOG(info) << Form("Reject seed %9.0f due to Reference Trigger requirements", dSeedTime);
    LOG(debug1) << "Reject seed due to Trigger requirements";
    delete fCurrentEvent;
    fCurrentEvent = nullptr;  /// delete does NOT set a pointer to nullptr...
  }
  /*
  /// Search for matches for each detector in selection list
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    SearchMatches(dSeedTime, *det);
  }

  CheckTriggerCondition(dSeedTime);
  */
}

//----------------------------------------------------------------------
/// Specialization of the GetDigi variants has to happen before first usage

template<>
const CbmStsDigi* CbmAlgoBuildRawEvents::GetDigi(UInt_t uDigi)
{
  return &((*fStsDigis)[uDigi]);
}
template<>
const CbmMuchBeamTimeDigi* CbmAlgoBuildRawEvents::GetDigi(UInt_t uDigi)
{
  return &((*fMuchBeamTimeDigis)[uDigi]);
}
template<>
const CbmMuchDigi* CbmAlgoBuildRawEvents::GetDigi(UInt_t uDigi)
{
  return &((*fMuchDigis)[uDigi]);
}
template<>
const CbmTrdDigi* CbmAlgoBuildRawEvents::GetDigi(UInt_t uDigi)
{
  return &((*fTrdDigis)[uDigi]);
}
template<>
const CbmTofDigi* CbmAlgoBuildRawEvents::GetDigi(UInt_t uDigi)
{
  return &((*fTofDigis)[uDigi]);
}
template<>
const CbmRichDigi* CbmAlgoBuildRawEvents::GetDigi(UInt_t uDigi)
{
  return &((*fRichDigis)[uDigi]);
}
template<>
const CbmPsdDigi* CbmAlgoBuildRawEvents::GetDigi(UInt_t uDigi)
{
  return &((*fPsdDigis)[uDigi]);
}
template<>
const CbmFsdDigi* CbmAlgoBuildRawEvents::GetDigi(UInt_t uDigi)
{
  return &((*fFsdDigis)[uDigi]);
}
template<>
const CbmBmonDigi* CbmAlgoBuildRawEvents::GetDigi(UInt_t uDigi)
{
  return &((*fBmonDigis)[uDigi]);
}


//----------------------------------------------------------------------

void CbmAlgoBuildRawEvents::SearchMatches(Double_t dSeedTime, RawEventBuilderDetector& detMatch)
{
  switch (detMatch.detId) {
    case ECbmModuleId::kSts: {
      SearchMatches<CbmStsDigi>(dSeedTime, detMatch);
      break;
    }
    case ECbmModuleId::kMuch: {
      if (fbUseMuchBeamtimeDigi) {
        SearchMatches<CbmMuchBeamTimeDigi>(dSeedTime, detMatch);
      }
      else {
        SearchMatches<CbmMuchDigi>(dSeedTime, detMatch);
      }
      break;
    }
    case ECbmModuleId::kTrd2d:  // Same data storage as trd 1d
    case ECbmModuleId::kTrd: {
      SearchMatches<CbmTrdDigi>(dSeedTime, detMatch);
      break;
    }
    case ECbmModuleId::kTof: {
      SearchMatches<CbmTofDigi>(dSeedTime, detMatch);
      break;
    }
    case ECbmModuleId::kRich: {
      SearchMatches<CbmRichDigi>(dSeedTime, detMatch);
      break;
    }
    case ECbmModuleId::kPsd: {
      SearchMatches<CbmPsdDigi>(dSeedTime, detMatch);
      break;
    }
    case ECbmModuleId::kFsd: {
      SearchMatches<CbmFsdDigi>(dSeedTime, detMatch);
      break;
    }
    case ECbmModuleId::kBmon: {
      SearchMatches<CbmBmonDigi>(dSeedTime, detMatch);
      break;
    }
    default: {
      LOG(fatal) << "CbmAlgoBuildRawEvents::LoopOnSeeds => "
                 << "Trying to search matches with unsupported det: " << detMatch.sName << std::endl
                 << "You may want to add support for it in the method.";
      break;
    }
  }
}

template<class DigiCheck>
void CbmAlgoBuildRawEvents::SearchMatches(Double_t dSeedTime, RawEventBuilderDetector& detMatch)
{
  /// This algo relies on time sorted vectors for the selected detectors
  UInt_t uLocalIndexStart = detMatch.fuStartIndex;
  UInt_t uLocalIndexEnd   = detMatch.fuStartIndex;
  LOG(debug4) << Form("CbmAlgoBuildRawEvents :: SearchMatches(%f, %s)\n", dSeedTime, detMatch.sName.data());
  /// Check the Digis until out of window
  const UInt_t uNbSelDigis = GetNofDigis(detMatch.detId);
  /// Loop on size of vector
  for (UInt_t uDigi = detMatch.fuStartIndex; uDigi < uNbSelDigis; ++uDigi) {
    const DigiCheck* pDigi   = GetDigi<DigiCheck>(uDigi);
    const Double_t dTime     = pDigi->GetTime();
    const Double_t dTimeDiff = dTime - dSeedTime;
    LOG(debug4) << detMatch.sName << Form(" => Checking match %6u / %6u, dt %f", uDigi, uNbSelDigis, dTimeDiff);
    // int32_t add = pDigi->GetAddress(), dId[3] = {-1, -1, -1};
    // switch (detMatch.detId) {
    //   case ECbmModuleId::kBmon:
    //     dId[0] = CbmTofAddress::GetChannelSide(add);
    //     dId[1] = CbmTofAddress::GetChannelId(add);
    //     break;
    //   case ECbmModuleId::kSts:
    //     dId[0] = CbmStsAddress::GetElementId(add, EStsElementLevel::kStsUnit);
    //     dId[1] = CbmStsAddress::GetElementId(add, EStsElementLevel::kStsLadder);
    //     dId[2] = CbmStsAddress::GetElementId(add, EStsElementLevel::kStsModule);
    //     break;
    //   case ECbmModuleId::kTrd:
    //   case ECbmModuleId::kTrd2d:
    //     dId[0] = CbmTrdAddress::GetLayerId(add);
    //     dId[1] = CbmTrdAddress::GetModuleId(add);
    //     dId[2] = CbmTrdAddress::GetModuleAddress(add);
    //     break;
    //   case ECbmModuleId::kTof:
    //     dId[0] = CbmTofAddress::GetSmId(add);
    //     dId[1] = CbmTofAddress::GetSmType(add);
    //     dId[2] = CbmTofAddress::GetRpcId(add);
    //     break;
    //   default: break;
    // }
    // LOG(debug4) << Form("CbmAlgoBuildRawEvents :: Checking match %6u / %6u, dt=%f %s[%d %d %d]\n", uDigi, uNbSelDigis, dTimeDiff, detMatch.sName.data(), dId[0], dId[1], dId[2]);

    /// Check if within time window, update start/stop indices if needed
    if (dTimeDiff < detMatch.fdTimeWinBeg) {
      ++uLocalIndexStart;
      continue;
    }
    else if (detMatch.fdTimeWinEnd < dTimeDiff) {
      /// Store as end the first digi out of window to avoid double counting in case of
      /// merged overlap event mode
      uLocalIndexEnd = uDigi;
      break;
    }

    // Filter TRD2D digis if 1D and reverse
    if (detMatch.detId == ECbmModuleId::kTrd) {
      const CbmTrdDigi* pTrdDigi = GetDigi<CbmTrdDigi>(uDigi);
      if (pTrdDigi->GetType() == CbmTrdDigi::eCbmTrdAsicType::kFASP) {  //
        continue;
      }
    }
    else if (detMatch.detId == ECbmModuleId::kTrd2d) {
      const CbmTrdDigi* pTrdDigi = GetDigi<CbmTrdDigi>(uDigi);
      if (pTrdDigi->GetType() == CbmTrdDigi::eCbmTrdAsicType::kSPADIC) {  //
        continue;
      }
    }

    AddDigiToEvent(detMatch, uDigi);
    if (fdPrevEvtEndTime < dTime) fdPrevEvtEndTime = dTime;
  }
  /// catch the case where we reach the end of the vector before being out of the time window
  if (uLocalIndexEnd < uLocalIndexStart) uLocalIndexEnd = uNbSelDigis;

  /// Update the StartIndex and EndIndex for the next event seed
  detMatch.fuStartIndex = uLocalIndexStart;
  detMatch.fuEndIndex   = uLocalIndexEnd;
  LOG(debug4) << Form("CbmAlgoBuildRawEvents :: SearchMatches(%d, %d)\n", uLocalIndexStart, uLocalIndexEnd);
}

void CbmAlgoBuildRawEvents::AddDigiToEvent(const RawEventBuilderDetector& det, Int_t _entry)
{
  LOG(debug4) << Form("CbmAlgoBuildRawEvents :: :: AddDigiToEvent(%s)\n", det.sName.data());
  fCurrentEvent->AddData(det.dataType, _entry);
}

void CbmAlgoBuildRawEvents::CheckTriggerCondition(Double_t dSeedTime)
{
  LOG(debug4) << Form("CbmAlgoBuildRawEvents :: :: CheckTriggerCondition(%f)\n", dSeedTime);
  /// Check if event is filling trigger conditions and clear it if not
  if (HasTrigger(fCurrentEvent)) {
    fdPrevEvtTime = dSeedTime;

    /// In case of NoOverlap or MergeOverlap, we can and should start checking the next window
    /// from end of current window in order to save CPU and avoid duplicating
    if (EOverlapModeRaw::NoOverlap == fOverMode || EOverlapModeRaw::MergeOverlap == fOverMode) {
      /// Update reference detector
      if (fRefDet.detId != ECbmModuleId::kNotExist) {
        fRefDet.fuStartIndex = fRefDet.fuEndIndex;
      }
      /// Loop on selection detectors
      for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
        (*det).fuStartIndex = (*det).fuEndIndex;
      }
    }
  }
  else {
    LOG(debug1) << "Reject seed due to Trigger requirements";
    delete fCurrentEvent;
    fCurrentEvent = nullptr;  /// delete does NOT set a pointer to nullptr...
  }
}

Bool_t CbmAlgoBuildRawEvents::HasTrigger(CbmEvent* event)
{
  /// Check first reference detector
  if (fRefDet.detId != ECbmModuleId::kNotExist) {
    if (kFALSE == CheckTriggerConditions(event, fRefDet)) {
      return kFALSE;
    }
  }
  /// Loop on selection detectors
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    if (kFALSE == CheckTriggerConditions(event, *det)) return kFALSE;
  }
  /// All Ok, trigger is there
  return kTRUE;
}

bool CbmAlgoBuildRawEvents::SetBmonEventTime(CbmEvent* event)
{
  const int32_t iNbDigis = event->GetNofData(ECbmDataType::kBmonDigi);
  double eventTime(0.);
  bool timeSet(false);

  LOG(debug4) << Form("CbmAlgoBuildRawEvents :: SetBmonEventTime(%p, %d)\n", (void*) event, iNbDigis);
  for (int idigi = 0; idigi < iNbDigis; ++idigi) {
    uint idx                 = event->GetIndex(ECbmDataType::kBmonDigi, idigi);
    const CbmBmonDigi* pDigi = GetDigi<CbmBmonDigi>(idx);
    if (nullptr == pDigi) continue;
    if (!filterBmon(pDigi->GetAddress())) continue;

    if (!timeSet) {
      eventTime = pDigi->GetTime();
      timeSet   = true;
    }
    else
      eventTime = std::min(pDigi->GetTime(), eventTime);
  }

  if (timeSet) {
    event->SetStartTime(eventTime);
  }
  else
    LOG(warning) << "CbmAlgoBuildRawEvents::SetBmonEventTime : failed";

  return timeSet;
}

Bool_t CbmAlgoBuildRawEvents::CheckTriggerConditions(CbmEvent* event, const RawEventBuilderDetector& det)
{
  LOG(debug4) << Form("CbmAlgoBuildRawEvents :: :: CheckTriggerConditions(%p, %s)\n", (void*) event, det.sName.data());
  /// Check if both Trigger conditions disabled for this detector
  if (0 == det.fuTriggerMinDigis && det.fiTriggerMaxDigis < 0) {
    return kTRUE;
  }

  /// Check if detector present
  if (!CheckDataAvailable(det.detId)) {
    LOG(debug2) << "Event does not have digis storage for " << det.sName
                << " while the following trigger min/max are defined: " << det.fuTriggerMinDigis << " "
                << det.fiTriggerMaxDigis;
    return kFALSE;
  }

  /// Check trigger rejection by minimal/maximal number or absence, if enabled/requested
  int32_t iNbDigis         = event->GetNofData(det.dataType);
  int32_t iNbFilteredDigis = (det.detId == ECbmModuleId::kBmon ? getNofFilteredBmonDigis(event) : iNbDigis);

  /// a.Check trigger rejection by minimal number (if enabled)
  if (0 < det.fuTriggerMinDigis
      && ((-1 == iNbFilteredDigis) || (static_cast<UInt_t>(iNbFilteredDigis) < det.fuTriggerMinDigis))) {
    LOG(debug2) << "Event does not have enough digis: " << iNbFilteredDigis << " vs " << det.fuTriggerMinDigis
                << " for " << det.sName;
    return kFALSE;
  }

  /// b.Check trigger rejection by maximal number (if enabled)
  if (0 <= det.fiTriggerMaxDigis && det.fiTriggerMaxDigis < iNbFilteredDigis) {
    LOG(debug2) << "Event Has too many digis: " << iNbFilteredDigis << " vs " << det.fiTriggerMaxDigis << " for "
                << det.sName;
    return kFALSE;
  }

  /// Check trigger rejection by minimal number of fired layers (if enabled)
  if (0 < det.fuTriggerMinLayers) {
    switch (det.detId) {
      case ECbmModuleId::kSts: {
        /// check for requested number of different stations
        /// loop over sts digis and check for
        std::set<uint32_t> setStations;  // Use set instead of vector as search by value later
        std::map<uint32_t, int> mModules;

        for (int idigi = 0; idigi < iNbDigis; ++idigi) {
          uint idx                = event->GetIndex(det.dataType, idigi);
          const CbmStsDigi* pDigi = GetDigi<CbmStsDigi>(idx);
          if (nullptr == pDigi) continue;

          int iAddr = pDigi->GetAddress();
          /// Module full address
          int iModuleAddr = CbmStsAddress::GetMotherAddress(iAddr, EStsElementLevel::kStsModule);
          /// Station index: station = unit in the mCBM addresses ?!?
          int iStationAddr = CbmStsAddress::GetElementId(iAddr, EStsElementLevel::kStsUnit);
          //int iStationAddr = CbmStsAddress::GetElementId(iAddr, EStsElementLevel::kStsUnit) / 2;

          std::map<uint32_t, int>::iterator itModule = mModules.find(iModuleAddr);
          if (itModule == mModules.end()) {
            // LOG(info) << Form("Found new module 0x%08x, side %u", iModuleAddr,
            //                   static_cast<uint32_t>(pDigi->GetChannel() / 1024));
            mModules[iModuleAddr] = static_cast<int32_t>(pDigi->GetChannel() / 1024);  // extend map
          }
          else {
            // LOG(info) << Form("Check side %u of module 0x%08x: %d ?",
            //                   static_cast<int32_t>(pDigi->GetChannel() / 1024),
            //                   iModuleAddr, itModule->second);
            if (static_cast<int32_t>(pDigi->GetChannel() / 1024) == (1 - itModule->second)) {
              /// Found other side => non-zero cluster chance, insert into stations set
              auto itStation = setStations.find(iStationAddr);
              if (itStation == setStations.end()) {
                // LOG(info) << Form("Add station 0x%08x ", iStationAddr);
                setStations.insert(iStationAddr);
              }
            }
          }
        }
        // LOG(info) << "Found " << setStations.size() << " Sts stations, " << " in " << iNbDigis << " Sts digis";
        if (setStations.size() < det.fuTriggerMinLayers) {
          LOG(debug2) << "Event does not have enough layers fired: " << setStations.size() << " vs "
                      << det.fuTriggerMinLayers << " for " << det.sName;
          return kFALSE;
        }
        break;
      }
      case ECbmModuleId::kMuch: {
        LOG(fatal) << "CbmAlgoBuildRawEvents::CheckTriggerConditions => Fired layers check not implemented yet for "
                   << det.sName;
        return kFALSE;
        break;
      }
      case ECbmModuleId::kTrd2d:  // Same data storage as trd 1d
      case ECbmModuleId::kTrd: {
        LOG(fatal) << "CbmAlgoBuildRawEvents::CheckTriggerConditions => Fired layers check not implemented yet for "
                   << det.sName;
        return kFALSE;
        break;
      }
      case ECbmModuleId::kTof: {
        /// check for requested number of different counters
        /// loop over tof digis and count different RPCs
        std::set<uint32_t> setRpcs;  // Use set instead of vector as search by value later
        std::map<uint32_t, int> mStrips;

        for (int idigi = 0; idigi < iNbDigis; ++idigi) {
          uint idx                = event->GetIndex(det.dataType, idigi);
          const CbmTofDigi* pDigi = GetDigi<CbmTofDigi>(idx);
          if (nullptr == pDigi) continue;

          int iAddr      = pDigi->GetAddress();
          int iStripAddr = CbmTofAddress::GetStripFullId(iAddr);
          int iRpcAddr   = CbmTofAddress::GetRpcFullId(iAddr);

          std::map<uint32_t, int>::iterator itStrip = mStrips.find(iStripAddr);
          if (itStrip == mStrips.end()) {
            // LOG(info) << Form("Found new strip 0x%08x, side %u", iStripAddr, pDigi->GetSide());
            mStrips[iStripAddr] = (int) pDigi->GetSide();  // extend map
          }
          else {
            // LOG(info) << Form("Check side %u of  strip 0x%08x: %d ?", pDigi->GetSide(), iStripAddr, itStrip->second);
            if ((int) pDigi->GetSide() == (1 - itStrip->second)) {
              /// Found other end => full strip, insert into counter set
              auto itRpc = setRpcs.find(iRpcAddr);
              if (itRpc == setRpcs.end()) {
                // LOG(info) << Form("Add counter 0x%08x ", iRpcAddr);
                setRpcs.insert(iRpcAddr);
              }
            }
          }
        }
        // LOG(info) << "Found " << setRpcs.size() << " Tof RPCs, " << " in " << iNbDigis << " Tof digis";
        if (setRpcs.size() < det.fuTriggerMinLayers) {
          LOG(debug2) << "Event does not have enough RPCs fired: " << setRpcs.size() << " vs " << det.fuTriggerMinLayers
                      << " for " << det.sName;
          return kFALSE;
        }
        break;
      }
      case ECbmModuleId::kRich: {
        LOG(fatal) << "CbmAlgoBuildRawEvents::CheckTriggerConditions => Fired layers check not implemented yet for "
                   << det.sName;
        return kFALSE;
        break;
      }
      case ECbmModuleId::kPsd: {
        LOG(fatal) << "CbmAlgoBuildRawEvents::CheckTriggerConditions => Fired layers check not implemented yet for "
                   << det.sName;
        return kFALSE;
        break;
      }
      case ECbmModuleId::kFsd: {
        LOG(fatal) << "CbmAlgoBuildRawEvents::CheckTriggerConditions => Fired layers check not implemented yet for "
                   << det.sName;
        return kFALSE;
        break;
      }
      case ECbmModuleId::kBmon: {
        LOG(fatal) << "CbmAlgoBuildRawEvents::CheckTriggerConditions => Fired layers check not implemented yet for "
                   << det.sName;
        return kFALSE;
        break;
      }
      default: {
        LOG(fatal) << "CbmAlgoBuildRawEvents::CheckTriggerConditions => Fired layers check not implemented yet for "
                   << det.sName;
        return kFALSE;
        break;
      }
    }
  }

  /// All checks passed, event is good
  return kTRUE;
}

//----------------------------------------------------------------------

Bool_t CbmAlgoBuildRawEvents::CheckDataAvailable(const RawEventBuilderDetector& det)
{
  if (!CheckDataAvailable(det.detId)) {
    LOG(info) << "No " << det.sName << " digi input found.";
    return kFALSE;
  }
  return kTRUE;
}

bool CbmAlgoBuildRawEvents::CheckDataAvailable(ECbmModuleId detId)
{
  switch (detId) {
    case ECbmModuleId::kSts: {
      return fStsDigis != nullptr;
    }
    case ECbmModuleId::kMuch: {
      if (fbUseMuchBeamtimeDigi) {
        return fMuchBeamTimeDigis != nullptr;
      }
      else {
        return fMuchDigis != nullptr;
      }
    }
    case ECbmModuleId::kTrd2d:  // Same data storage as trd 1d
    case ECbmModuleId::kTrd: {
      return fTrdDigis != nullptr;
    }
    case ECbmModuleId::kTof: {
      return fTofDigis != nullptr;
    }
    case ECbmModuleId::kRich: {
      return fRichDigis != nullptr;
    }
    case ECbmModuleId::kPsd: {
      return fPsdDigis != nullptr;
    }
    case ECbmModuleId::kFsd: {
      return fFsdDigis != nullptr;
    }
    case ECbmModuleId::kBmon: {
      return fBmonDigis != nullptr;
    }
    default: {
      LOG(fatal) << "CbmAlgoBuildRawEvents::CheckDataAvailable => "
                 << "Unsupported detector.";
      return -1;
    }
  }
}

UInt_t CbmAlgoBuildRawEvents::GetNofDigis(ECbmModuleId detId)
{
  switch (detId) {
    case ECbmModuleId::kSts: {
      return fStsDigis->size();
    }
    case ECbmModuleId::kMuch: {
      if (fbUseMuchBeamtimeDigi) {
        return fMuchBeamTimeDigis->size();
      }
      else {
        return fMuchDigis->size();
      }
    }
    case ECbmModuleId::kTrd2d:  // Same data storage as trd 1d
    case ECbmModuleId::kTrd: {
      return fTrdDigis->size();
    }
    case ECbmModuleId::kTof: {
      return fTofDigis->size();
    }
    case ECbmModuleId::kRich: {
      return fRichDigis->size();
    }
    case ECbmModuleId::kPsd: {
      return fPsdDigis->size();
    }
    case ECbmModuleId::kFsd: {
      return fFsdDigis->size();
    }
    case ECbmModuleId::kBmon: {
      return fBmonDigis->size();
    }
    default: {
      LOG(fatal) << "CbmAlgoBuildRawEvents::GetNofDigis => "
                 << "Trying to get digi number with unsupported detector.";
      return -1;
    }
  }
}
uint64_t CbmAlgoBuildRawEvents::GetSizeFromDigisNb(ECbmModuleId detId, uint64_t ulNbDigis)
{
  switch (detId) {
    case ECbmModuleId::kSts: {
      return ulNbDigis * sizeof(CbmStsDigi);
    }
    case ECbmModuleId::kMuch: {
      if (fbUseMuchBeamtimeDigi) {
        return ulNbDigis * sizeof(CbmMuchBeamTimeDigi);
      }
      else {
        return ulNbDigis * sizeof(CbmMuchDigi);
      }
    }
    case ECbmModuleId::kTrd2d:  // Same data storage as trd 1d
    case ECbmModuleId::kTrd: {
      return ulNbDigis * sizeof(CbmTrdDigi);
    }
    case ECbmModuleId::kTof: {
      return ulNbDigis * sizeof(CbmTofDigi);
    }
    case ECbmModuleId::kRich: {
      return ulNbDigis * sizeof(CbmRichDigi);
    }
    case ECbmModuleId::kPsd: {
      return ulNbDigis * sizeof(CbmPsdDigi);
    }
    case ECbmModuleId::kFsd: {
      return ulNbDigis * sizeof(CbmFsdDigi);
    }
    case ECbmModuleId::kBmon: {
      return ulNbDigis * sizeof(CbmBmonDigi);
    }
    default: {
      LOG(fatal) << "CbmAlgoBuildRawEvents::GetSizeFromDigisNb => "
                 << "Trying to get digi number with unsupported detector.";
      return -1;
    }
  }
}

//----------------------------------------------------------------------
void CbmAlgoBuildRawEvents::CreateHistograms()
{
  outFolder = new TDirectoryFile("AlgoBuildRawEventsHist", " AlgoBuildRawEvents Histos");
  outFolder->Clear();

  fhEventTime = new TH1F("hEventTime", "seed time of the events; Seed time within TS [s]; Events", 100000, 0, 0.2);
  // fhEventTime->SetCanExtend(TH1::kAllAxes);  // Breaks the MQ histogram server as cannot be merged!

  fhEventDt =
    new TH1F("fhEventDt", "interval in seed time of consecutive events; Seed time dt [ns]; Events", 10000, 0, 100000);
  // fhEventDt->SetCanExtend(TH1::kAllAxes);  // Breaks the MQ histogram server as cannot be merged!

  double_t dHistMaxTotDigis = fRefDet.fdHistMaxDigiNb;
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    dHistMaxTotDigis += (*det).fdHistMaxDigiNb;
  }
  fhEventSize = new TH1F("hEventSize", "nb of all  digis in the event; Nb Digis []; Events []", dHistMaxTotDigis, 0,
                         dHistMaxTotDigis);
  // fhEventSize->SetCanExtend(TH1::kAllAxes);  // Breaks the MQ histogram server as cannot be merged!

  fhNbDigiPerEvtTime = new TH2I("hNbDigiPerEvtTime",
                                "nb of all  digis per event vs seed time of the events; Seed time "
                                "[s]; Nb Digis []; Events []",
                                1000, 0, 0.2, dHistMaxTotDigis, 0, dHistMaxTotDigis);
  // fhNbDigiPerEvtTime->SetCanExtend(TH2::kAllAxes);  // Breaks he MQ histogram server as cannot be merged!

  fhCpuTimePerTs  = new TH1D("hCpuTimePerTs", "CPU Processing time of TS vs TS; Ts; CPU time [ms]", 6000, 0, 6000);
  fhRealTimePerTs = new TH1D("hRealTimePerTs", "Real Processing time of TS vs TS; Ts; Real time [ms]", 6000, 0, 6000);

  fhCpuTimePerTsHist =
    new TH1D("hCpuTimePerTsHist", "CPU Histo filling time of TS vs TS; Ts; CPU time [ms]", 6000, 0, 6000);
  fhRealTimePerTsHist =
    new TH1D("hRealTimePerTsHist", "Real Histo filling time of TS vs TS; Ts; Real time [ms]", 6000, 0, 6000);

  AddHistoToVector(fhEventTime, "evtbuild");
  AddHistoToVector(fhEventDt, "evtbuild");
  AddHistoToVector(fhEventSize, "evtbuild");
  AddHistoToVector(fhNbDigiPerEvtTime, "evtbuild");
  AddHistoToVector(fhCpuTimePerTs, "evtbuild-eff");
  AddHistoToVector(fhRealTimePerTs, "evtbuild-eff");
  AddHistoToVector(fhCpuTimePerTsHist, "evtbuild-eff");
  AddHistoToVector(fhRealTimePerTsHist, "evtbuild-eff");
  outFolder->Add(fhEventTime);
  outFolder->Add(fhEventDt);
  outFolder->Add(fhEventSize);
  outFolder->Add(fhNbDigiPerEvtTime);
  outFolder->Add(fhCpuTimePerTs);
  outFolder->Add(fhRealTimePerTs);
  outFolder->Add(fhCpuTimePerTsHist);
  outFolder->Add(fhRealTimePerTsHist);

  if (EOverlapModeRaw::AllowOverlap == fOverMode) {
    fhOverEventShare   = new TH1I("fhOverEventShare", "Share of overlap evt; Overlap? []; Events", 2, -0.5, 1.5);
    fhOverEventShareTs = new TProfile(
      "fhOverEventShareTs", "Share of overlap evt per TS; TS index []; Overlap Events prop. []", 2500, 0, 2500);
    fhOverEventSizeTs =
      new TH2I("fhOverEventSizeTs", "Size of overlap of evt per TS; TS index []; Size of overlap between events [ns]",
               2500, 0, 2500, 200, 0, 1000);
    AddHistoToVector(fhOverEventShare, "evtbuild");
    AddHistoToVector(fhOverEventShareTs, "evtbuild");
    AddHistoToVector(fhOverEventSizeTs, "evtbuild");
    outFolder->Add(fhOverEventShare);
    outFolder->Add(fhOverEventShareTs);
    outFolder->Add(fhOverEventSizeTs);
  }

  /// Loop on selection detectors
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    /// In case name not provided, do not create the histo to avoid name conflicts!
    if ("Invalid" == (*det).sName) {
      fvhNbDigiPerEvtTimeDet.push_back(nullptr);
      fvhNbDigiPerEvtDet.push_back(nullptr);
      continue;
    }
    TH2I* hNbDigiPerEvtTimeDet = new TH2I(Form("hNbDigiPerEvtTime%s", (*det).sName.data()),
                                          Form("nb of %s digis per event vs seed time of the events; Seed time in TS "
                                               "[s]; Nb Digis []; Events []",
                                               (*det).sName.data()),
                                          1000, 0, 0.2, (*det).fdHistMaxDigiNb, 0, (*det).fdHistMaxDigiNb);
    // hNbDigiPerEvtTimeDet->SetCanExtend(TH2::kAllAxes);   // Breaks he MQ histogram server as cannot be merged!
    fvhNbDigiPerEvtTimeDet.push_back(hNbDigiPerEvtTimeDet);

    TH1* hNbDigiPerEvtDet = new TH1I(Form("hNbDigiPerEvt%s", (*det).sName.data()),
                                     Form("nb of %s digis per event; Nb Digis []", (*det).sName.data()),
                                     (*det).fdHistMaxDigiNb, 0, (*det).fdHistMaxDigiNb);
    fvhNbDigiPerEvtDet.push_back(hNbDigiPerEvtDet);

    TH1* hTDiff =
      new TH1I(Form("hTDiff%s", (*det).sName.data()),
               Form("#DeltaT of %s digis to seed time of event;#DeltaT (ns); Counts []", (*det).sName.data()), 200,
               (*det).fdTimeWinBeg, (*det).fdTimeWinEnd);
    fvhTDiff.push_back(hTDiff);

    // clang-format off
    TH1* hSelRatioPerTsNb = new TH1D(Form("hSelRatioPerTsNb%s", (*det).sName.data()),
                                     Form("ratio of sel digis per TS vs TS for %s; TS; Sel Digis Ratio []",
                                          (*det).sName.data()),
                                     6000, 0, 6000);
    TH1* hInpRatioPerTsSz = new TH1D(Form("hInpRatioPerTsSz%s", (*det).sName.data()),
                                     Form("ratio of input digi size in total input size vs TS for %s; TS; Size Ratio []",
                                          (*det).sName.data()),
                                     6000, 0, 6000);
    TH1* hOutRatioPerTsSz = new TH1D(Form("hOutRatioPerTsSz%s", (*det).sName.data()),
                                     Form("ratio of selected digi size in event size vs TS for %s; TS; Size Ratio []",
                                          (*det).sName.data()),
                                     6000, 0, 6000);
    // clang-format on

    fvhSelRatioPerTsNb.push_back(hSelRatioPerTsNb);
    fvhInpRatioPerTsSz.push_back(hInpRatioPerTsSz);
    fvhOutRatioPerTsSz.push_back(hOutRatioPerTsSz);

    AddHistoToVector(hSelRatioPerTsNb, "evtbuild-eff");
    AddHistoToVector(hInpRatioPerTsSz, "evtbuild-eff");
    AddHistoToVector(hOutRatioPerTsSz, "evtbuild-eff");

    outFolder->Add(hSelRatioPerTsNb);
    outFolder->Add(hInpRatioPerTsSz);
    outFolder->Add(hOutRatioPerTsSz);
  }

  /// Same plots for the reference detector
  if (ECbmModuleId::kNotExist != fRefDet.detId) {
    TH2I* hNbDigiPerEvtTimeDet = new TH2I(Form("hNbDigiPerEvtTime%s", fRefDet.sName.data()),
                                          Form("nb of %s digis per event vs seed time of the events; Seed time in TS "
                                               "[s]; Nb Digis []; Events []",
                                               fRefDet.sName.data()),
                                          1000, 0, 0.2, fRefDet.fdHistMaxDigiNb, 0, fRefDet.fdHistMaxDigiNb);
    fvhNbDigiPerEvtTimeDet.push_back(hNbDigiPerEvtTimeDet);

    TH1I* hNbDigiPerEvtDet = new TH1I(Form("hNbDigiPerEvt%s", fRefDet.sName.data()),
                                      Form("nb of %s digis per event; Nb Digis []", fRefDet.sName.data()),
                                      fRefDet.fdHistMaxDigiNb, 0, fRefDet.fdHistMaxDigiNb);
    fvhNbDigiPerEvtDet.push_back(hNbDigiPerEvtDet);

    TH1I* hTDiff =
      new TH1I(Form("hTDiff%s", fRefDet.sName.data()),
               Form("#DeltaT of %s digis to seed time of event;#DeltaT (ns); Counts []", fRefDet.sName.data()), 200,
               fRefDet.fdTimeWinBeg, fRefDet.fdTimeWinEnd);  // FIXME, adjust to configured window
    fvhTDiff.push_back(hTDiff);

    // clang-format off
    TH1* hSelRatioPerTsNb = new TH1D(Form("hSelRatioPerTsNb%s", fRefDet.sName.data()),
                                     Form("ratio of sel digis per TS vs TS for %s; TS; Sel Digis Ratio []",
                                          fRefDet.sName.data()),
                                     6000, 0, 6000);
    TH1* hInpRatioPerTsSz = new TH1D(Form("hInpRatioPerTsSz%s", fRefDet.sName.data()),
                                     Form("ratio of input digi size in total input size vs TS for %s; TS; Size Ratio []",
                                          fRefDet.sName.data()),
                                     6000, 0, 6000);
    TH1* hOutRatioPerTsSz = new TH1D(Form("hOutRatioPerTsSz%s", fRefDet.sName.data()),
                                     Form("ratio of selected digi size in event size vs TS for %s; TS; Size Ratio []",
                                          fRefDet.sName.data()),
                                     6000, 0, 6000);
    // clang-format on

    fvhSelRatioPerTsNb.push_back(hSelRatioPerTsNb);
    fvhInpRatioPerTsSz.push_back(hInpRatioPerTsSz);
    fvhOutRatioPerTsSz.push_back(hOutRatioPerTsSz);

    AddHistoToVector(hSelRatioPerTsNb, "evtbuild-eff");
    AddHistoToVector(hInpRatioPerTsSz, "evtbuild-eff");
    AddHistoToVector(hOutRatioPerTsSz, "evtbuild-eff");

    outFolder->Add(hSelRatioPerTsNb);
    outFolder->Add(hInpRatioPerTsSz);
    outFolder->Add(hOutRatioPerTsSz);
  }

  fhSizeReductionPerTs =
    new TH1D("hSizeReductionPerTs", "ratio of tot. sel. digi size to tot. input digi size vs TS; TS; Size Ratio []",
             6000, 0, 6000);
  AddHistoToVector(fhSizeReductionPerTs, "evtbuild-eff");
  outFolder->Add(fhSizeReductionPerTs);

  for (std::vector<TH2*>::iterator itHist = fvhNbDigiPerEvtTimeDet.begin(); itHist != fvhNbDigiPerEvtTimeDet.end();
       ++itHist) {
    if (nullptr != (*itHist)) {
      AddHistoToVector((*itHist), "evtbuild");
      outFolder->Add((*itHist));
    }
  }

  for (std::vector<TH1*>::iterator itHist = fvhNbDigiPerEvtDet.begin(); itHist != fvhNbDigiPerEvtDet.end(); ++itHist) {
    if (nullptr != (*itHist)) {
      AddHistoToVector((*itHist), "evtbuild");
      outFolder->Add((*itHist));
    }
  }
  for (std::vector<TH1*>::iterator itHist = fvhTDiff.begin(); itHist != fvhTDiff.end(); ++itHist) {
    if (nullptr != (*itHist)) {
      AddHistoToVector((*itHist), "evtbuild");
      outFolder->Add((*itHist));
    }
  }

  /// Canvases creation
  // std::vector<std::pair<TCanvas*, std::string>> vCanvases = {};

  TCanvas* fcSummary = new TCanvas("cEvBSummary", "EvB monitoring plots");
  fcSummary->Divide(2, 2);

  fcSummary->cd(1);
  gPad->SetGridx();
  gPad->SetGridy();
  fhEventTime->Draw("hist");

  fcSummary->cd(2);
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogx();
  gPad->SetLogy();
  fhEventDt->Draw("hist");

  fcSummary->cd(3);
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogy();
  fhEventSize->Draw("hist");

  fcSummary->cd(4);
  gPad->SetGridx();
  gPad->SetGridy();
  fhNbDigiPerEvtTime->Draw("colz");


  /// Add canvas pointers to the canvas vector
  AddCanvasToVector(fcSummary, "canvases");

  // ------------------------ //
  TCanvas* fcNbDigi = new TCanvas("cEvBNbDigi", "EvB NbDigi evolution ");
  if (fvhNbDigiPerEvtDet.size() <= 6) {  //
    fcNbDigi->Divide(2, 3);
  }
  else {  //
    fcNbDigi->Divide(3, 3);
  }
  int iPad = 1;
  for (std::vector<TH1*>::iterator itHist = fvhNbDigiPerEvtDet.begin(); itHist != fvhNbDigiPerEvtDet.end(); ++itHist) {
    if (nullptr != (*itHist)) {
      fcNbDigi->cd(iPad++);
      gPad->SetGridx();
      gPad->SetGridy();
      gPad->SetLogy();
      (*itHist)->Draw();  //"colz");
    }
  }
  AddCanvasToVector(fcNbDigi, "canvases");

  // ------------------------ //
  TCanvas* fcTdif = new TCanvas("cEvBTdif", "EvB Time Difference plots");
  if (fvhNbDigiPerEvtDet.size() <= 6) {  //
    fcTdif->Divide(2, 3);
  }
  else {  //
    fcTdif->Divide(3, 3);
  }
  iPad = 1;
  for (std::vector<TH1*>::iterator itHist = fvhTDiff.begin(); itHist != fvhTDiff.end(); ++itHist) {
    if (nullptr != (*itHist)) {
      fcTdif->cd(iPad++);
      gPad->SetGridx();
      gPad->SetGridy();
      gPad->SetLogy();
      (*itHist)->Draw();
    }
  }
  AddCanvasToVector(fcTdif, "canvases");
}

void CbmAlgoBuildRawEvents::FillHistos()
{
  /// I/O monitoring
  uint32_t uRefDetIdx        = fvDets.size();
  uint64_t ulTotalInputSize  = 0;
  uint64_t ulTotalOutputSize = 0;
  std::vector<uint64_t> vulTotalInputSizeDet(fvDets.size() + 1, 0);
  std::vector<uint64_t> vulTotalOutputSizeDet(fvDets.size() + 1, 0);

  /// Output monitoring
  Double_t dPreEvtTime = -1.0;
  for (CbmEvent* evt : fEventVector) {
    fhEventTime->Fill(evt->GetStartTime() * 1e-9);
    if (0.0 <= dPreEvtTime) {
      fhEventDt->Fill((evt->GetStartTime() - dPreEvtTime) * 1e-9);

      if (EOverlapModeRaw::AllowOverlap == fOverMode) {
        if (evt->GetStartTime() - dPreEvtTime < fdWidestTimeWinRange) {
          fhOverEventShare->Fill(1);
          fhOverEventShareTs->Fill(fuNrTs, 1);
          fhOverEventSizeTs->Fill(fuNrTs, fdWidestTimeWinRange - (evt->GetStartTime() - dPreEvtTime));
        }
        else {
          fhOverEventShare->Fill(0);
          fhOverEventShareTs->Fill(fuNrTs, 0);
        }
      }
    }
    else if (EOverlapModeRaw::AllowOverlap == fOverMode) {
      /// First event cannot be in overlap
      fhOverEventShare->Fill(0);
      fhOverEventShareTs->Fill(fuNrTs, 0);
    }

    fhEventSize->Fill(evt->GetNofData());
    fhNbDigiPerEvtTime->Fill(evt->GetStartTime() * 1e-9, evt->GetNofData());

    /// Loop on selection detectors
    uint32_t uNbDataTrd1d = 0;
    uint32_t uNbDataTrd2d = 0;
    for (UInt_t uDetIdx = 0; uDetIdx < fvDets.size(); ++uDetIdx) {
      if (nullptr == fvhNbDigiPerEvtDet[uDetIdx]) continue;

      for (size_t idigi = 0; idigi < evt->GetNofData(fvDets[uDetIdx].dataType); ++idigi) {
        double dTimeDiff = 1.E30;
        uint idx         = evt->GetIndex(fvDets[uDetIdx].dataType, idigi);
        switch (fvDets[uDetIdx].dataType) {
          case ECbmDataType::kBmonDigi: {
            auto pDigi = GetDigi<CbmBmonDigi>(idx);
            if (nullptr == pDigi) continue;
            dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
            break;
          }
          case ECbmDataType::kStsDigi: {
            auto pDigi = GetDigi<CbmStsDigi>(idx);
            if (nullptr == pDigi) continue;
            dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
            break;
          }
          case ECbmDataType::kMuchDigi: {
            if (fbUseMuchBeamtimeDigi) {
              auto pDigi = GetDigi<CbmMuchBeamTimeDigi>(idx);
              if (nullptr == pDigi) continue;
              dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
            }
            else {
              auto pDigi = GetDigi<CbmMuchDigi>(idx);
              if (nullptr == pDigi) continue;
              dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
            }
            break;
          }
          case ECbmDataType::kTofDigi: {
            auto pDigi = GetDigi<CbmTofDigi>(idx);
            if (nullptr == pDigi) continue;
            dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
            break;
          }
          case ECbmDataType::kTrdDigi: {
            auto pDigi = GetDigi<CbmTrdDigi>(idx);
            if (nullptr == pDigi) continue;
            dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
            if (pDigi->GetType() == CbmTrdDigi::eCbmTrdAsicType::kSPADIC) {
              if (fvDets[uDetIdx].sName == "Trd2D") continue;
              ++uNbDataTrd1d;
            }
            else if (pDigi->GetType() == CbmTrdDigi::eCbmTrdAsicType::kFASP) {
              if (fvDets[uDetIdx].sName == "Trd1D") continue;
              ++uNbDataTrd2d;
            }
            break;
          }
          case ECbmDataType::kRichDigi: {
            auto pDigi = GetDigi<CbmRichDigi>(idx);  // FIXME, need to find the proper digi template
            if (nullptr == pDigi) continue;
            dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
            break;
          }
          case ECbmDataType::kPsdDigi: {
            auto pDigi = GetDigi<CbmPsdDigi>(idx);  // FIXME, need to find the proper digi template
            if (nullptr == pDigi) continue;
            dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
            break;
          }
          case ECbmDataType::kFsdDigi: {
            auto pDigi = GetDigi<CbmFsdDigi>(idx);  // FIXME, need to find the proper digi template
            if (nullptr == pDigi) continue;
            dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
            break;
          }
          default: LOG(error) << "Unkown dataType " << fvDets[uDetIdx].dataType;
        }

        if (dTimeDiff < 1.E30) fvhTDiff[uDetIdx]->Fill(dTimeDiff);
      }
    }

    /// Reference detector
    if (ECbmModuleId::kNotExist != fRefDet.detId) {
      if (nullptr != fvhNbDigiPerEvtDet[uRefDetIdx]) {
        for (size_t idigi = 0; idigi < evt->GetNofData(fRefDet.dataType); ++idigi) {
          double dTimeDiff = 1.E30;
          uint idx         = evt->GetIndex(fRefDet.dataType, idigi);
          switch (fRefDet.dataType) {
            case ECbmDataType::kBmonDigi: {
              auto pDigi = GetDigi<CbmBmonDigi>(idx);
              if (nullptr == pDigi) continue;
              dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
              break;
            }
            case ECbmDataType::kStsDigi: {
              auto pDigi = GetDigi<CbmStsDigi>(idx);
              if (nullptr == pDigi) continue;
              dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
              break;
            }
            case ECbmDataType::kMuchDigi: {
              if (fbUseMuchBeamtimeDigi) {
                auto pDigi = GetDigi<CbmMuchBeamTimeDigi>(idx);
                if (nullptr == pDigi) continue;
                dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
              }
              else {
                auto pDigi = GetDigi<CbmMuchDigi>(idx);
                if (nullptr == pDigi) continue;
                dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
              }
              break;
            }
            case ECbmDataType::kTofDigi: {
              auto pDigi = GetDigi<CbmTofDigi>(idx);
              if (nullptr == pDigi) continue;
              dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
              break;
            }
            case ECbmDataType::kTrdDigi: {
              auto pDigi = GetDigi<CbmTrdDigi>(idx);
              if (nullptr == pDigi) continue;
              dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
              if (pDigi->GetType() == CbmTrdDigi::eCbmTrdAsicType::kSPADIC) {
                if (fRefDet.sName == "Trd2D") continue;
                ++uNbDataTrd1d;
              }
              else if (pDigi->GetType() == CbmTrdDigi::eCbmTrdAsicType::kFASP) {
                if (fRefDet.sName == "Trd1D") continue;
                ++uNbDataTrd2d;
              }
              break;
            }
            case ECbmDataType::kRichDigi: {
              auto pDigi = GetDigi<CbmRichDigi>(idx);  // FIXME, need to find the proper digi template
              if (nullptr == pDigi) continue;
              dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
              break;
            }
            case ECbmDataType::kPsdDigi: {
              auto pDigi = GetDigi<CbmPsdDigi>(idx);  // FIXME, need to find the proper digi template
              if (nullptr == pDigi) continue;
              dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
              break;
            }
            case ECbmDataType::kFsdDigi: {
              auto pDigi = GetDigi<CbmFsdDigi>(idx);  // FIXME, need to find the proper digi template
              if (nullptr == pDigi) continue;
              dTimeDiff = pDigi->GetTime() - evt->GetStartTime();
              break;
            }
            default: LOG(error) << "Unkown dataType " << fRefDet.dataType;
          }

          if (dTimeDiff < 1.E30) fvhTDiff[uRefDetIdx]->Fill(dTimeDiff);
        }
      }
    }

    /// Re-Loop on selection detectors due to detectors with same data type
    for (UInt_t uDetIdx = 0; uDetIdx < fvDets.size(); ++uDetIdx) {
      if (nullptr == fvhNbDigiPerEvtTimeDet[uDetIdx]) continue;

      if (fvDets[uDetIdx].sName == "Trd1D") {
        fvhNbDigiPerEvtDet[uDetIdx]->Fill(uNbDataTrd1d);
        fvhNbDigiPerEvtTimeDet[uDetIdx]->Fill(evt->GetStartTime() * 1e-9, uNbDataTrd1d);

        if (0 < GetNofDigis(fvDets[uDetIdx].detId)) {
          /// Selection ratio
          uint64_t ulDigiSizeOut = GetSizeFromDigisNb(fvDets[uDetIdx].detId, uNbDataTrd1d + uNbDataTrd2d);

          ulTotalOutputSize += ulDigiSizeOut;
          vulTotalOutputSizeDet[uDetIdx] += ulDigiSizeOut;
        }
      }
      else if (fvDets[uDetIdx].sName == "Trd2D") {
        fvhNbDigiPerEvtDet[uDetIdx]->Fill(uNbDataTrd2d);
        fvhNbDigiPerEvtTimeDet[uDetIdx]->Fill(evt->GetStartTime() * 1e-9, uNbDataTrd2d);
      }
      else {
        fvhNbDigiPerEvtDet[uDetIdx]->Fill(evt->GetNofData(fvDets[uDetIdx].dataType));
        fvhNbDigiPerEvtTimeDet[uDetIdx]->Fill(evt->GetStartTime() * 1e-9, evt->GetNofData(fvDets[uDetIdx].dataType));

        if (0 < GetNofDigis(fvDets[uDetIdx].detId)) {
          /// Selection ratio
          uint64_t ulDigiSizeOut = GetSizeFromDigisNb(fvDets[uDetIdx].detId, evt->GetNofData(fvDets[uDetIdx].dataType));

          ulTotalOutputSize += ulDigiSizeOut;
          vulTotalOutputSizeDet[uDetIdx] += ulDigiSizeOut;
        }
      }
    }

    /// Same for the reference detector
    if (ECbmModuleId::kNotExist != fRefDet.detId) {
      if (fRefDet.sName == "Trd1D") {
        fvhNbDigiPerEvtDet[uRefDetIdx]->Fill(uNbDataTrd1d);
        fvhNbDigiPerEvtTimeDet[uRefDetIdx]->Fill(evt->GetStartTime() * 1e-9, uNbDataTrd1d);

        if (0 < GetNofDigis(fRefDet.detId)) {
          /// Selection ratio
          uint64_t ulDigiSizeOut = GetSizeFromDigisNb(fRefDet.detId, uNbDataTrd1d + uNbDataTrd2d);

          ulTotalOutputSize += ulDigiSizeOut;
          vulTotalOutputSizeDet[uRefDetIdx] += ulDigiSizeOut;
        }
      }
      else if (fRefDet.sName == "Trd2D") {
        fvhNbDigiPerEvtDet[uRefDetIdx]->Fill(uNbDataTrd2d);
        fvhNbDigiPerEvtTimeDet[uRefDetIdx]->Fill(evt->GetStartTime() * 1e-9, uNbDataTrd2d);
      }
      else {
        fvhNbDigiPerEvtDet[uRefDetIdx]->Fill(evt->GetNofData(fRefDet.dataType));
        fvhNbDigiPerEvtTimeDet[uRefDetIdx]->Fill(evt->GetStartTime() * 1e-9, evt->GetNofData(fRefDet.dataType));

        if (0 < GetNofDigis(fRefDet.detId)) {
          /// Selection ratio
          uint64_t ulDigiSizeOut = GetSizeFromDigisNb(fRefDet.detId, evt->GetNofData(fRefDet.dataType));

          ulTotalOutputSize += ulDigiSizeOut;
          vulTotalOutputSizeDet[uRefDetIdx] += ulDigiSizeOut;
        }
      }
    }

    dPreEvtTime = evt->GetStartTime();
  }

  /// Loop on selection detectors to count input data
  for (UInt_t uDetIdx = 0; uDetIdx < fvDets.size(); ++uDetIdx) {
    if (fvDets[uDetIdx].sName == "Trd2D") {
      /// Skip as cannot be distinguished from TR1D wihout looping on digis itselves.
      /// => Same is done for output share (combination in single number for 1D & 2D) in order to keep things consistent
      continue;
    }
    uint64_t ulDigiSizeIn = GetSizeFromDigisNb(fvDets[uDetIdx].detId, GetNofDigis(fvDets[uDetIdx].detId));
    ulTotalInputSize += ulDigiSizeIn;
    vulTotalInputSizeDet[uDetIdx] += ulDigiSizeIn;
  }
  if (ECbmModuleId::kNotExist != fRefDet.detId) {
    uint64_t ulDigiSizeIn = GetSizeFromDigisNb(fRefDet.detId, GetNofDigis(fRefDet.detId));
    ulTotalInputSize += ulDigiSizeIn;
    vulTotalInputSizeDet[uRefDetIdx] += ulDigiSizeIn;
  }

  /// Re-Loop on selection detectors to fill global TS ratios
  for (UInt_t uDetIdx = 0; uDetIdx < fvDets.size(); ++uDetIdx) {
    if (0 != vulTotalInputSizeDet[uDetIdx]) {  //
      fvhSelRatioPerTsNb[uDetIdx]->Fill(fuNrTs, vulTotalOutputSizeDet[uDetIdx] * 1.0 / vulTotalInputSizeDet[uDetIdx]);
    }
    if (0 != ulTotalInputSize) {  //
      fvhInpRatioPerTsSz[uDetIdx]->Fill(fuNrTs, vulTotalInputSizeDet[uDetIdx] * 1.0 / ulTotalInputSize);
    }
    if (0 != ulTotalOutputSize) {  //
      fvhOutRatioPerTsSz[uDetIdx]->Fill(fuNrTs, vulTotalOutputSizeDet[uDetIdx] * 1.0 / ulTotalOutputSize);
    }
  }
  /// Same for the reference detector
  if (ECbmModuleId::kNotExist != fRefDet.detId) {
    if (0 != vulTotalInputSizeDet[uRefDetIdx]) {  //
      fvhSelRatioPerTsNb[uRefDetIdx]->Fill(fuNrTs,
                                           vulTotalOutputSizeDet[uRefDetIdx] * 1.0 / vulTotalInputSizeDet[uRefDetIdx]);
    }
    if (0 != ulTotalInputSize) {  //
      fvhInpRatioPerTsSz[uRefDetIdx]->Fill(fuNrTs, vulTotalInputSizeDet[uRefDetIdx] * 1.0 / ulTotalInputSize);
    }
    if (0 != ulTotalOutputSize) {  //
      fvhOutRatioPerTsSz[uRefDetIdx]->Fill(fuNrTs, vulTotalOutputSizeDet[uRefDetIdx] * 1.0 / ulTotalOutputSize);
    }
  }
  /// Global value for all detectors
  if (0 != ulTotalInputSize) {  //
    fhSizeReductionPerTs->Fill(fuNrTs, ulTotalOutputSize * 1.0 / ulTotalInputSize);
  }
  LOG(debug) << "I/O Size ratio: " << (ulTotalOutputSize * 1.0 / ulTotalInputSize);
}

void CbmAlgoBuildRawEvents::ResetHistograms(Bool_t /*bResetTime*/)
{
  fhEventTime->Reset();
  fhEventDt->Reset();
  fhEventSize->Reset();

  fhNbDigiPerEvtTime->Reset();
  /// Loop on histograms
  for (std::vector<TH2*>::iterator itHist = fvhNbDigiPerEvtTimeDet.begin(); itHist != fvhNbDigiPerEvtTimeDet.end();
       ++itHist) {
    (*itHist)->Reset();
  }

  for (std::vector<TH1*>::iterator itHist = fvhNbDigiPerEvtDet.begin(); itHist != fvhNbDigiPerEvtDet.end(); ++itHist) {
    (*itHist)->Reset();
  }

  for (std::vector<TH1*>::iterator itHist = fvhTDiff.begin(); itHist != fvhTDiff.end(); ++itHist) {
    (*itHist)->Reset();
  }

  /*
   if( kTRUE == bResetTime )
   {
      /// Also reset the Start time for the evolution plots!
      fdStartTime = -1.0;
   }
   */
}

void CbmAlgoBuildRawEvents::SetReferenceDetector(ECbmModuleId refDet, ECbmDataType dataTypeIn, std::string sNameIn,
                                                 UInt_t uTriggerMinDigisIn, Int_t iTriggerMaxDigisIn,
                                                 Double_t fdTimeWinBegIn, Double_t fdTimeWinEndIn)
{
  /// FIXME: Deprecated method to be removed later. For now create temp object.
  SetReferenceDetector(RawEventBuilderDetector(refDet, dataTypeIn, sNameIn, uTriggerMinDigisIn, iTriggerMaxDigisIn,
                                               fdTimeWinBegIn, fdTimeWinEndIn));
}
void CbmAlgoBuildRawEvents::AddDetector(ECbmModuleId selDet, ECbmDataType dataTypeIn, std::string sNameIn,
                                        UInt_t uTriggerMinDigisIn, Int_t iTriggerMaxDigisIn, Double_t fdTimeWinBegIn,
                                        Double_t fdTimeWinEndIn)
{
  /// FIXME: Deprecated method to be removed later. For now create temp object.
  AddDetector(RawEventBuilderDetector(selDet, dataTypeIn, sNameIn, uTriggerMinDigisIn, iTriggerMaxDigisIn,
                                      fdTimeWinBegIn, fdTimeWinEndIn));
}

void CbmAlgoBuildRawEvents::SetReferenceDetector(RawEventBuilderDetector refDetIn, std::vector<bool> select)
{
  /// Loop on selection detectors
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    if ((*det) == refDetIn) {
      LOG(warning) << "CbmAlgoBuildRawEvents::SetReferenceDetector => "
                      "Reference detector already in selection detector list! "
                   << refDetIn.sName;
      LOG(warning) << "                                                         => "
                      "It will be automatically removed from selection detector list!";
      LOG(warning) << "                                                         => "
                      "Please also remember to update the selection windows to store "
                      "clusters!";
      RemoveDetector(refDetIn);
      break;
    }
  }

  if (fRefDet == refDetIn) {
    LOG(warning) << "CbmAlgoBuildRawEvents::SetReferenceDetector => "
                    "Doing nothing, identical reference detector already in use";
  }
  else {
    LOG(info) << "CbmAlgoBuildRawEvents::SetReferenceDetector => "
              << "Replacing " << fRefDet.sName << " with " << refDetIn.sName << " as reference detector";
    LOG(warning) << "                                                         => "
                    "You may want to use AddDetector after this command to add in "
                    "selection "
                 << fRefDet.sName;
    LOG(warning) << "                                                         => "
                    "Please also remember to update the selection windows!";
  }
  fRefDet = refDetIn;

  /// Update the variables storing the earliest and latest time window boundaries
  UpdateTimeWinBoundariesExtrema();
  /// Update the variable storing the size if widest time window for overlap detection
  UpdateWidestTimeWinRange();
  /// Detect usage of BMon to set Event StartTime
  CheckBmonInUse();

  if (fbBmonInUse) {
    if (select.size()) {
      LOG(info) << "CbmAlgoBuildRawEvents::SetReferenceDetector => Detected Bmon station selection.";
      fUseBmonMap.assign(select.size(), false);
      int it0(0);
      for (auto t0 : select)
        SwitchBmonStation(it0++, (t0 > 0));
    }
  }
  else {
    if (select.size())
      LOG(warning) << "CbmAlgoBuildRawEvents::SetReferenceDetector => Detected use of selector\nfor a reference "
                      "detector which does not support this option. Skip selection for the moment.";
  }
}

void CbmAlgoBuildRawEvents::AddDetector(RawEventBuilderDetector selDet)
{
  if (fRefDet == selDet) {
    LOG(fatal) << "CbmAlgoBuildRawEvents::AddDetector => Cannot "
                  "add the reference detector as selection detector!"
               << std::endl
               << "=> Maybe first change the reference detector with "
                  "SetReferenceDetector?";
  }

  /// Loop on selection detectors
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    if ((*det) == selDet) {
      LOG(warning) << "CbmAlgoBuildRawEvents::AddDetector => "
                      "Doing nothing, selection detector already in list! "
                   << selDet.sName;
      return;
    }
  }
  fvDets.push_back(selDet);

  /// Update the variables storing the earliest and latest time window boundaries
  UpdateTimeWinBoundariesExtrema();
  /// Update the variable storing the size if widest time window for overlap detection
  UpdateWidestTimeWinRange();
  /// Detect usage of BMon to set Event StartTime
  CheckBmonInUse();
}

void CbmAlgoBuildRawEvents::RemoveDetector(RawEventBuilderDetector selDet)
{
  /// Loop on selection detectors
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    if ((*det) == selDet) {
      fvDets.erase(det);
      /// Detect usage of BMon to set Event StartTime
      CheckBmonInUse();
      return;
    }
  }
  LOG(warning) << "CbmAlgoBuildRawEvents::RemoveDetector => Doing "
                  "nothing, selection detector not in list! "
               << selDet.sName;
}

void CbmAlgoBuildRawEvents::CheckBmonInUse()
{
  if (ECbmModuleId::kBmon == fRefDet.detId) {  //
    fbBmonInUse = true;
  }
  else {
    fbBmonInUse = std::any_of(fvDets.cbegin(), fvDets.cend(),
                              [](RawEventBuilderDetector det) { return ECbmModuleId::kBmon == det.detId; });
  }
}

void CbmAlgoBuildRawEvents::SetTriggerMinNumber(ECbmModuleId selDet, UInt_t uVal)
{
  /// Check first if reference detector
  if (fRefDet.detId == selDet) {
    fRefDet.fuTriggerMinDigis = uVal;
    LOG(debug) << "Set Trigger min limit for " << fRefDet.sName << " to " << uVal;
    return;
  }

  /// Loop on selection detectors
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    if ((*det).detId == selDet) {
      (*det).fuTriggerMinDigis = uVal;
      LOG(debug) << "Set Trigger min limit for " << (*det).sName << " to " << uVal;
      return;
    }
  }
  LOG(warning) << "CbmAlgoBuildRawEvents::SetTriggerMinNumber => "
                  "Doing nothing, detector neither reference nor in selection list! "
               << selDet;
}

void CbmAlgoBuildRawEvents::SetTriggerMaxNumber(ECbmModuleId selDet, Int_t iVal)
{
  /// Check first if reference detector
  if (fRefDet.detId == selDet) {
    fRefDet.fiTriggerMaxDigis = iVal;
    LOG(debug) << "Set Trigger min limit for " << fRefDet.sName << " to " << iVal;
    return;
  }

  /// Loop on selection detectors
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    if ((*det).detId == selDet) {
      (*det).fiTriggerMaxDigis = iVal;
      LOG(debug) << "Set Trigger min limit for " << (*det).sName << " to " << iVal;
      return;
    }
  }
  LOG(warning) << "CbmAlgoBuildRawEvents::SetTriggerMaxNumber => "
                  "Doing nothing, detector neither reference nor in selection list! "
               << selDet;
}

void CbmAlgoBuildRawEvents::SetTriggerMinLayersNumber(ECbmModuleId selDet, UInt_t uVal)
{
  /// Check first if reference detector
  if (fRefDet.detId == selDet) {
    fRefDet.fuTriggerMinLayers = uVal;
    LOG(debug) << "Set Trigger min fired layers limit for " << fRefDet.sName << " to " << uVal;
    return;
  }

  /// Loop on selection detectors
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    if ((*det).detId == selDet) {
      (*det).fuTriggerMinLayers = uVal;
      LOG(debug) << "Set Trigger min fired layers limit for " << (*det).sName << " to " << uVal;
      return;
    }
  }
  LOG(warning) << "CbmAlgoBuildRawEvents::SetTriggerMinLayersNumber => "
                  "Doing nothing, detector neither reference nor in selection list! "
               << selDet;
}

void CbmAlgoBuildRawEvents::SetTriggerWindow(ECbmModuleId selDet, Double_t dWinBeg, Double_t dWinEnd)
{
  /// Check if valid time window: end strictly after beginning
  if (dWinEnd <= dWinBeg) {
    LOG(fatal) << "CbmAlgoBuildRawEvents::SetTriggerWindow => "
                  "Invalid time window: [ "
               << dWinBeg << ", " << dWinEnd << " ]";
  }

  Bool_t bFound = kFALSE;
  /// Check first if reference detector
  if (fRefDet.detId == selDet) {
    fRefDet.fdTimeWinBeg = dWinBeg;
    fRefDet.fdTimeWinEnd = dWinEnd;
    bFound               = kTRUE;
  }

  /// Loop on selection detectors
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    if ((*det).detId == selDet) {
      (*det).fdTimeWinBeg = dWinBeg;
      (*det).fdTimeWinEnd = dWinEnd;
      bFound              = kTRUE;
    }
  }

  if (kFALSE == bFound) {
    LOG(warning) << "CbmAlgoBuildRawEvents::SetTriggerWindow => "
                    "Doing nothing, detector neither reference nor in selection list! "
                 << selDet;
  }

  /// Update the variables storing the earliest and latest time window boundaries
  UpdateTimeWinBoundariesExtrema();
  /// Update the variable storing the size if widest time window for overlap detection
  UpdateWidestTimeWinRange();
}

void CbmAlgoBuildRawEvents::SetHistogramMaxDigiNb(ECbmModuleId selDet, Double_t dVal)
{
  /// Check first if reference detector
  if (fRefDet.detId == selDet) {
    fRefDet.fdHistMaxDigiNb = dVal;
    LOG(debug) << "Set histogram max digi nb for " << fRefDet.sName << " to " << dVal;
    return;
  }

  /// Loop on selection detectors
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    if ((*det).detId == selDet) {
      (*det).fdHistMaxDigiNb = dVal;
      LOG(debug) << "Set histogram max digi nb " << (*det).sName << " to " << dVal;
      return;
    }
  }
  LOG(warning) << "CbmAlgoBuildRawEvents::SetHistogramMaxDigiNb => "
                  "Doing nothing, detector neither reference nor in selection list! "
               << selDet;
}

void CbmAlgoBuildRawEvents::UpdateTimeWinBoundariesExtrema()
{
  /// Initialize with reference detector or explicit times
  if (fRefDet.detId != ECbmModuleId::kNotExist) {
    fdEarliestTimeWinBeg = fRefDet.fdTimeWinBeg;
    fdLatestTimeWinEnd   = fRefDet.fdTimeWinEnd;
  }
  else {
    fdEarliestTimeWinBeg = fdSeedTimeWinBeg;
    fdLatestTimeWinEnd   = fdSeedTimeWinEnd;
  }

  /// Loop on selection detectors
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    fdEarliestTimeWinBeg = std::min(fdEarliestTimeWinBeg, (*det).fdTimeWinBeg);
    fdLatestTimeWinEnd   = std::max(fdLatestTimeWinEnd, (*det).fdTimeWinEnd);
  }
}

void CbmAlgoBuildRawEvents::UpdateWidestTimeWinRange()
{
  /// Initialize with reference detector
  fdWidestTimeWinRange = GetSeedTimeWinRange();

  /// Loop on selection detectors
  for (std::vector<RawEventBuilderDetector>::iterator det = fvDets.begin(); det != fvDets.end(); ++det) {
    fdWidestTimeWinRange = std::max(fdWidestTimeWinRange, (*det).fdTimeWinEnd - (*det).fdTimeWinBeg);
  }
}

void CbmAlgoBuildRawEvents::SwitchBmonStation(int id, bool on)
{
  if (id < 0 || id >= (int) fUseBmonMap.size()) {
    LOG(warning) << "CbmAlgoBuildRawEvents::SwitchBmonStation: Bmon station id outside range. Skip.";
    return;
  }
  LOG(info) << "CbmAlgoBuildRawEvents::SwitchBmonStation: Bmon" << id << " station switched " << (on ? "ON" : "OFF")
            << " for trigger.";
  fUseBmonMap[id] = on;
}

bool CbmAlgoBuildRawEvents::filterBmon(int32_t add)
{
  // skip any filtering if not explicitly requested by user.
  if (!fUseBmonMap.size()) return true;

  // consistency check on the Bmon detector
  if (CbmTofAddress::GetSmType(add) != 5) {
    LOG(warning) << "CbmAlgoBuildRawEvents::filterBmon: Bmon digi with wrong address [GetSmType() != 5]. Skip.";
    return false;
  }
  size_t mod = (size_t) CbmTofAddress::GetChannelSide(add);
  // select Bmon detector
  if (!fUseBmonMap[mod]) {
    LOG(debug2) << "CbmAlgoBuildRawEvents::filterBmon : reject seed from Bmon" << mod;
    return false;
  }
  return true;
}

int32_t CbmAlgoBuildRawEvents::getNofFilteredBmonDigis(CbmEvent* event)
{
  // skip any filtering if not explicitly requested by user.
  if (!fUseBmonMap.size()) return event->GetNofData(ECbmDataType::kBmonDigi);

  int32_t ndigi(0);
  for (size_t idigi = 0; idigi < event->GetNofData(ECbmDataType::kBmonDigi); ++idigi) {
    uint idx                 = event->GetIndex(ECbmDataType::kBmonDigi, idigi);
    const CbmBmonDigi* pDigi = GetDigi<CbmBmonDigi>(idx);
    if (!filterBmon(pDigi->GetAddress())) continue;
    ndigi++;
  }
  return ndigi;
}

ClassImp(CbmAlgoBuildRawEvents)
