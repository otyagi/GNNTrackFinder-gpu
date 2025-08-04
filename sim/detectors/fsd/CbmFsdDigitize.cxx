/* Copyright (C) 2023 Physikalisches Institut Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alla Maevskaya, Selim Seddiki, Sergey Morozov, Volker Friese, Evgeny Kashirin, Lukas Chlad [committer] */

#include "CbmFsdDigitize.h"

#include "CbmFsdAddress.h"
#include "CbmFsdDigi.h"
#include "CbmFsdDigiPar.h"
#include "CbmFsdPoint.h"
#include "CbmLink.h"
#include "CbmMatch.h"

#include <FairRootManager.h>
#include <FairRun.h>
#include <FairRuntimeDb.h>
#include <Logger.h>

#include <TArrayD.h>
#include <TClonesArray.h>
#include <TMath.h>
#include <TRandom3.h>
#include <TStopwatch.h>

#include <cassert>
#include <iomanip>
#include <iostream>
#include <map>

using std::cout;
using std::endl;
using std::fixed;
using std::right;
using std::setprecision;
using std::setw;

using std::map;
using std::pair;

// -----   Public method Init   --------------------------------------------
InitStatus CbmFsdDigitize::Init()
{
  if (!fEventMode) { LOG(info) << GetName() << " uses TimeBased mode."; }
  else {
    LOG(info) << GetName() << " uses Events mode.";
  }

  // Get RootManager
  FairRootManager* ioman = FairRootManager::Instance();

  // Get input array
  fPointArray = dynamic_cast<TClonesArray*>(ioman->GetObject("FsdPoint"));
  if (nullptr == fPointArray) {
    LOG(error) << GetName() << ": Error in reading from file! No FsdPoint array.";
    return kERROR;
  }
  TString objectName = fPointArray->GetClass()->GetName();
  if (0 != objectName.CompareTo("CbmFsdPoint")) {
    LOG(fatal) << GetName() << ": TClonesArray does not contain data of the expected class CbmFsdPoint";
  }

  // Initialise parameters
  InitParams();

  // Create and register output array
  RegisterOutput();

  // Statistics
  fNumEvents = 0;
  fNumPoints = 0.;
  fNumDigis  = 0.;
  fTimeTot   = 0.;

  LOG(info) << GetName() << ": Initialisation successful " << kSUCCESS;
  return kSUCCESS;
}
// -------------------------------------------------------------------------

void CbmFsdDigitize::SetParContainers()
{
  LOG(info) << GetName() << ": Get the digi parameters for FSD";

  // Get Base Container
  FairRun* ana        = FairRun::Instance();
  FairRuntimeDb* rtdb = ana->GetRuntimeDb();

  fDigiPar = dynamic_cast<CbmFsdDigiPar*>(rtdb->getContainer("CbmFsdDigiPar"));
  if (fDigiPar == nullptr) {
    LOG(fatal) << GetName() << ": parameter container CbmFsdDigiPar not available in current RuntimeDb!!";
  }
}

void CbmFsdDigitize::InitParams()
{
  if (!fDigiPar) { LOG(fatal) << GetName() << ": parameter container CbmFsdDigiPar not found!!"; }

  // Get values from loaded parameter container
  fNumPhotoDets = fDigiPar->GetNumPhotoDets();
  fNumUnits     = fDigiPar->GetNumUnits();

  if (fNumPhotoDets < 1 || fNumUnits < 1) {
    LOG(fatal) << GetName() << ": parameter values for FSD digitization does not meet a sanity check!!";
  }

  fTimeResolution.Set(fNumUnits);
  fEnergyResolution.Set(fNumUnits);
  fDeadTime.Set(fNumUnits);

  for (Int_t iu = 0; iu < fNumUnits; iu++) {
    fTimeResolution[iu]   = fDigiPar->GetTimeResolution(iu);
    fEnergyResolution[iu] = fDigiPar->GetEnergyResolution(iu);
    fDeadTime[iu]         = fDigiPar->GetDeadTime(iu);

    if (fDeadTime[iu] < 0. || fTimeResolution[iu] < 0. || fEnergyResolution[iu] < 0.) {
      LOG(fatal) << GetName() << ": parameter values for FSD digitization does not meet a sanity check!!";
    }
  }
}

