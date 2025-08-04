/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMTOFCUT_H_
#define CBMTOFCUT_H_
#include "Hal/TrackToFMass2Cut.h"


class HalCbmTofCut : public Hal::TrackToFMass2Cut {
 public:
  HalCbmTofCut();
  void AcceptTracksWithoutTof() { SetMinMax(0, 1, 1); };
  void AcceptTracksOnlyWithToF() { SetMinMax(1, 1, 1); };
  static Int_t Flag() { return 1; };
  static Int_t M2() { return 0; };
  virtual Bool_t Init(Int_t task_id);
  virtual Bool_t Pass(Hal::Track* track);
  virtual ~HalCbmTofCut();
  ClassDef(HalCbmTofCut, 1)
};

#endif /* CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_CUTS_ANATREECUTS_TRACK_CBMTOFCUT_H_ */
