/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMNHITSCUT_H_
#define CBMNHITSCUT_H_

#include <Rtypes.h>
#include <RtypesCore.h>

#include <Hal/TrackExpCut.h>

namespace Hal
{
  class Track;
} /* namespace Hal */

/**
 * set cut on number of hits
 * 0 - total number of hits (MVD + STS + TRD)
 * 1 - MVD hits
 * 2 - STS hits
 * 3 - TRD hits
 */
class HalCbmNHitsCut : public Hal::TrackExpCut {
 public:
  HalCbmNHitsCut();
  void SetNTotalHits(Int_t min, Int_t max) { SetMinMax(min, max, 0); };
  void SetNMvdHits(Int_t min, Int_t max) { SetMinMax(min, max, 1); };
  void SetNStsHits(Int_t min, Int_t max) { SetMinMax(min, max, 2); };
  void SetNTrdHits(Int_t min, Int_t max) { SetMinMax(min, max, 3); };
  virtual Bool_t Init(Int_t task_id);
  virtual Bool_t Pass(Hal::Track* track);
  virtual ~HalCbmNHitsCut();
  ClassDef(HalCbmNHitsCut, 1)
};
#endif /* CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_CUTS_ANATREECUTS_TRACK_CBMNHITSCUT_H_ */
