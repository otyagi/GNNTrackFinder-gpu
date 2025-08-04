/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "CbmQACoreManager.h"

#include "CbmAnaTreeSource.h"
#include "HalCbmBasicTrackCuts.h"
#include "HalCbmEvent.h"
#include "HalCbmFullEvent.h"
#include "HalCbmHbtEvent.h"
#include "HalCbmHbtFullEvent.h"
#include "HalCbmMCEvent.h"
#include "HalCbmTofCut.h"

#include <FairRunAna.h>

#include <TString.h>

#include <Hal/TrackAna.h>


CbmQACoreManager::CbmQACoreManager()
{
  fEta[0]  = -2;
  fEta[1]  = 4;
  fDCAz[0] = 0;
  fDCAz[1] = 10;
}

Hal::Event* CbmQACoreManager::GetFormat(eFormatType type, eAnaType ana)
{
  switch (ana) {
    case eAnaType::kDefault:
      switch (type) {
        case eFormatType::kComplex: return new HalCbmFullEvent(); break;
        case eFormatType::kReco: return new HalCbmEvent(); break;
        case eFormatType::kSim: return new HalCbmMCEvent(); break;
      }
      break;
    case eAnaType::kHbt:
      switch (type) {
        case eFormatType::kComplex: return new HalCbmHbtFullEvent(); break;
        case eFormatType::kReco: return new HalCbmHbtEvent(); break;
        case eFormatType::kSim: return new HalCbmMCEvent(); break;
      }
      break;
  }
  return nullptr;
}

void CbmQACoreManager::SetRecoTrackCut(Hal::TrackAna* ana, ePidCut cut, eParticleType primary, TString flag)
{
  HalCbmBasicTrackCuts cuts;
  switch (cut) {
    case ePidCut::kAntiProton:
      cuts.SetCharge(-1);
      cuts.SetNHits(4, 20);
      cuts.SetM2(0.75, 1.1);
      break;
    case ePidCut::kKaonMinus:
      cuts.SetCharge(-1);
      cuts.SetNHits(4, 20);
      cuts.SetM2(0.15, 0.29);
      break;
    case ePidCut::kKaonPlus:
      cuts.SetCharge(1);
      cuts.SetNHits(4, 20);
      cuts.SetM2(0.15, 0.29);
      break;
    case ePidCut::kPionMinus:
      cuts.SetCharge(-1);
      cuts.SetNHits(4, 20);
      cuts.SetM2(-0.1, 0.15);
      break;
    case ePidCut::kPionPlus:
      cuts.SetCharge(1);
      cuts.SetNHits(4, 20);
      cuts.SetM2(0.0, 0.05);
      cuts.SetPt(0.2, 10);
      break;
    case ePidCut::kProton:
      cuts.SetPt(0.2, 10);
      cuts.SetEta(1, 3.5);
      cuts.SetCharge(1);
      cuts.SetNHits(0, 20);
      cuts.SetNStsHits(5, 20);
      cuts.SetM2(0.75, 1.1);
      //cuts.SetChi2(0, 12);
      cuts.GetTofCut()->SetDownPoints(0.5, 1, 0.7, 0.8, 20, 0.76);
      break;
    default: break;
  }
  if (fUsetPid == kFALSE) {
    cuts.SetM2(-1E+9, 1E+9);
  }
  switch (primary) {
    case eParticleType::kPrimaryOnly:
      cuts.SetDCAXY(0, 1);
      cuts.SetDCAZ(-0.01, 0.01);
      switch (cut) {
        case ePidCut::kProton: {
          cuts.SetDCAXY(0, 0.5);
        } break;
        default: break;
      }
      break;
    case eParticleType::kSecondaryOnly: {
      cuts.SetDCAXY(1, 1E+3);
      cuts.SetDCAZ(0.00, 1E+3);
      switch (cut) {
        case ePidCut::kProton: {
          cuts.SetDCAXY(1, 1E+3);
        } break;
        default: break;
      }
      break;
    } break;
    case eParticleType::kAll: break;
  }
  cuts.SetOptionForAllCuts(flag);

  ana->AddCutsAndMonitors(cuts);
}

FairRunAna* CbmQACoreManager::GetRunAna(TString outFile, TString simFile, TString /*recoFile*/, TString /*parFile*/)
{
  FairRunAna* run        = new FairRunAna();
  CbmAnaTreeSource* file = new CbmAnaTreeSource(simFile);
  run->SetOutputFile(outFile);
  run->SetSource(file);
  return run;
}

CbmQACoreManager::~CbmQACoreManager() {}
