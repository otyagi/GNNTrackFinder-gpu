/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICACBMATMCTRACK_H_
#define NICACBMATMCTRACK_H_

#include <Hal/McTrack.h>
class HalCbmMCTrack : public Hal::McTrack {
 public:
  HalCbmMCTrack();
  virtual ~HalCbmMCTrack();
  ClassDef(HalCbmMCTrack, 1)
};
#endif /* NICACBMATMCTRACK_H_ */
