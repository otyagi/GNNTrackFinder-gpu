/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmBasicTrackCuts.h"

#include "HalCbmNHitsCut.h"
#include "HalCbmTofCut.h"

#include <TString.h>

#include <Hal/Cut.h>
#include <Hal/CutMonitorRequest.h>
#include <Hal/TrackChargeCut.h>
#include <Hal/TrackChi2Cut.h>
#include <Hal/TrackDCACut.h>
#include <Hal/TrackEtaCut.h>
#include <Hal/TrackPCut.h>
#include <Hal/TrackPtCut.h>


HalCbmBasicTrackCuts::HalCbmBasicTrackCuts()
  : fKinPt("Hal::TrackPtCut", 0, 100, 0, 4)
  , fKinEta("Hal::TrackEtaCut", 0, 200, -2, 4)
  , fTofP("Hal::TrackPCut", 0, 200, 0, 2)
  , fTofM2("HalCbmTofCut", 0, 100, -0.1, 1.2)
  , fHits("HalCbmNHitsCut", 0, 12, 0.5, 12.5)
  , fHitsSts("HalCbmNHitsCut", 2, 10, -0.5, 9.5)
  , fChi2("Hal::TrackChi2Cut", 0, 100, 0, 25)
  , fDCAxy("Hal::TrackDCACut", 1, 100, 0, 10)
  , fDCAz("Hal::TrackDCACut", 2, 100, -5, 5)
{
  AddCut(Hal::TrackChargeCut());  //0
  AddCut(HalCbmNHitsCut());       //1
  AddCut(Hal::TrackChi2Cut());    //2
  AddCut(Hal::TrackPCut());       //3
  AddCut(Hal::TrackPtCut());      //4
  AddCut(Hal::TrackEtaCut());     //5
  AddCut(Hal::TrackDCACut());     //6
  AddCut(HalCbmTofCut());         //7
}

Hal::TrackChargeCut* HalCbmBasicTrackCuts::GetChargeCut() const { return (Hal::TrackChargeCut*) this->CutAt(0); }

HalCbmNHitsCut* HalCbmBasicTrackCuts::GetNHitsCut() const { return (HalCbmNHitsCut*) this->CutAt(1); }

Hal::TrackChi2Cut* HalCbmBasicTrackCuts::GetChi2Cut() const { return (Hal::TrackChi2Cut*) this->CutAt(2); }

Hal::TrackPCut* HalCbmBasicTrackCuts::GetPCut() const { return (Hal::TrackPCut*) this->CutAt(3); }

Hal::TrackPtCut* HalCbmBasicTrackCuts::GetPtCut() const { return (Hal::TrackPtCut*) this->CutAt(4); }

Hal::TrackEtaCut* HalCbmBasicTrackCuts::GetEtaCut() const { return (Hal::TrackEtaCut*) this->CutAt(5); }

Hal::TrackDCACut* HalCbmBasicTrackCuts::GetDCACut() const { return (Hal::TrackDCACut*) this->CutAt(6); }

void HalCbmBasicTrackCuts::AddAllCutMonitorRequests(Option_t* opt)
{
  //Hal::HistogramAxisConf requestHits(bins, min, max);
  TString option = opt;
  if (Hal::Std::FindParam(option, "all")) {
    AddCutMonitorRequest(fHits);
    AddCutMonitorRequest(fHitsSts);
    AddCutMonitorRequest(fChi2);
    AddCutMonitorRequest(fKinEta, fKinPt);
    AddCutMonitorRequest(fTofP, fTofM2);
    AddCutMonitorRequest(fDCAz, fDCAxy);
    return;
  }
  if (Hal::Std::FindParam(option, "chi2")) AddCutMonitorRequest(fChi2);
  if (Hal::Std::FindParam(option, "hits")) AddCutMonitorRequest(fHits);
  if (Hal::Std::FindParam(option, "hits_sts")) AddCutMonitorRequest(fHitsSts);
  if (Hal::Std::FindParam(option, "tof")) AddCutMonitorRequest(fTofP, fTofM2);
  if (Hal::Std::FindParam(option, "dca")) AddCutMonitorRequest(fDCAz, fDCAxy);
}

