/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICACBMATRECOTRACKINTERFACE_H_
#define NICACBMATRECOTRACKINTERFACE_H_

#include "CbmStsTrack.h"

#include <Hal/TrackInterface.h>

class HalCbmTrackInterface : public Hal::TrackInterface {
 public:
  HalCbmTrackInterface();
  virtual ~HalCbmTrackInterface();
  ClassDef(HalCbmTrackInterface, 1)
};
#endif /* NICACBMATTRACKINTERFACE_H_ */
