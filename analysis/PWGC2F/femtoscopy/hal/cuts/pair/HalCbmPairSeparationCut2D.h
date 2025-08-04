/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_CUTS_PAIR_CBMPAIRSEPARATIONCUT2D_H_
#define CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_CUTS_PAIR_CBMPAIRSEPARATIONCUT2D_H_


#include "HalCbmPairCut.h"

namespace Hal
{
  class TwoTrack;
} /* namespace Hal */

class HalCbmPairSeparationCut2D : public HalCbmPairCut {
 protected:
  Double_t fR;
  virtual Bool_t PassHbt(Hal::TwoTrack* pair);
  virtual Bool_t PassAnaTree(Hal::TwoTrack* pair);

 public:
  HalCbmPairSeparationCut2D();
  void SetDeltaZ(Double_t min, Double_t max) { SetMinMax(min, max, 0); }
  void SetDeltaXY(Double_t min, Double_t max) { SetMinMax(min, max, 1); }
  void SetR(Double_t r) { fR = r; }
  virtual Hal::Package* Report() const;
  virtual ~HalCbmPairSeparationCut2D();
  ClassDef(HalCbmPairSeparationCut2D, 1)
};

#endif /* CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_CUTS_PAIR_CBMPAIRSEPARATIONCUT2D_H_ */
