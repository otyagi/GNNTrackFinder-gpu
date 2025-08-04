/* Copyright (C) 2015-2024 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Martin Beyer, Andrey Lebedev [committer], Volker Friese, Florian Uhlig, Semen Lebedev */

/**
* \file CbmRichDigitizer.cxx
*
* \author S.Lebedev
* \date 2015
**/

#include "CbmRichDigitizer.h"

#include "CbmLink.h"
#include "CbmMCTrack.h"
#include "CbmMatch.h"
#include "CbmRichDetectorData.h"
#include "CbmRichDigi.h"
#include "CbmRichDigiMapManager.h"
#include "CbmRichGeoManager.h"
#include "CbmRichPoint.h"

#include <FairRootManager.h>
#include <Logger.h>

#include <TClonesArray.h>
#include <TMath.h>
#include <TRandom.h>
#include <TStopwatch.h>

#include <cstdlib>
#include <iomanip>
#include <iostream>

using std::fixed;
using std::left;
using std::right;
using std::setprecision;
using std::setw;

InitStatus CbmRichDigitizer::Init()
{
  std::cout << std::endl;
  LOG(info) << "==========================================================";
  LOG(info) << GetName() << ": Initialisation";

  if (!fEventMode) { LOG(info) << "CbmRichDigitizer uses TimeBased mode."; }
  else {
    LOG(info) << "CbmRichDigitizer uses Events mode.";
  }

  FairRootManager* manager = FairRootManager::Instance();

  // --- Initialise helper singletons in order not to do it in event
  // --- processing (spoils timing measurement)
  CbmRichDigiMapManager::GetInstance();
  CbmRichGeoManager::GetInstance();

  fRichPoints = static_cast<TClonesArray*>(manager->GetObject("RichPoint"));
  if (!fRichPoints) LOG(fatal) << "CbmRichDigitizer::Init: No RichPoint array!";

  fMcTracks = static_cast<TClonesArray*>(manager->GetObject("MCTrack"));
  if (!fMcTracks) LOG(fatal) << "CbmRichDigitizer::Init: No MCTrack array!";

  RegisterOutput();

  // --- Read list of inactive channels
  if (!fInactiveChannelFileName.IsNull()) {
    LOG(info) << GetName() << ": Reading inactive channels from " << fInactiveChannelFileName;
    auto result = ReadInactiveChannels();
    if (!result.second) {
      LOG(error) << GetName() << ": Error in reading from file! Task will be inactive.";
      return kFATAL;
    }
    LOG(info) << GetName() << ": " << std::get<0>(result) << " lines read from file, " << fInactiveChannels.size()
              << " channels set inactive";
  }

  LOG(info) << GetName() << ": Initialisation successful";
  LOG(info) << "==========================================================";
  std::cout << std::endl;

  return kSUCCESS;
}

void CbmRichDigitizer::Exec(Option_t* /*option*/)
{
  TStopwatch timer;
  timer.Start();

  Double_t oldEventTime = fCurrentEventTime;
  GetEventInfo();

  Int_t nPoints = ProcessMcEvent();

  Double_t tNoiseStart = fEventNum ? oldEventTime : fRunStartTime;
  if (fProduceNoise) AddDarkRateNoise(tNoiseStart, fCurrentEventTime);

  if (fEventMode) fPixelsLastFiredTime.clear();
  Double_t processTime = fEventMode ? -1. : fCurrentEventTime;
  Int_t nDigis         = ProcessBuffers(processTime);

  // --- Statistics
  timer.Stop();
  fEventNum++;
  fNofPoints += nPoints;
  fNofDigis += nDigis;
  fTimeTot += timer.RealTime();

  // --- Event log
  LOG(info) << "+ " << setw(15) << GetName() << ": Event " << setw(6) << right << fCurrentEvent << " at " << fixed
            << setprecision(3) << fCurrentEventTime << " ns, points: " << nPoints << ", digis: " << nDigis
            << ". Exec time " << setprecision(6) << timer.RealTime() << " s.";
}

