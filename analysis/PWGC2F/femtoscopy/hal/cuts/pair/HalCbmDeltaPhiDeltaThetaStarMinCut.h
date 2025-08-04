/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_CUTS_PAIR_CBMDELTAPHIDELTATHETASTARMINCUT_H_
#define CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_CUTS_PAIR_CBMDELTAPHIDELTATHETASTARMINCUT_H_

#include "HalCbmFormatTypes.h"
#include "HalCbmPairCut.h"

class HalCbmDeltaPhiDeltaThetaStarMinCut : public HalCbmPairCut {
 protected:
  virtual Bool_t PassHbt(Hal::TwoTrack* pair);
  virtual Bool_t PassAnaTree(Hal::TwoTrack* pair);

 public:
  HalCbmDeltaPhiDeltaThetaStarMinCut();
  virtual ~HalCbmDeltaPhiDeltaThetaStarMinCut(){};
  ClassDef(HalCbmDeltaPhiDeltaThetaStarMinCut, 1)
};


#endif /* CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_CUTS_PAIR_CBMDELTAPHIDELTATHETASTARMINCUT_H_ */
