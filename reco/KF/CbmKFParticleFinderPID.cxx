/* Copyright (C) 2014-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Maksym Zyzak, Volker Friese [committer] */

//-----------------------------------------------------------
//-----------------------------------------------------------

// Cbm Headers ----------------------
#include "CbmKFParticleFinderPID.h"

#include "CbmDigiManager.h"
#include "CbmEvent.h"
#include "CbmGlobalTrack.h"
#include "CbmMCDataManager.h"
#include "CbmMCTrack.h"
#include "CbmMuchTrack.h"
#include "CbmRichRing.h"
#include "CbmStsCluster.h"
#include "CbmStsDigi.h"
#include "CbmStsHit.h"
#include "CbmStsTrack.h"
#include "CbmTofHit.h"
#include "CbmTrackMatchNew.h"
#include "CbmTrdHit.h"
#include "CbmTrdTrack.h"
#include "FairRunAna.h"

//ROOT headers
#include "TClonesArray.h"

//c++ and std headers
#include <iostream>
#include <vector>
using std::vector;

ClassImp(CbmKFParticleFinderPID);

double vecMedian(const vector<double>& vec)
{
  double median          = 0.;
  vector<double> vecCopy = vec;
  sort(vecCopy.begin(), vecCopy.end());
  int size = vecCopy.size();
  if (size % 2 == 0) {
    median = (vecCopy[size / 2 - 1] + vecCopy[size / 2]) / 2;
  }
  else {
    median = vecCopy[size / 2];
  }
  return median;
}

CbmKFParticleFinderPID::CbmKFParticleFinderPID(const char* name, Int_t iVerbose) : FairTask(name, iVerbose)
{
  //MuCh cuts
  fMuchCutsInt[0]   = 7;     // N sts hits
  fMuchCutsInt[1]   = 14;    // N MuCh hits for LMVM
  fMuchCutsInt[2]   = 17;    // N MuCh hits for J/Psi
  fMuchCutsFloat[0] = 1.e6;  // STS  Chi2/NDF for muons
  fMuchCutsFloat[1] = 1.5;   // MuSh Chi2/NDF for muons
}

CbmKFParticleFinderPID::~CbmKFParticleFinderPID() {}

InitStatus CbmKFParticleFinderPID::Init()
{
  std::string prefix = std::string(GetName()) + "::Init: ";

  //Get ROOT Manager
  FairRootManager* ioman = FairRootManager::Instance();

  if (ioman == nullptr) {
    LOG(error) << prefix << "FairRootManager not instantiated!";
    return kERROR;
  }

  if (fPIDMode == 1) {  // PID from MC
    auto* mcManager = dynamic_cast<CbmMCDataManager*>(ioman->GetObject("MCDataManager"));
    if (mcManager == nullptr) {
      LOG(error) << prefix << "MC Data Manager not found!";
      return kERROR;
    }
    fMcTrackArray = mcManager->InitBranch("MCTrack");
    if (fMcTrackArray == nullptr) {
      LOG(error) << prefix << "MC track array not found!";
      return kERROR;
    }
    fStsTrackMatchArray = (TClonesArray*) ioman->GetObject("StsTrackMatch");
    if (!fStsTrackMatchArray) {
      LOG(error) << prefix << "track match array not found!";
      return kERROR;
    }
  }

  // Get sts tracks
  fStsTrackArray = (TClonesArray*) ioman->GetObject("StsTrack");
  if (fStsTrackArray == nullptr) {
    LOG(error) << prefix << "track-array not found!";
    return kERROR;
  }

  if (fPIDMode == 2) {  // PID from reconstructed data

    // Get reconstructed events

    fRecoEvents = dynamic_cast<TClonesArray*>(ioman->GetObject("CbmEvent"));
    if (nullptr == fRecoEvents) {
      LOG(error) << prefix << ": No event branch found. Run the event builder first.";
      return kERROR;
    }

    // Get global tracks
    fGlobalTrackArray = dynamic_cast<TClonesArray*>(ioman->GetObject("GlobalTrack"));
    if (fGlobalTrackArray == nullptr) {
      LOG(error) << prefix << "global track array not found!";
      return kERROR;
    }

    // Get STS hit
    fStsHitArray = dynamic_cast<TClonesArray*>(ioman->GetObject("StsHit"));
    if (fStsHitArray == nullptr) {
      LOG(error) << prefix << "STS hit array not found!";
      return kERROR;
    }

    // Get sts clusters
    fStsClusterArray = dynamic_cast<TClonesArray*>(ioman->GetObject("StsCluster"));
    if (fStsClusterArray == nullptr) {
      LOG(error) << prefix << "STS cluster array not found!";
      return kERROR;
    }

    // --- Digi Manager
    fDigiManager = CbmDigiManager::Instance();
    fDigiManager->Init();

    // --- Check input array (StsDigis)
    if (!fDigiManager->IsPresent(ECbmModuleId::kSts)) {
      LOG(fatal) << GetName() << ": No StsDigi branch in input!";
      return kERROR;
    }

    // Get ToF hits
    fTofHitArray = dynamic_cast<TClonesArray*>(ioman->GetObject("TofHit"));
    if (fTofHitArray == nullptr) {
      LOG(error) << prefix << "TOF hit array not found!";
      return kERROR;
    }

    fTrdTrackArray = dynamic_cast<TClonesArray*>(ioman->GetObject("TrdTrack"));

    if ((fTrdPIDMode > 0) && (fTrdTrackArray == nullptr)) {
      LOG(error) << prefix << "TRD track-array not found!";
      return kERROR;
    }

    if (fTrdTrackArray != nullptr) {
      fTrdHitArray = dynamic_cast<TClonesArray*>(ioman->GetObject("TrdHit"));
      if (fTrdHitArray == nullptr) {
        LOG(error) << prefix << "TRD hit array not found!";
        return kERROR;
      }
    }

    if (fRichPIDMode > 0) {
      fRichRingArray = dynamic_cast<TClonesArray*>(ioman->GetObject("RichRing"));
      if (fRichRingArray == nullptr) {
        LOG(error) << prefix << "Rich ring array not found!";
        return kERROR;
      }
    }

    if (fMuchMode > 0) {
      fMuchTrackArray = dynamic_cast<TClonesArray*>(ioman->GetObject("MuchTrack"));
      if (fMuchTrackArray == nullptr) {
        LOG(error) << prefix << "Much track-array not found!";
        return kERROR;
      }
    }

  }  // if (fPIDMode == 2)

  return kSUCCESS;
}