void CbmRichDigitizer::Finish()
{
  TStopwatch timer;
  if (!fEventMode) {
    std::cout << std::endl;
    LOG(info) << GetName() << ": Finish run";
    LOG(info) << GetName() << ": Processing analogue buffers";
    timer.Start();
    Int_t nDigis = ProcessBuffers(-1.);
    timer.Stop();
    fTimeTot += timer.RealTime();
    LOG(info) << GetName() << ": " << nDigis << " digis created and sent to DAQ";
    fNofDigis += nDigis;
  }

  std::cout << std::endl;
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Events processed    : " << fEventNum;
  LOG(info) << "RichPoint / event   : " << setprecision(6) << fNofPoints / Double_t(fEventNum);
  LOG(info) << "RichDigi / event    : " << fNofDigis / Double_t(fEventNum);
  LOG(info) << "Digis per point     : " << setprecision(6) << fNofDigis / Double_t(fNofPoints);
  LOG(info) << "Real time per event : " << fTimeTot / Double_t(fEventNum) << " s";
  LOG(info) << "=====================================";
}

Int_t CbmRichDigitizer::ProcessMcEvent()
{
  Int_t nofRichPoints = fRichPoints->GetEntriesFast();

  LOG(debug) << fName << ": EventNum:" << fCurrentEvent << " InputNum:" << fCurrentInput
             << " EventTime:" << fCurrentEventTime << " nofRichPoints:" << nofRichPoints;

  for (Int_t j = 0; j < nofRichPoints; j++) {
    CbmRichPoint* point = static_cast<CbmRichPoint*>(fRichPoints->At(j));
    ProcessPoint(point, j, fCurrentMCEntry, fCurrentInput);
  }

  AddEventNoise(fCurrentMCEntry, fCurrentInput);

  return nofRichPoints;
}

void CbmRichDigitizer::ProcessPoint(CbmRichPoint* point, Int_t pointId, Int_t eventNum, Int_t inputNum)
{
  Int_t address = point->GetDetectorID();
  if (address == -1) {
    LOG(error) << "CbmRichDigitizer::ProcessPoint: address == -1";
    return;
  }
  Int_t trackId = point->GetTrackID();
  if (trackId < 0) return;
  CbmMCTrack* mcTrack = static_cast<CbmMCTrack*>(fMcTracks->At(trackId));
  if (!mcTrack) return;
  Int_t gcode = TMath::Abs(mcTrack->GetPdgCode());

  Bool_t isDetected = false;
  // For photons weight with quantum efficiency of PMT
  if (gcode == 50000050) {
    TVector3 mom;
    point->Momentum(mom);
    Double_t momTotal = TMath::Sqrt(mom.Px() * mom.Px() + mom.Py() * mom.Py() + mom.Pz() * mom.Pz());
    isDetected        = fPmt.isPhotonDetected(fDetectorType, momTotal);
  }
  else {  // If not photon
    // Worst case: assume that all charged particles crossing
    // the PMTplane leave Cherenkov light in the PMTglass which will be detected
    isDetected = true;
  }

  if (isDetected) {
    Double_t time = fCurrentEventTime + point->GetTime();
    CbmLink link(1., pointId, eventNum, inputNum);
    AddSignalToBuffer(address, time, link);
    if (gcode == 50000050 && address > 0) AddCrossTalk(address, time, link);
    if (fClusterSize > 0 && mcTrack->GetCharge() != 0. && address > 0) {
      AddChargedParticleCluster(address, time, eventNum, inputNum);
    }
  }
}

void CbmRichDigitizer::AddSignalToBuffer(Int_t address, Double_t time, const CbmLink& link)
{
  fSignalBuffer[address].emplace_back(std::make_pair(time, new CbmLink(link)));
}