// -----   Public method Exec   --------------------------------------------
void CbmFsdDigitize::Exec(Option_t*)
{
  // Event info (for event time)
  GetEventInfo();

  TStopwatch timer;
  timer.Start();

  Int_t nDigis = 0;

  // digis that are distant in time to this event (and thus can not be modified anymore) should be send to DAQ
  if (!fEventMode) ReleaseBuffer(kFALSE);


  Int_t nPoints = fPointArray->GetEntriesFast();
  LOG(debug) << fName << ": processing event " << fCurrentEvent << " at t = " << fCurrentEventTime << " ns";

  // Declare some variables
  CbmFsdPoint* point = nullptr;

  Int_t modId      = -1;
  Int_t unitId     = -1;
  Int_t photodetId = -1;

  if (fNumPhotoDets > 1) {
    // random uniform split into photo detectors?!
    photodetId = gRandom->Integer(static_cast<UInt_t>(fNumPhotoDets));
  }
  else {
    photodetId = 0;
  }

  // Loop over FsdPoints
  for (Int_t iPoint = 0; iPoint < nPoints; iPoint++) {
    point = static_cast<CbmFsdPoint*>(fPointArray->At(iPoint));
    if (!point) continue;

    int32_t pointAddress = static_cast<int32_t>(point->GetDetectorID());
    modId =
      static_cast<Int_t>(CbmFsdAddress::GetElementId(pointAddress, static_cast<int32_t>(CbmFsdAddress::Level::Module)));
    unitId =
      static_cast<Int_t>(CbmFsdAddress::GetElementId(pointAddress, static_cast<int32_t>(CbmFsdAddress::Level::Unit)));
    Double_t eloss   = point->GetEnergyLoss() + gRandom->Gaus(0., fEnergyResolution[unitId]);
    Double_t time    = fCurrentEventTime + point->GetTime() + gRandom->Gaus(0., fTimeResolution[unitId]);
    uint32_t address = static_cast<uint32_t>(CbmFsdAddress::GetAddress(
      static_cast<uint32_t>(unitId), static_cast<uint32_t>(modId), static_cast<uint32_t>(photodetId)));

    CbmLink link(eloss, iPoint, fCurrentMCEntry, fCurrentInput);  // weight of link is the energy loss

    auto it = fDigiBuffer.find(pointAddress);
    if (it != fDigiBuffer.end()) {
      // there is already in buffer a digi with this key
      Double_t timeDiff = std::fabs(time - it->second.first->GetTime());
      if (timeDiff < fDeadTime[unitId]) {
        // modify found digi : add energy deposition, modify time if earlier
        it->second.first->SetEdep(it->second.first->GetEdep() + eloss);
        if (time < it->second.first->GetTime()) it->second.first->SetTime(time);
        // add link to digimatch
        it->second.second->AddLink(link);
      }
      else {
        // not within time cut -> send the digi from buffer and replace by the new one
        CbmFsdDigi* oldDigi =
          new CbmFsdDigi(it->second.first->GetAddress(), it->second.first->GetTime(), it->second.first->GetEdep());
        CbmMatch* oldDigiMatch = new CbmMatch(*it->second.second);
        if (fCreateMatches) SendData(oldDigi->GetTime(), oldDigi, oldDigiMatch);
        else
          SendData(oldDigi->GetTime(), oldDigi);
        fDigiBuffer.erase(it);

        CbmFsdDigi* digi = new CbmFsdDigi(address, time, eloss);
        CbmMatch* match  = new CbmMatch();
        match->AddLink(link);
        fDigiBuffer.insert(std::make_pair(pointAddress, std::make_pair(digi, match)));
        nDigis++;
      }
    }
    else {
      // digi with this key is not yet in buffer -> insert it
      CbmFsdDigi* digi = new CbmFsdDigi(address, time, eloss);
      CbmMatch* match  = new CbmMatch();
      match->AddLink(link);
      fDigiBuffer.insert(std::make_pair(pointAddress, std::make_pair(digi, match)));
      nDigis++;
    }
  }  // Loop over MCPoints

  // in EventMode send and clear the whole buffer after MCEvent is processed
  if (fEventMode) ReleaseBuffer(kTRUE);

  // --- Event log
  timer.Stop();
  LOG(info) << "+ " << setw(15) << GetName() << ": Event " << setw(6) << right << fCurrentEvent << " at " << fixed
            << setprecision(3) << fCurrentEventTime << " ns, points: " << nPoints << ", digis: " << nDigis
            << ". Exec time " << setprecision(6) << timer.RealTime() << " s.";

  // --- Run statistics
  fNumEvents++;
  fNumPoints += nPoints;
  fNumDigis += nDigis;
  fTimeTot += timer.RealTime();
}
// -------------------------------------------------------------------------


