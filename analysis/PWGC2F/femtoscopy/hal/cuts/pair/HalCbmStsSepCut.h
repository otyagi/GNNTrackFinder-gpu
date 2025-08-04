/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef PAIRCUTS_CBMSTSEXITSEPCUT_H_
#define PAIRCUTS_CBMSTSEXITSEPCUT_H_

#include "HalCbmFormatTypes.h"
#include "HalCbmPairCut.h"

class HalCbmStsExitSepCut : public HalCbmPairCut {
 protected:
  Bool_t PassHbt(Hal::TwoTrack* pair);
  Bool_t PassAnaTree(Hal::TwoTrack* pair);

 public:
  HalCbmStsExitSepCut();
  virtual ~HalCbmStsExitSepCut();
  ClassDef(HalCbmStsExitSepCut, 1)
};

class HalCbmStsEntranceSepCut : public HalCbmPairCut {
 protected:
  Bool_t PassHbt(Hal::TwoTrack* pair);
  Bool_t PassAnaTree(Hal::TwoTrack* pair);

 public:
  HalCbmStsEntranceSepCut();
  virtual ~HalCbmStsEntranceSepCut(){};
  ClassDef(HalCbmStsEntranceSepCut, 1)
};

class HalCbmStsSeparationCutLayers : public HalCbmPairCut {
 protected:
  Bool_t PassHbt(Hal::TwoTrack* pair);
  Bool_t PassAnaTree(Hal::TwoTrack* pair);

 public:
  HalCbmStsSeparationCutLayers();
  virtual ~HalCbmStsSeparationCutLayers();
  ClassDef(HalCbmStsSeparationCutLayers, 1);
};


#endif /* PAIRCUTS_CBMSTSEXITSEPCUT_H_ */
