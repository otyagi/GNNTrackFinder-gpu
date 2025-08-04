/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmTrack.h"

#include "CbmHelix.h"
#include "HalCbmDetectorID.h"
#include "HalCbmEventInterface.h"

#include <Hal/DataFormat.h>
#include <Hal/Event.h>
#include <Hal/ExpEvent.h>
#include <Hal/Track.h>

HalCbmTrack::HalCbmTrack()
  : fTofTrack(new Hal::ToFTrack())
  , fHelix()
  , fChi2Vertex(0)
  , fMvdHits(0)
  , fStsHits(0)
  , fTrdHits(0)
{
}

Hal::DetectorTrack* HalCbmTrack::GetDetTrack(const UInt_t detID) const
{
  if (detID == HalCbm::DetectorID::kTOF) {
    return fTofTrack;
  }
  return nullptr;
}

HalCbmTrack::HalCbmTrack(const HalCbmTrack& other) : Hal::ExpTrack(other)
{
  fTofTrack   = new Hal::ToFTrack(*other.fTofTrack);
  fHelix      = other.fHelix;
  fChi2Vertex = other.fChi2Vertex;
  fMvdHits    = other.fMvdHits;
  fStsHits    = other.fStsHits;
  fTrdHits    = other.fTrdHits;
}

HalCbmTrack& HalCbmTrack::operator=(const HalCbmTrack& other)
{
  if (this == &other) return *this;
  Hal::ExpTrack::operator=(other);
  *fTofTrack             = *other.fTofTrack;
  fHelix                 = other.fHelix;
  fChi2Vertex            = other.fChi2Vertex;
  fMvdHits               = other.fMvdHits;
  fStsHits               = other.fStsHits;
  fTrdHits               = other.fTrdHits;
  return *this;
}

void HalCbmTrack::BuildHelix()
{
  TVector3 mom(GetPx(), GetPy(), GetPz());
  TVector3 pos(GetDCA().X() + GetEvent()->GetVertex()->X(), GetDCA().Y() + GetEvent()->GetVertex()->Y(),
               GetDCA().Z() + GetEvent()->GetVertex()->Z());
  fHelix.Build(pos, mom, GetCharge());
}

HalCbmTrack::~HalCbmTrack() { delete fTofTrack; }

void HalCbmTrack::CopyData(Hal::Track* other)
{
  Hal::ExpTrack::CopyData(other);
  fTofTrack   = (static_cast<HalCbmTrack*>(other))->fTofTrack;
  fHelix      = (static_cast<HalCbmTrack*>(other))->fHelix;
  fChi2Vertex = (static_cast<HalCbmTrack*>(other))->fChi2Vertex;
  fMvdHits    = (static_cast<HalCbmTrack*>(other))->fMvdHits;
  fStsHits    = (static_cast<HalCbmTrack*>(other))->fStsHits;
  fTrdHits    = (static_cast<HalCbmTrack*>(other))->fTrdHits;
}