void HalCbmBasicTrackCuts::SetCharge(Int_t charge) { GetChargeCut()->SetMinAndMax(charge); }

void HalCbmBasicTrackCuts::SetNHits(Int_t min, Int_t max) { GetNHitsCut()->SetMinMax(min, max); }

void HalCbmBasicTrackCuts::SetNMvdHits(Int_t min, Int_t max) { GetNHitsCut()->SetMinMax(min, max, 1); }

void HalCbmBasicTrackCuts::SetNStsHits(Int_t min, Int_t max) { GetNHitsCut()->SetMinMax(min, max, 2); }

void HalCbmBasicTrackCuts::SetNTrdHits(Int_t min, Int_t max) { GetNHitsCut()->SetMinMax(min, max, 3); }

void HalCbmBasicTrackCuts::SetPt(Double_t min, Double_t max) { GetPtCut()->SetMinMax(min, max); }

void HalCbmBasicTrackCuts::SetEta(Double_t min, Double_t max) { GetEtaCut()->SetMinMax(min, max); }

void HalCbmBasicTrackCuts::SetM2(Double_t min, Double_t max) { GetTofCut()->SetMinMax(min, max); }

void HalCbmBasicTrackCuts::SetDCAXY(Double_t min, Double_t max)
{
  GetDCACut()->SetMinMax(min, max, GetDCACut()->DCAxy());
}

void HalCbmBasicTrackCuts::SetDCAZ(Double_t min, Double_t max)
{
  GetDCACut()->SetMinMax(min, max, GetDCACut()->DCAz());
}

void HalCbmBasicTrackCuts::SetTofMonitorPAxis(Int_t nbins, Double_t min, Double_t max)
{
  fTofP.SetAxis(nbins, min, max);
}

void HalCbmBasicTrackCuts::SetTofMonitorM2Axis(Int_t nbins, Double_t min, Double_t max)
{
  fTofM2.SetAxis(nbins, min, max);
}

void HalCbmBasicTrackCuts::SetPtEtaMonitorPtAxis(Int_t nbins, Double_t min, Double_t max)
{
  fKinPt.SetAxis(nbins, min, max);
}

void HalCbmBasicTrackCuts::SetPtEtaMonitorEtaAxis(Int_t nbins, Double_t min, Double_t max)
{
  fKinEta.SetAxis(nbins, min, max);
}

void HalCbmBasicTrackCuts::SetNHitsMonitorAxis(Int_t nbins, Double_t min, Double_t max)
{
  fHits.SetAxis(nbins, min, max);
}

void HalCbmBasicTrackCuts::SetChi2MonitorAxis(Int_t nbins, Double_t min, Double_t max)
{
  fChi2.SetAxis(nbins, min, max);
}

void HalCbmBasicTrackCuts::SetDCAMonitorZAxis(Int_t nbins, Double_t min, Double_t max)
{
  fDCAz.SetAxis(nbins, min, max);
}

void HalCbmBasicTrackCuts::SetDCAMonitorXYAxis(Int_t nbins, Double_t min, Double_t max)
{
  fDCAxy.SetAxis(nbins, min, max);
}

HalCbmTofCut* HalCbmBasicTrackCuts::GetTofCut() const { return (HalCbmTofCut*) this->CutAt(7); }

void HalCbmBasicTrackCuts::AcceptOnlyWithTofHit(Bool_t val)
{
  if (val)
    GetTofCut()->AcceptTracksOnlyWithToF();
  else
    GetTofCut()->SetMinMax(0, 1, 1);
}

HalCbmBasicTrackCuts::~HalCbmBasicTrackCuts() {}

void HalCbmBasicTrackCuts::SetChi2(Double_t min, Double_t max) { GetChi2Cut()->SetMinMax(min, max); };