void CbmKFParticleFinderPID::Exec(Option_t* /*opt*/)
{
  fPID.clear();

  Int_t nTracks = fStsTrackArray->GetEntriesFast();
  fPID.resize(nTracks, -1);

  if (fPIDMode == 1) {
    SetMCPID();
  }
  if (fPIDMode == 2) {
    SetRecoPID();
  }
}

void CbmKFParticleFinderPID::Finish() {}

void CbmKFParticleFinderPID::SetMCPID()
{
  Int_t nTracks = fStsTrackArray->GetEntriesFast();

  for (int iTr = 0; iTr < nTracks; iTr++) {
    fPID[iTr] = -2;

    CbmTrackMatchNew* stsTrackMatch = (CbmTrackMatchNew*) fStsTrackMatchArray->At(iTr);
    if (stsTrackMatch->GetNofLinks() == 0) {
      continue;
    }
    Float_t bestWeight  = 0.f;
    Float_t totalWeight = 0.f;
    Int_t mcTrackId     = -1;
    Int_t iFile         = -1;
    Int_t iEvent        = -1;

    for (int iLink = 0; iLink < stsTrackMatch->GetNofLinks(); iLink++) {
      totalWeight += stsTrackMatch->GetLink(iLink).GetWeight();
      if (stsTrackMatch->GetLink(iLink).GetWeight() > bestWeight) {
        bestWeight = stsTrackMatch->GetLink(iLink).GetWeight();
        mcTrackId  = stsTrackMatch->GetLink(iLink).GetIndex();
        iFile      = stsTrackMatch->GetLink(iLink).GetFile();
        iEvent     = stsTrackMatch->GetLink(iLink).GetEntry();
      }
    }
    if (bestWeight / totalWeight < 0.7) {
      continue;
    }

    CbmMCTrack* cbmMCTrack = dynamic_cast<CbmMCTrack*>(fMcTrackArray->Get(iFile, iEvent, mcTrackId));

    if (!(TMath::Abs(cbmMCTrack->GetPdgCode()) == 11 || TMath::Abs(cbmMCTrack->GetPdgCode()) == 13
          || TMath::Abs(cbmMCTrack->GetPdgCode()) == 211 || TMath::Abs(cbmMCTrack->GetPdgCode()) == 321
          || TMath::Abs(cbmMCTrack->GetPdgCode()) == 2212 || TMath::Abs(cbmMCTrack->GetPdgCode()) == 1000010020
          || TMath::Abs(cbmMCTrack->GetPdgCode()) == 1000010030 || TMath::Abs(cbmMCTrack->GetPdgCode()) == 1000020030
          || TMath::Abs(cbmMCTrack->GetPdgCode()) == 1000020040)) {
      fPID[iTr] = -1;
    }
    else {
      fPID[iTr] = cbmMCTrack->GetPdgCode();
    }

  }  // track loop
}


