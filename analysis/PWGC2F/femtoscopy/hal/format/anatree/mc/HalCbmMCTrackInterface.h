/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICACBMATMMCTRACKINTERFACE_H_
#define NICACBMATMMCTRACKINTERFACE_H_

#include <Hal/McTrackInterface.h>


class HalCbmMCTrackInterface : public Hal::McTrackInterface {
 public:
  HalCbmMCTrackInterface();
  virtual ~HalCbmMCTrackInterface();
  ClassDef(HalCbmMCTrackInterface, 1)
};

#endif /* NICACBMATMMCTRACKINTERFACE_H_ */
