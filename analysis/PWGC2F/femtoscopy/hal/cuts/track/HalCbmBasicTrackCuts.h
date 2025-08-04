/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMBASICTRACKCUT_H_
#define CBMBASICTRACKCUT_H_

#include <Rtypes.h>
#include <RtypesCore.h>

#include <Hal/CutMonitorRequest.h>
#include <Hal/CutsAndMonitors.h>

namespace Hal
{
  class TrackPCut;
  class TrackChargeCut;
  class TrackChi2Cut;
  class TrackPtCut;
  class TrackEtaCut;
  class TrackDCACut;
}  // namespace Hal

class HalCbmTofCut;
class HalCbmHalCbmNHitsCut;
class HalCbmNHitsCut;

/**collection of basic cuts and cut monitors for track in AnaTree format **/

class HalCbmBasicTrackCuts : public Hal::CutsAndMonitors {

  Hal::TrackChargeCut* GetChargeCut() const;
  HalCbmNHitsCut* GetNHitsCut() const;
  Hal::TrackChi2Cut* GetChi2Cut() const;
  Hal::TrackPCut* GetPCut() const;
  Hal::TrackPtCut* GetPtCut() const;
  Hal::TrackEtaCut* GetEtaCut() const;
  Hal::TrackDCACut* GetDCACut() const;
  Hal::CutMonAxisConf fKinPt;
  Hal::CutMonAxisConf fKinEta;
  Hal::CutMonAxisConf fTofP;
  Hal::CutMonAxisConf fTofM2;
  Hal::CutMonAxisConf fHits;
  Hal::CutMonAxisConf fHitsSts;
  Hal::CutMonAxisConf fChi2;
  Hal::CutMonAxisConf fDCAxy;
  Hal::CutMonAxisConf fDCAz;

 protected:
  virtual void AddAllCutMonitorRequests(Option_t* opt);

 public:
  HalCbmBasicTrackCuts();
  void SetCharge(Int_t charge);
  void SetChi2(Double_t min, Double_t max);
  void SetNHits(Int_t min, Int_t max);
  void SetNMvdHits(Int_t min, Int_t max);
  void SetNStsHits(Int_t min, Int_t max);
  void SetNTrdHits(Int_t min, Int_t max);
  void SetPt(Double_t min, Double_t max);
  void SetEta(Double_t min, Double_t max);
  void AcceptOnlyWithTofHit(Bool_t val);
  void SetM2(Double_t min, Double_t max);
  void SetDCAXY(Double_t min, Double_t max);
  void SetDCAZ(Double_t min, Double_t max);
  void SetTofMonitorPAxis(Int_t nbins, Double_t min, Double_t max);
  void SetTofMonitorM2Axis(Int_t nbins, Double_t min, Double_t max);
  void SetPtEtaMonitorPtAxis(Int_t nbins, Double_t min, Double_t max);
  void SetPtEtaMonitorEtaAxis(Int_t nbins, Double_t min, Double_t max);
  void SetNHitsMonitorAxis(Int_t nbins, Double_t min, Double_t max);
  void SetChi2MonitorAxis(Int_t nbins, Double_t min, Double_t max);
  void SetDCAMonitorZAxis(Int_t nbins, Double_t min, Double_t max);
  void SetDCAMonitorXYAxis(Int_t nbins, Double_t min, Double_t max);
  HalCbmTofCut* GetTofCut() const;
  virtual ~HalCbmBasicTrackCuts();
  ClassDef(HalCbmBasicTrackCuts, 1)
};
#endif /* CBMBASICTRACKCUT_H_ */