void CbmKFParticleFinderPID::SetRecoPID()
{
  std::string prefix = std::string(GetName()) + "::SetRecoPID: ";

  const Double_t m2TOF[7] = {0.885, 0.245, 0.019479835, 0., 3.49, 7.83, 1.95};

  const Int_t PdgHypo[9] = {2212, 321, 211, -11, 1000010029, 1000010030, 1000020030, -13, -19};

  if (!fRecoEvents) {
    LOG(fatal) << GetName() << " no reco events! ";
    return;
  }

  int nRecoEvents = fRecoEvents->GetEntriesFast();

  for (int iEvent = 0; iEvent < nRecoEvents; iEvent++) {

    CbmEvent* event = dynamic_cast<CbmEvent*>(fRecoEvents->At(iEvent));

    double eventTime = event->GetTzero();

    if (eventTime < 0.) {
      LOG(error) << prefix << "T0 of the event " << iEvent
                 << " is undefined. Ensure that the CbmRecoT0 task is run before this task.";
      continue;
    }

    int nTracksEvent = event->GetNofData(ECbmDataType::kGlobalTrack);

    for (Int_t iTrack = 0; iTrack < nTracksEvent; iTrack++) {

      int globalTrackIndex = event->GetIndex(ECbmDataType::kGlobalTrack, iTrack);
      auto* globalTrack    = static_cast<const CbmGlobalTrack*>(fGlobalTrackArray->At(globalTrackIndex));

      Int_t stsTrackIndex = globalTrack->GetStsTrackIndex();
      if (stsTrackIndex < 0) {
        continue;
      }
      assert(stsTrackIndex < (int) fPID.size());

      Bool_t isElectronTRD  = 0;
      Bool_t isElectronRICH = 0;
      Bool_t isElectron     = 0;

      CbmStsTrack* cbmStsTrack = (CbmStsTrack*) fStsTrackArray->At(stsTrackIndex);

      const FairTrackParam* stsPar = cbmStsTrack->GetParamFirst();
      TVector3 mom;
      stsPar->Momentum(mom);

      Double_t p  = mom.Mag();
      Double_t pt = mom.Perp();
      Double_t pz = sqrt(p * p - pt * pt);
      Int_t q     = stsPar->GetQp() > 0 ? 1 : -1;

      if (fRichPIDMode == 1) {
        if (fRichRingArray) {
          Int_t richIndex = globalTrack->GetRichRingIndex();
          if (richIndex > -1) {
            CbmRichRing* richRing = (CbmRichRing*) fRichRingArray->At(richIndex);
            if (richRing) {
              Double_t axisA = richRing->GetAaxis();
              Double_t axisB = richRing->GetBaxis();
              Double_t dist  = 0;  // richRing->GetDistance();

              Double_t fMeanA    = 4.95;
              Double_t fMeanB    = 4.54;
              Double_t fRmsA     = 0.30;
              Double_t fRmsB     = 0.22;
              Double_t fRmsCoeff = 3.5;
              Double_t fDistCut  = 1.;


              //            if(fElIdAnn->DoSelect(richRing, p) > -0.5) isElectronRICH = 1;
              if (p < 5.) {
                if (fabs(axisA - fMeanA) < fRmsCoeff * fRmsA && fabs(axisB - fMeanB) < fRmsCoeff * fRmsB
                    && dist < fDistCut) {
                  isElectronRICH = 1;
                }
              }
              else {
                ///3 sigma
                // Double_t polAaxis = 5.80008 - 4.10118 / (momentum - 3.67402);
                // Double_t polBaxis = 5.58839 - 4.75980 / (momentum - 3.31648);
                // Double_t polRaxis = 5.87252 - 7.64641/(momentum - 1.62255);
                ///2 sigma
                Double_t polAaxis = 5.64791 - 4.24077 / (p - 3.65494);
                Double_t polBaxis = 5.41106 - 4.49902 / (p - 3.52450);
                //Double_t polRaxis = 5.66516 - 6.62229/(momentum - 2.25304);
                if (axisA < (fMeanA + fRmsCoeff * fRmsA) && axisA > polAaxis && axisB < (fMeanB + fRmsCoeff * fRmsB)
                    && axisB > polBaxis && dist < fDistCut) {
                  isElectronRICH = 1;
                }
              }
            }
          }
        }
      }

      if (fTrdPIDMode == 1) {
        if (fTrdTrackArray) {
          Int_t trdIndex = globalTrack->GetTrdTrackIndex();
          if (trdIndex > -1) {
            CbmTrdTrack* trdTrack = (CbmTrdTrack*) fTrdTrackArray->At(trdIndex);
            if (trdTrack) {
              if (trdTrack->GetPidWkn() > 0.635) {
                isElectronTRD = 1;
              }
            }
          }
        }
      }

      if (fTrdPIDMode == 2) {
        if (fTrdTrackArray) {
          Int_t trdIndex = globalTrack->GetTrdTrackIndex();
          if (trdIndex > -1) {
            CbmTrdTrack* trdTrack = (CbmTrdTrack*) fTrdTrackArray->At(trdIndex);
            if (trdTrack) {
              if (trdTrack->GetPidANN() > 0.9) {
                isElectronTRD = 1;
              }
            }
          }
        }
      }

      // dEdX in TRD
      double dEdXTRD = 1e6;  // in case if TRD is not used
      if (fTrdTrackArray) {
        Int_t trdIndex = globalTrack->GetTrdTrackIndex();
        if (trdIndex > -1) {
          CbmTrdTrack* trdTrack = (CbmTrdTrack*) fTrdTrackArray->At(trdIndex);
          if (trdTrack) {
            Double_t eloss = 0.;
            for (Int_t iTRD = 0; iTRD < trdTrack->GetNofHits(); iTRD++) {
              Int_t TRDindex    = trdTrack->GetHitIndex(iTRD);
              CbmTrdHit* trdHit = (CbmTrdHit*) fTrdHitArray->At(TRDindex);
              eloss += trdHit->GetELoss();
            }
            if (trdTrack->GetNofHits() > 0.) {
              dEdXTRD = 1e6 * pz / p * eloss / trdTrack->GetNofHits();
            }
          }
        }
      }

      //STS dE/dX
      vector<double> dEdxAllveto;
      int nClustersWveto = cbmStsTrack->GetNofStsHits() + cbmStsTrack->GetNofStsHits();  //assume all clusters with veto
      double dr          = 1.;
      for (int iHit = 0; iHit < cbmStsTrack->GetNofStsHits(); ++iHit) {
        bool frontVeto = kFALSE, backVeto = kFALSE;
        CbmStsHit* stsHit = (CbmStsHit*) fStsHitArray->At(cbmStsTrack->GetStsHitIndex(iHit));

        double x, y, z, xNext, yNext, zNext;
        x = stsHit->GetX();
        y = stsHit->GetY();
        z = stsHit->GetZ();

        if (iHit != cbmStsTrack->GetNofStsHits() - 1) {
          CbmStsHit* stsHitNext = (CbmStsHit*) fStsHitArray->At(cbmStsTrack->GetStsHitIndex(iHit + 1));
          xNext                 = stsHitNext->GetX();
          yNext                 = stsHitNext->GetY();
          zNext                 = stsHitNext->GetZ();
          dr = sqrt((xNext - x) * (xNext - x) + (yNext - y) * (yNext - y) + (zNext - z) * (zNext - z))
               / (zNext - z);  // if *300um, you get real reconstructed dr
        }                      // else dr stay previous

        CbmStsCluster* frontCluster = (CbmStsCluster*) fStsClusterArray->At(stsHit->GetFrontClusterId());
        CbmStsCluster* backCluster  = (CbmStsCluster*) fStsClusterArray->At(stsHit->GetBackClusterId());

        if (!frontCluster || !backCluster) {
          LOG(info) << "CbmKFParticleFinderPID: no front or back cluster";
          continue;
        }

        //check if at least one digi in a cluster has overflow --- charge is registered in the last ADC channel #31
        for (int iDigi = 0; iDigi < frontCluster->GetNofDigis(); ++iDigi) {
          if (31 == (fDigiManager->Get<CbmStsDigi>(frontCluster->GetDigi(iDigi))->GetCharge())) {
            frontVeto = kTRUE;
          }
        }
        for (int iDigi = 0; iDigi < backCluster->GetNofDigis(); ++iDigi) {
          if (31 == (fDigiManager->Get<CbmStsDigi>(backCluster->GetDigi(iDigi))->GetCharge())) {
            backVeto = kTRUE;
          }
        }

        if (!frontVeto) {
          dEdxAllveto.push_back((frontCluster->GetCharge()) / dr);
          nClustersWveto--;
        }
        if (!backVeto) {
          dEdxAllveto.push_back((backCluster->GetCharge()) / dr);
          nClustersWveto--;
        }
      }
      float dEdXSTS = 1.e6;
      if (dEdxAllveto.size() != 0) {
        dEdXSTS = vecMedian(dEdxAllveto);
      }


      int isMuon = 0;
      if (fMuchTrackArray && fMuchMode > 0) {
        Int_t muchIndex = globalTrack->GetMuchTrackIndex();
        if (muchIndex > -1) {
          CbmMuchTrack* muchTrack = (CbmMuchTrack*) fMuchTrackArray->At(muchIndex);
          if (muchTrack) {
            if ((cbmStsTrack->GetChiSq() / cbmStsTrack->GetNDF()) < fMuchCutsFloat[0]
                && (muchTrack->GetChiSq() / muchTrack->GetNDF()) < fMuchCutsFloat[1]
                && cbmStsTrack->GetTotalNofHits() >= fMuchCutsInt[0]) {
              if (muchTrack->GetNofHits() >= fMuchCutsInt[1]) {
                isMuon = 2;
              }
              if (muchTrack->GetNofHits() >= fMuchCutsInt[2]) {
                isMuon = 1;
              }
            }
          }
        }
      }

      if (p > 1.5) {
        if (isElectronRICH && isElectronTRD) {
          isElectron = 1;
        }
        if (fRichPIDMode == 0 && isElectronTRD) {
          isElectron = 1;
        }
        if (fTrdPIDMode == 0 && isElectronRICH) {
          isElectron = 1;
        }
      }
      else if (isElectronRICH) {
        isElectron = 1;
      }

      if (fTofHitArray && isMuon == 0) {
        Double_t l = globalTrack->GetLength();  // l is calculated by global tracking
        if (!((l > fCuts.fTrackLengthMin) && (l < fCuts.fTrackLengthMax))) {
          continue;
        }

        Int_t tofHitIndex = globalTrack->GetTofHitIndex();
        if (tofHitIndex < 0) {
          continue;
        }

        const CbmTofHit* tofHit = static_cast<const CbmTofHit*>(fTofHitArray->At(tofHitIndex));
        if (!tofHit) {
          LOG(error) << "null Tof hit pointer";
          continue;
        }

        Double_t time = tofHit->GetTime() - eventTime;

        if (!((time > fCuts.fTrackTofTimeMin) && (time < fCuts.fTrackTofTimeMax))) {
          continue;
        }

        Double_t m2 = p * p * (1. / ((l / time / 29.9792458) * (l / time / 29.9792458)) - 1.);

        Double_t sigma[7];
        Double_t dm2[7];

        for (int iSigma = 0; iSigma < 7; iSigma++) {
          sigma[iSigma] = fCuts.fSP[iSigma][0] + fCuts.fSP[iSigma][1] * p + fCuts.fSP[iSigma][2] * p * p
                          + fCuts.fSP[iSigma][3] * p * p * p + fCuts.fSP[iSigma][4] * p * p * p * p;
          dm2[iSigma] = fabs(m2 - m2TOF[iSigma]) / sigma[iSigma];
        }

        if (isElectron) {
          if (dm2[3] > 3.) {
            isElectron = 0;
          }
        }

        int iPdg        = 2;
        Double_t dm2min = dm2[2];

        if (!isElectron && isMuon == 0) {
          //         if(p>12.) continue;

          for (int jPDG = 0; jPDG < 7; jPDG++) {
            if (jPDG == 3) {
              continue;
            }
            if (dm2[jPDG] < dm2min) {
              iPdg   = jPDG;
              dm2min = dm2[jPDG];
            }
          }

          if (dm2min > 2) {
            iPdg = -1;
          }

          Double_t dEdXTRDthresholdProton = 10.;
          if (iPdg == 6) {
            if (fUseTRDdEdX && (dEdXTRD < dEdXTRDthresholdProton)) {
              iPdg = 0;
            }
            if (fUseSTSdEdX && (dEdXSTS < 7.5e4)) {
              iPdg = 0;
            }
          }

          if (iPdg > -1) {
            fPID[stsTrackIndex] = q * PdgHypo[iPdg];
          }
        }
      }

      if (isElectron) {
        fPID[stsTrackIndex] = q * PdgHypo[3];
      }

      if (isMuon == 1) {
        fPID[stsTrackIndex] = q * PdgHypo[7];
      }
      if (isMuon == 2) {
        fPID[stsTrackIndex] = q * PdgHypo[8];
      }
    }  // track loop
  }    // event loop
}
