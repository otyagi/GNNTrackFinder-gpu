/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_CUTS_PAIR_CBMPAIRCUT_H_
#define CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_CUTS_PAIR_CBMPAIRCUT_H_

#include "HalCbmFormatTypes.h"

#include <Hal/TwoTrackCut.h>

class HalCbmPairCut : public Hal::TwoTrackCut {
 protected:
  HalCbm::EFormatType fDataType;
  /**
   * pass pair of hbt particles
   * @param pair
   * @return
   */
  virtual Bool_t PassHbt(Hal::TwoTrack* pair) = 0;
  /**
   * pass pair of cbm tracks from analysis tree
   * @param pair
   * @return
   */
  virtual Bool_t PassAnaTree(Hal::TwoTrack* pair) = 0;

 public:
  HalCbmPairCut(Int_t parNo = 1);
  virtual Bool_t Pass(Hal::TwoTrack* pair);
  virtual Bool_t Init(Int_t format_id);
  virtual ~HalCbmPairCut();
  HalCbmPairCut& operator=(const HalCbmPairCut& other);
  HalCbmPairCut(const HalCbmPairCut& other);
  ClassDef(HalCbmPairCut, 1)
};


#endif /* CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_CUTS_PAIR_CBMPAIRCUT_H_ */
