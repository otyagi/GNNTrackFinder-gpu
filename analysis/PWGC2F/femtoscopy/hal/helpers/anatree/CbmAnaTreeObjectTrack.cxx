/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "CbmAnaTreeObjectTrack.h"

CbmAnaTreeObjectTrack::CbmAnaTreeObjectTrack() {}

CbmAnaTreeObjectTrack::~CbmAnaTreeObjectTrack() {}

CbmAnaTreeObjectTrack::CbmAnaTreeObjectTrack(const CbmAnaTreeObjectTrack& other)
  : TObject()
  , fTreeParticle(other.fTreeParticle)
  , fTreeTrack(other.fTreeTrack)
  , fTreeHit(other.fTreeHit)
{
}

CbmAnaTreeObjectTrack& CbmAnaTreeObjectTrack::operator=(const CbmAnaTreeObjectTrack& other)
{
  if (this == &other) return *this;
  fTreeParticle = other.fTreeParticle;
  fTreeTrack    = other.fTreeTrack;
  fTreeHit      = other.fTreeHit;
  return *this;
}