void CbmRichDigitizer::AddCrossTalk(Int_t address, Double_t time, const CbmLink& link)
{
  std::vector<Int_t> directHorizontalPixels =
    CbmRichDigiMapManager::GetInstance().GetDirectNeighbourPixels(address, true, false);
  std::vector<Int_t> directVerticalPixels =
    CbmRichDigiMapManager::GetInstance().GetDirectNeighbourPixels(address, false, true);
  std::vector<Int_t> diagonalPixels = CbmRichDigiMapManager::GetInstance().GetDiagonalNeighbourPixels(address);
  /* clang-format off */
  Double_t crosstalkProbability = 1 - pow(1 - fCrossTalkProbability[0], directHorizontalPixels.size()) *
                                      pow(1 - fCrossTalkProbability[1], directVerticalPixels.size()) *
                                      pow(1 - fCrossTalkProbability[2], diagonalPixels.size());
  Int_t crosstalkAddress = -1;
  if (gRandom->Uniform() < crosstalkProbability) {
    // Split into 3 intervals based on crosstalk probability and number of pixels
    Double_t denom = directHorizontalPixels.size() * fCrossTalkProbability[0] + 
                     directVerticalPixels.size()   * fCrossTalkProbability[1] + 
                     diagonalPixels.size()         * fCrossTalkProbability[2];
    // Threshold values for end of each interval
    Double_t thHorizontal = directHorizontalPixels.size() * fCrossTalkProbability[0] / denom;
    Double_t thVertical = directVerticalPixels.size()     * fCrossTalkProbability[1] / denom + thHorizontal;    
    Double_t rnd = gRandom->Uniform();
    if (rnd < thHorizontal) {
      crosstalkAddress = directHorizontalPixels[gRandom->Integer(directHorizontalPixels.size())];
    }
    else if (rnd < thVertical) {
      crosstalkAddress = directVerticalPixels[gRandom->Integer(directVerticalPixels.size())];
    } else {
      crosstalkAddress = diagonalPixels[gRandom->Integer(diagonalPixels.size())];
    }
  }
  /* clang-format on */
  if (crosstalkAddress != -1) AddSignalToBuffer(crosstalkAddress, time, link);
}

void CbmRichDigitizer::AddChargedParticleCluster(Int_t address, Double_t time, Int_t eventNum, Int_t inputNum)
{
  // pixelId % 8 is x index, pixelId / 8 is y index
  auto sourcePixelIdx = std::div(CbmRichDigiMapManager::GetInstance().GetPixelDataByAddress(address)->fPixelId, 8);
  std::vector<Int_t> neighbourAddresses =
    CbmRichDigiMapManager::GetInstance().GetNxNNeighbourPixels(address, fClusterSize);
  for (const Int_t& addr : neighbourAddresses) {
    auto iPixelIdx = std::div(CbmRichDigiMapManager::GetInstance().GetPixelDataByAddress(addr)->fPixelId, 8);
    Double_t d     = TMath::Sqrt((sourcePixelIdx.rem - iPixelIdx.rem) * (sourcePixelIdx.rem - iPixelIdx.rem)
                             + (sourcePixelIdx.quot - iPixelIdx.quot) * (sourcePixelIdx.quot - iPixelIdx.quot));
    if (gRandom->Uniform(1.) < fClusterSignalProbability / d) {
      CbmLink link(1., -1, eventNum, inputNum);
      AddSignalToBuffer(addr, time, link);
    }
  }
}

void CbmRichDigitizer::AddEventNoise(Int_t eventNum, Int_t inputNum)
{
  Int_t nofPixels        = static_cast<Int_t>(CbmRichDigiMapManager::GetInstance().GetPixelAddresses().size());
  Double_t nofRichPoints = fRichPoints->GetEntriesFast();
  Double_t nofNoiseDigis = nofRichPoints * nofPixels * fEventNoiseInterval * (fEventNoiseRate / 1.e9);

  LOG(debug) << "CbmRichDigitizer::AddEventNoise NofAllPixels:" << nofPixels << " nofNoiseDigis:" << nofNoiseDigis;

  for (Int_t j = 0; j < nofNoiseDigis; j++) {
    Int_t address = CbmRichDigiMapManager::GetInstance().GetRandomPixelAddress();
    CbmLink link(1., -1, eventNum, inputNum);
    Double_t time = fCurrentEventTime + gRandom->Uniform(0, fEventNoiseInterval);
    AddSignalToBuffer(address, time, link);
  }
}