// -----   End-of-run   ----------------------------------------------------
void CbmFsdDigitize::Finish()
{
  std::cout << std::endl;
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Finish run";
  if (!fEventMode) ReleaseBuffer(kTRUE);  // in time-based mode, send all what is left in buffer to DAQ
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Events processed    : " << fNumEvents;
  LOG(info) << "FsdPoint / event    : " << setprecision(1) << fNumPoints / Double_t(fNumEvents);
  LOG(info) << "FsdDigi / event     : " << fNumDigis / Double_t(fNumEvents);
  LOG(info) << "Digis per point     : " << setprecision(6) << fNumDigis / fNumPoints;
  LOG(info) << "Real time per event : " << fTimeTot / Double_t(fNumEvents) << " s";
  LOG(info) << "=====================================";
}
// -------------------------------------------------------------------------


void CbmFsdDigitize::ReleaseBuffer(Bool_t sendEverything)
{
  if (sendEverything) {
    // send everything
    for (const auto& dp : fDigiBuffer) {
      CbmFsdDigi* digi =
        new CbmFsdDigi(dp.second.first->GetAddress(), dp.second.first->GetTime(), dp.second.first->GetEdep());
      CbmMatch* digiMatch = new CbmMatch(*dp.second.second);
      if (fCreateMatches) SendData(digi->GetTime(), digi, digiMatch);
      else
        SendData(digi->GetTime(), digi);
    }  // # digi buffer
    fDigiBuffer.clear();
  }
  else {
    // send only if time difference to the new start of event is larger than deadtime
    std::vector<int32_t> keysSent;
    for (const auto& dp : fDigiBuffer) {
      Int_t unitId = static_cast<Int_t>(CbmFsdAddress::GetElementId(static_cast<int32_t>(dp.second.first->GetAddress()),
                                                                    static_cast<int32_t>(CbmFsdAddress::Level::Unit)));
      Double_t timeDiff = std::fabs(dp.second.first->GetTime() - fCurrentEventTime);
      if (timeDiff > fDeadTime[unitId]) {
        // send this digi
        CbmFsdDigi* digi =
          new CbmFsdDigi(dp.second.first->GetAddress(), dp.second.first->GetTime(), dp.second.first->GetEdep());
        CbmMatch* digiMatch = new CbmMatch(*dp.second.second);
        if (fCreateMatches) SendData(digi->GetTime(), digi, digiMatch);
        else
          SendData(digi->GetTime(), digi);

        // save which keys were sent and should be removed
        keysSent.push_back(dp.first);
      }  // ? time
    }    // # digi buffer

    // remove the sent elements from buffer
    for (auto& key : keysSent) {
      fDigiBuffer.erase(key);
    }
  }  // ? sendEverything
}

ClassImp(CbmFsdDigitize)
