/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMDELTAPHIDELTATHETACUT_H_
#define CBMDELTAPHIDELTATHETACUT_H_
#include "HalCbmFormatTypes.h"
#include "HalCbmPairCut.h"

class HalCbmDeltaPhiDeltaThetaStarCut : public HalCbmPairCut {
 protected:
  Double_t fR;
  virtual Bool_t PassHbt(Hal::TwoTrack* pair);
  virtual Bool_t PassAnaTree(Hal::TwoTrack* pair);

 public:
  HalCbmDeltaPhiDeltaThetaStarCut();
  HalCbmDeltaPhiDeltaThetaStarCut& operator=(const HalCbmDeltaPhiDeltaThetaStarCut& other);
  void SetR(Double_t r) { fR = r; }
  virtual Hal::Package* Report() const;
  virtual ~HalCbmDeltaPhiDeltaThetaStarCut();
  ClassDef(HalCbmDeltaPhiDeltaThetaStarCut, 1)
};

class HalCbmDeltaPhiDeltaThetaCut : public HalCbmPairCut {
 protected:
 public:
  HalCbmDeltaPhiDeltaThetaCut();
  Bool_t Pass(Hal::TwoTrack* pair);
  virtual ~HalCbmDeltaPhiDeltaThetaCut();
  ClassDef(HalCbmDeltaPhiDeltaThetaCut, 1)
};


class HalCbmDeltaPhiDeltaThetaStarCutLayers : public HalCbmPairCut {
 protected:
  Double_t fR;
  virtual Bool_t PassHbt(Hal::TwoTrack* pair);
  virtual Bool_t PassAnaTree(Hal::TwoTrack* pair);

 public:
  HalCbmDeltaPhiDeltaThetaStarCutLayers();
  HalCbmDeltaPhiDeltaThetaStarCutLayers& operator=(const HalCbmDeltaPhiDeltaThetaStarCutLayers& other);
  void SetR(Double_t r) { fR = r; }
  virtual Hal::Package* Report() const;
  virtual ~HalCbmDeltaPhiDeltaThetaStarCutLayers();
  ClassDef(HalCbmDeltaPhiDeltaThetaStarCutLayers, 1)
};


#endif /* CBMDELTAPHIDELTATHETACUT_H_ */
