/* Copyright (C) 2023-2023 UGiessen, Giessen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Martin Beyer [committer] */

#include "CbmRichDigiQa.h"

#include "CbmDigiManager.h"
#include "CbmHistManager.h"
#include "CbmRichDetectorData.h"
#include "CbmRichDigi.h"
#include "CbmRichDigiMapManager.h"

#include <FairRootManager.h>
#include <Logger.h>

#include <TDirectory.h>
#include <TFile.h>

InitStatus CbmRichDigiQa::Init()
{
  CbmRichDigiMapManager::GetInstance();

  fDigiMan = CbmDigiManager::Instance();
  fDigiMan->Init();
  if (!fDigiMan->IsPresent(ECbmModuleId::kRich)) LOG(fatal) << "CbmRichDigiQa::Init: No RichDigi array!";

  InitHistograms();

  return kSUCCESS;
}

void CbmRichDigiQa::InitHistograms()
{
  fHM = new CbmHistManager();

  fHM->Create1<TH1D>("fhDigiDt", "same address consecutive digis time diff;dt [ns];entries", 1001, -0.5, 10000.5);

  fHM->Create1<TH1D>("fhDigiDtEdge", "same address consecutive digis time diff;dt [ns];entries", 101, -0.5, 100.5);

  fHM->Create1<TH1D>("fhDigiDistH", "horizontal distance of digis same pmt;DIndX [in pixels];entries", 11, -0.5, 10.5);

  fHM->Create1<TH1D>("fhDigiDistV", "vertical distance of digis same pmt;DIndY [in pixels];entries", 11, -0.5, 10.5);

  fHM->Create2<TH2D>("fhDigiNeighbours", "digi neighbours same pmt;DIndX [in pixels];DIndY [in pixels];entries", 5,
                     -2.5, 2.5, 5, -2.5, 2.5);
}

void CbmRichDigiQa::Exec(Option_t* /*option*/)
{
  LOG(debug) << GetName() << " Event " << fEventNum;
  fEventNum++;

  Int_t nofDigis = fDigiMan->GetNofDigis(ECbmModuleId::kRich);
  for (Int_t i = 0; i < nofDigis; i++) {
    const CbmRichDigi* digi = fDigiMan->Get<CbmRichDigi>(i);
    if (!digi) continue;
    Int_t pmtId = CbmRichDigiMapManager::GetInstance().GetPixelDataByAddress(digi->GetAddress())->fPmtId;
    if (fToTLimitLow > 0. && fToTLimitHigh > 0.) {
      if (digi->GetToT() < fToTLimitHigh && digi->GetToT() > fToTLimitLow) {
        fFiredTimes[digi->GetAddress()].push_back(digi->GetTime());
        fPmtDigisTimeAddress[pmtId].push_back(std::make_pair(digi->GetTime(), digi->GetAddress()));
      }
    }
    else {
      fFiredTimes[digi->GetAddress()].push_back(digi->GetTime());
      fPmtDigisTimeAddress[pmtId].push_back(std::make_pair(digi->GetTime(), digi->GetAddress()));
    }
  }

  // Fill deadtimes
  for (auto& [address, times] : fFiredTimes) {
    std::sort(times.begin(), times.end());
    for (auto it = times.begin(); it != times.end(); ++it) {
      if (*it < 0.)
        LOG(warn) << "Digi time < 0.: "
                  << "address: " << address << " time: " << *it << " ns";

      if (it != times.begin()) {
        Double_t dt = *it - *std::prev(it);
        fHM->H1("fhDigiDt")->Fill(dt);
        if (dt < 100.5) {
          fHM->H1("fhDigiDtEdge")->Fill(dt);
        }
      }
    }
  }

  // Fill neighbour digis
  for (auto& [pmt, digis] : fPmtDigisTimeAddress) {
    std::sort(digis.begin(), digis.end());
    for (auto it0 = digis.begin(); it0 != digis.end(); ++it0) {
      Int_t indX0 = CbmRichDigiMapManager::GetInstance().GetPixelDataByAddress(it0->second)->fPixelId % 8;
      Int_t indY0 = CbmRichDigiMapManager::GetInstance().GetPixelDataByAddress(it0->second)->fPixelId / 8;
      for (auto it1 = it0 + 1; it1 != digis.end(); ++it1) {
        if (abs(it0->first - it1->first) < fNeighbourTimeLimit) {
          Int_t indX1 = CbmRichDigiMapManager::GetInstance().GetPixelDataByAddress(it1->second)->fPixelId % 8;
          Int_t indY1 = CbmRichDigiMapManager::GetInstance().GetPixelDataByAddress(it1->second)->fPixelId / 8;
          if (indX0 == indX1 && indY0 == indY1) continue;
          if (indY0 == indY1) fHM->H1("fhDigiDistH")->Fill(indX1 - indX0);
          if (indX0 == indX1) fHM->H1("fhDigiDistV")->Fill(indY1 - indY0);
          if (abs(indX0 - indX1) > 1 || abs(indY0 - indY1) > 1) continue;
          fHM->H2("fhDigiNeighbours")->Fill(indX1 - indX0, indY0 - indY1);
        }
        else if (it1->first - it0->first > fNeighbourTimeLimit) {
          break;
        }
      }
    }
  }

  fFiredTimes.clear();
  fPmtDigisTimeAddress.clear();
}

void CbmRichDigiQa::Finish()
{
  TDirectory* oldir = gDirectory;
  TFile* outFile    = FairRootManager::Instance()->GetOutFile();
  if (outFile != nullptr) {
    outFile->mkdir(GetName());
    outFile->cd(GetName());
    fHM->WriteToFile();
  }
  gDirectory->cd(oldir->GetPath());
}
