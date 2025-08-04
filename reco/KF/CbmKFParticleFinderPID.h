/* Copyright (C) 2014-2019 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Maksym Zyzak, Volker Friese [committer] */

//-----------------------------------------------------------
//-----------------------------------------------------------

#ifndef CbmKFParticleFinderPID_HH
#define CbmKFParticleFinderPID_HH

#include "CbmMCDataArray.h"
#include "FairTask.h"
#include "TString.h"

#include <vector>

class TClonesArray;
class TFile;
class TObject;
class CbmDigiManager;

class CbmKFParticleFinderPID : public FairTask {
 public:
  struct Cuts {
    Double_t fTrackLengthMin{0.};
    Double_t fTrackLengthMax{1.e10};
    Double_t fTrackTofTimeMin{0.};
    Double_t fTrackTofTimeMax{1.e10};
    Double_t fSP[7][5]{{0.}};  // ?
  };

  // Constructors/Destructors ---------
  CbmKFParticleFinderPID(const char* name = "CbmKFParticleFinderPID", Int_t iVerbose = 0);
  ~CbmKFParticleFinderPID();

  virtual InitStatus Init();
  virtual void Exec(Option_t* opt);
  virtual void Finish();

  void SetPIDMode(int mode) { fPIDMode = mode; }
  void UseNoPID() { fPIDMode = 0; }
  void UseMCPID() { fPIDMode = 1; }
  void UseDetectorPID() { fPIDMode = 2; }
  void SetCuts(const Cuts& val) { fCuts = val; }

  void DoNotUseTRD() { fTrdPIDMode = 0; }
  void UseTRDWknPID() { fTrdPIDMode = 1; }
  void UseTRDANNPID() { fTrdPIDMode = 2; }

  void DoNotUseRICH() { fRichPIDMode = 0; }
  void UseRICHRvspPID() { fRichPIDMode = 1; }
  void UseRICHANNPID() { fRichPIDMode = 2; }

  void DoNotUseMuch() { fMuchMode = 0; }
  void UseMuch() { fMuchMode = 1; }

  void UseSTSdEdX() { fUseSTSdEdX = kTRUE; }
  void DoNotUseSTSdEdX() { fUseSTSdEdX = kFALSE; }
  void UseTRDdEdX() { fUseTRDdEdX = kTRUE; }
  void DoNotUseTRDdEdX() { fUseTRDdEdX = kFALSE; }

  //setters for MuCh cuts
  void SetNMinStsHitsForMuon(int cut) { fMuchCutsInt[0] = cut; }
  void SetNMinMuchHitsForLMVM(int cut) { fMuchCutsInt[1] = cut; }
  void SetNMinMuchHitsForJPsi(int cut) { fMuchCutsInt[2] = cut; }
  void SetMaxChi2ForStsMuonTrack(float cut) { fMuchCutsFloat[0] = cut; }
  void SetMaxChi2ForMuchMuonTrack(float cut) { fMuchCutsFloat[1] = cut; }

  const std::vector<int>& GetPID() const { return fPID; }

 private:
  const CbmKFParticleFinderPID& operator=(const CbmKFParticleFinderPID&);
  CbmKFParticleFinderPID(const CbmKFParticleFinderPID&);

  void SetMCPID();
  void SetRecoPID();

  //input branches
  TClonesArray* fRecoEvents{nullptr};        // Array of CbmEvent objects
  TClonesArray* fGlobalTrackArray{nullptr};  // reco global tracks
  TClonesArray* fStsTrackArray{nullptr};     // reco STS tracks
  TClonesArray* fStsHitArray{nullptr};
  TClonesArray* fStsClusterArray{nullptr};
  TClonesArray* fRichRingArray{nullptr};
  TClonesArray* fMuchTrackArray{nullptr};  //input much tracks
  TClonesArray* fTrdTrackArray{nullptr};
  TClonesArray* fTrdHitArray{nullptr};
  TClonesArray* fTofHitArray{nullptr};  //input reco tracks

  CbmDigiManager* fDigiManager{nullptr};  // Interface to digi branch

  CbmMCDataArray* fMcTrackArray{nullptr};
  TClonesArray* fStsTrackMatchArray{nullptr};  // STS track match

  //PID variables
  Cuts fCuts{};  // cuts for reco PID
  Int_t fPIDMode{0};
  Int_t fTrdPIDMode{0};
  Int_t fRichPIDMode{0};
  Int_t fMuchMode{0};
  Bool_t fUseSTSdEdX{false};
  Bool_t fUseTRDdEdX{false};

  //MuCh cuts
  float fMuchCutsFloat[2] = {0.};
  int fMuchCutsInt[3]     = {0};

  std::vector<int> fPID;

  ClassDef(CbmKFParticleFinderPID, 0);
};

#endif
