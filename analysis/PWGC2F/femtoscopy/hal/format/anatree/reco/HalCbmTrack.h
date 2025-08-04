/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICACBMATRECOTRACK_H_
#define NICACBMATRECOTRACK_H_

#include "CbmGlobalTrack.h"
#include "CbmHelix.h"
#include "HalCbmEventInterface.h"

#include <AnalysisTree/Detector.hpp>
#include <AnalysisTree/Particle.hpp>

#include <Hal/ExpTrack.h>
#include <Hal/ToFTrack.h>

class CbmHelix;
namespace Hal
{
  class Track;
}
class HalCbmTrackInterface;


class HalCbmTrack : public Hal::ExpTrack {
  Hal::ToFTrack* fTofTrack;
  CbmHelix fHelix;
  Float_t fChi2Vertex;
  Int_t fMvdHits;
  Int_t fStsHits;
  Int_t fTrdHits;

 public:
  HalCbmTrack();
  HalCbmTrack(const HalCbmTrack& other);
  HalCbmTrack& operator=(const HalCbmTrack& other);
  Hal::ToFTrack* GetTofTrack() { return fTofTrack; };
  CbmHelix& GetHelix() { return fHelix; };
  Double_t GetVertexChi2() const { return fChi2Vertex; };
  Int_t GetNMvdHits() const { return fMvdHits; };
  Int_t GetNStsHits() const { return fStsHits; }
  Int_t GetNTrdHits() const { return fTrdHits; }
  Int_t GetNHits() const { return GetNMvdHits() + GetNStsHits() + GetNTrdHits(); };
  void SetVertexChi2(Double_t v) { fChi2Vertex = v; };
  void SetNMvdHits(Int_t h) { fMvdHits = h; };
  void SetNStsHits(Int_t h) { fStsHits = h; };
  void SetNTrdHits(Int_t h) { fTrdHits = h; };
  void BuildHelix();
  virtual void CopyData(Hal::Track* other);
  virtual Hal::DetectorTrack* GetDetTrack(const UInt_t detID) const;
  virtual ~HalCbmTrack();
  ClassDef(HalCbmTrack, 1)
};
#endif /* NICACBMATRECOTRACK_H_ */
