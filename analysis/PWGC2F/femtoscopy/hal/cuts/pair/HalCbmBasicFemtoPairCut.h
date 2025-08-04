/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMBASICFEMTOPAIRCUT_H_
#define CBMBASICFEMTOPAIRCUT_H_

#include <TVector3.h>

#include <Hal/CutsAndMonitors.h>

class HalCbmStsExitSepCut;
class HalCbmDeltaPhiDeltaThetaStarCut;

class HalCbmBasicFemtoPairCut : public Hal::CutsAndMonitors {
  Hal::CutMonAxisConf fDeltaEtaAx;
  Hal::CutMonAxisConf fDeltaPhiStarAx;
  Hal::CutMonAxisConf fStsExitSepAx;
  HalCbmStsExitSepCut* GetStsExitCut() const { return (HalCbmStsExitSepCut*) CutAt(0); }
  HalCbmDeltaPhiDeltaThetaStarCut* GetDeltaPhiEtaStarCut() const { return (HalCbmDeltaPhiDeltaThetaStarCut*) CutAt(1); }

 protected:
  virtual void AddAllCutMonitorRequests(Option_t* opt);

 public:
  HalCbmBasicFemtoPairCut();
  void SetSeparationMonitorAxis(Int_t nbins, Double_t min, Double_t max) { fStsExitSepAx.SetAxis(nbins, min, max); }
  void SetDeltaPhiStarAxis(Int_t bin, Double_t min, Double_t max) { fDeltaPhiStarAx.SetAxis(bin, min, max); }
  void SetDeltaEtaStarAxis(Int_t bin, Double_t min, Double_t max) { fDeltaEtaAx.SetAxis(bin, min, max); }
  void SetDeltaPhiStarCut(Double_t min, Double_t max);
  void SetDeltaEtaStarCut(Double_t min, Double_t max);
  void SetR(Double_t R);
  void SetStsExitSeparationCut(Double_t min, Double_t max);
  virtual ~HalCbmBasicFemtoPairCut();
  ClassDef(HalCbmBasicFemtoPairCut, 1)
};

#endif /* CBMBASICFEMTOPAIRCUT_H_ */