void CbmRichDigitizer::AddDarkRateNoise(Double_t oldEventTime, Double_t newEventTime)
{
  Int_t nofPixels         = static_cast<Int_t>(CbmRichDigiMapManager::GetInstance().GetPixelAddresses().size());
  Double_t dT             = newEventTime - oldEventTime;
  Double_t nofNoisePixels = nofPixels * dT * (fDarkRatePerPixel / 1.e9);

  LOG(debug) << "CbmRichDigitizer::AddDarkRateNoise deltaTime:" << dT << " nofPixels:" << nofPixels
             << " nofNoisePixels:" << nofNoisePixels;

  for (Int_t j = 0; j < nofNoisePixels; j++) {
    Int_t address = CbmRichDigiMapManager::GetInstance().GetRandomPixelAddress();
    CbmLink link(1., -1, -1, -1);
    Double_t time = gRandom->Uniform(oldEventTime, newEventTime);
    AddSignalToBuffer(address, time, link);
  }
}

Int_t CbmRichDigitizer::ProcessBuffers(Double_t processTime)
{
  Double_t maxNewDigiTime = processTime - fPixelDeadTime;
  Int_t nofDigis {};

  for (auto& dm : fSignalBuffer) {
    std::sort(
      dm.second.begin(), dm.second.end(),
      [](const std::pair<Double_t, CbmLink*>& a, const std::pair<Double_t, CbmLink*>& b) { return a.first < b.first; });

    CbmRichDigi* digi   = nullptr;
    CbmMatch* digiMatch = nullptr;
    auto it             = dm.second.begin();
    for (; it != dm.second.end(); ++it) {
      Double_t signalTime = it->first;
      if (processTime >= 0. && signalTime > processTime) break;
      CbmLink* link      = it->second;
      Bool_t createsDigi = true;

      auto itFpm = fPixelsLastFiredTime.find(dm.first);
      if (itFpm != fPixelsLastFiredTime.end()) {
        Double_t lastFiredTime = itFpm->second;
        if (signalTime - lastFiredTime < fPixelDeadTime) createsDigi = false;
      }

      if (createsDigi) {
        if (digi) {  // Add previous digi if exists
          fCreateMatches ? SendData(digi->GetTime(), digi, digiMatch) : SendData(digi->GetTime(), digi);
          nofDigis++;
          digi      = nullptr;  // Needed because of potential break statement
          digiMatch = nullptr;
        }
        Double_t digiTime = signalTime + gRandom->Gaus(0., fTimeResolution);
        if (processTime >= 0. && digiTime > maxNewDigiTime) break;
        digi = new CbmRichDigi();
        digi->SetAddress(dm.first);
        digi->SetTime(digiTime);
        if (fCreateMatches) {  // If fCreateMatches is false, SendData does not take over ownership via unique_ptr
          digiMatch = new CbmMatch();
          digiMatch->AddLink(*link);
        }
        fPixelsLastFiredTime[dm.first] = signalTime;  // Deadtime is from signal to signal
      }
      else {
        if (digi && digiMatch) digiMatch->AddLink(*link);
      }
      delete it->second;
    }
    dm.second.erase(dm.second.begin(), it);
    if (digi) {  // Add last digi if exists
      fCreateMatches ? SendData(digi->GetTime(), digi, digiMatch) : SendData(digi->GetTime(), digi);
      nofDigis++;
    }
  }

  return nofDigis;
}

ClassImp(CbmRichDigitizer)
