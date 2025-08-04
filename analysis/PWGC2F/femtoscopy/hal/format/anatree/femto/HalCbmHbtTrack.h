/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMHBTTRACK_H_
#define CBMHBTTRACK_H_

#include "CbmHelix.h"

#include <TVector3.h>

#include <Hal/ExpTrack.h>

namespace Hal
{
  class Track;
}

class HalCbmHbtTrack : public Hal::ExpTrack {
  TVector3 fPosAt[9];
  TVector3 fMomAt[9];
  TVector3 fPosAtCustom;
  TVector3 fMomAtCustom;
  CbmHelix fHelix;
  Double_t fR;

 public:
  HalCbmHbtTrack();
  inline const TVector3& GetPosAtMiddle() const { return fPosAt[3]; };
  inline const TVector3& GetPosAtStsEntrance() const { return fPosAt[1]; };
  inline const TVector3& GetPosAtStsExit() const { return fPosAt[8]; };
  inline const TVector3& GetPosAtCustom() const { return fPosAtCustom; }
  inline const TVector3& GetPosAtPlane(Int_t plane) const { return fPosAt[plane]; }
  inline const TVector3& GetMomAtMiddle() const { return fMomAt[3]; };
  inline const TVector3& GetMomAtStsEntrance() const { return fMomAt[1]; };
  inline const TVector3& GetMomAtStsExit() const { return fMomAt[8]; };
  inline const TVector3& GetMomAtCustom() const { return fMomAtCustom; }
  inline const TVector3& GetMomAtPlane(Int_t plane) const { return fMomAt[plane]; }

  CbmHelix& GetHelix() { return fHelix; };
  void CalculateAtR(Double_t R);
  virtual void CopyData(Hal::Track* other);
  HalCbmHbtTrack(const HalCbmHbtTrack& other);
  HalCbmHbtTrack& operator=(const HalCbmHbtTrack& other);
  virtual ~HalCbmHbtTrack();
  ClassDef(HalCbmHbtTrack, 1)
};

#endif /* CBMHBTTRACK_H_ */
