/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef COMMONCUTS_TRACK_CBMTOFM2POLYCUT_H_
#define COMMONCUTS_TRACK_CBMTOFM2POLYCUT_H_

#include <Hal/TrackToFMass2Cut.h>

namespace Hal
{
  class Track;
}

class HalCbmTofM2PolyCut : public Hal::TrackToFMass2Cut {
 public:
  HalCbmTofM2PolyCut();
  Bool_t Pass(Hal::Track* track);
  virtual ~HalCbmTofM2PolyCut();
  ClassDef(HalCbmTofM2PolyCut, 1)
};
#endif /* CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_CUTS_COMMONCUTS_TRACK_CBMTOFM2POLYCUT_H_ */
