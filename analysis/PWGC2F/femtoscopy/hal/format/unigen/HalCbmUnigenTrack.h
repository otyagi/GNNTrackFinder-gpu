/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICAUNIGENTRACK_H_
#define NICAUNIGENTRACK_H_

#include "UParticle.h"

#include <iostream>

#include <Hal/McTrack.h>

/**
 * class for representation of track from unigen in "fake" format
 */

class HalCbmUnigenTrack : public Hal::McTrack {
 public:
  HalCbmUnigenTrack();
  virtual ~HalCbmUnigenTrack();
  ClassDef(HalCbmUnigenTrack, 1)
};

#endif /* NICAUNIGENTRACK_H_ */
