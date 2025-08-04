/* Copyright (C) 2020-2021 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Dominik Smith [committer] */

/// \file   CbmMuchTransportQa.cxx
/// \brief  Implementation of the CbmMuchTransportQa class
/// \author Sergey Gorbunov <se.gorbunov@gsi.de>
/// \author Eugeny Kryshen
/// \author Vikas Singhal
/// \author Ekata Nandy
/// \author Dominik Smith
/// \date   21.10.2020

#include "CbmMuchTransportQa.h"

#include "CbmMCDataArray.h"
#include "CbmMCDataManager.h"
#include "CbmMCTrack.h"
#include "CbmMuchAddress.h"
#include "CbmMuchGeoScheme.h"
#include "CbmMuchPoint.h"
#include "CbmMuchStation.h"
#include "CbmQaCanvas.h"
#include "CbmQaPie.h"
#include "CbmTimeSlice.h"

#include <FairRootManager.h>
#include <FairSink.h>
#include <FairTask.h>
#include <Logger.h>

#include "TClonesArray.h"
#include "TDatabasePDG.h"
#include "TH1.h"
#include "TH2.h"
#include "TLegend.h"
#include "TStyle.h"
#include <TAxis.h>
#include <TDirectory.h>
#include <TMath.h>
#include <TParameter.h>
#include <TString.h>
#include <TVector3.h>

#include <vector>

#define BINS_STA fNstations, 0, fNstations

ClassImp(CbmMuchTransportQa);

// -------------------------------------------------------------------------
CbmMuchTransportQa::CbmMuchTransportQa(const char* name, Int_t verbose)
  : FairTask(name, verbose)
  , fOutFolder("MuchTransportQA", "Much Transport QA")
  , fNevents("nEvents", 0)
  , fvUsNtra()
  , fvMcPointXY()
  , fvMcPointPhiZ()
  , fvMcPointRZ()
  , fvFraction()
  , fvMcPointPRatio()
  , fvMcPointPrimRatio()
{
}

// -------------------------------------------------------------------------
CbmMuchTransportQa::~CbmMuchTransportQa() { DeInit(); }

// -------------------------------------------------------------------------
void CbmMuchTransportQa::DeInit()
{

  fPoints   = nullptr;
  fMcTracks = nullptr;
  fOutFolder.Clear();
  histFolder = nullptr;
  fNevents.SetVal(0);

  SafeDelete(fhUsNtraAll);
  SafeDelete(fhUsNtraPrim);
  SafeDelete(fhUsNtraSec);
  SafeDelete(fhUsNtraPr);
  SafeDelete(fhUsNtraPi);
  SafeDelete(fhUsNtraEl);
  SafeDelete(fhUsNtraMu);
  SafeDelete(fhUsNtraKa);
  fvUsNtra.clear();

  for (uint i = 0; i < fvMcPointXY.size(); i++) {
    SafeDelete(fvMcPointXY[i]);
  }
  for (uint i = 0; i < fvMcPointPhiZ.size(); i++) {
    SafeDelete(fvMcPointPhiZ[i]);
  }
  for (uint i = 0; i < fvMcPointRZ.size(); i++) {
    SafeDelete(fvMcPointRZ[i]);
  }
  fvMcPointXY.clear();
  fvMcPointPhiZ.clear();
  fvMcPointRZ.clear();

  SafeDelete(fhNtracks);
  SafeDelete(fhFractionPrim);
  SafeDelete(fhFractionSec);
  SafeDelete(fhFractionPr);
  SafeDelete(fhFractionPi);
  SafeDelete(fhFractionEl);
  SafeDelete(fhFractionMu);
  SafeDelete(fhFractionKa);
  fvFraction.clear();

  for (uint i = 0; i < fvMcPointPRatio.size(); i++) {
    SafeDelete(fvMcPointPRatio[i]);
  }
  for (uint i = 0; i < fvMcPointPrimRatio.size(); i++) {
    SafeDelete(fvMcPointPrimRatio[i]);
  }
  fvMcPointPRatio.clear();
  fvMcPointPrimRatio.clear();

  SafeDelete(fCanvStationXY);
  SafeDelete(fCanvStationPhiZ);
  SafeDelete(fCanvStationRZ);
  SafeDelete(fCanvNtra);
  SafeDelete(fCanvStationPRatio);
  SafeDelete(fCanvStationPrimRatio);
  fNstations = 0;
}

// -------------------------------------------------------------------------
InitStatus CbmMuchTransportQa::Init()
{
  DeInit();

  TDirectory* oldDirectory = gDirectory;
  fManager                 = FairRootManager::Instance();

  if (!fManager) {
    LOG(error) << "No FairRootManager found";
    return kERROR;
  }

  fMcManager = dynamic_cast<CbmMCDataManager*>(fManager->GetObject("MCDataManager"));

  fTimeSlice = (CbmTimeSlice*) fManager->GetObject("TimeSlice.");
  if (!fTimeSlice) { LOG(error) << GetName() << ": No time slice found"; }

  if (fMcManager) {
    // Get MCTrack array
    fMcTracks = fMcManager->InitBranch("MCTrack");

    // Get StsPoint array
    fPoints = fMcManager->InitBranch("MuchPoint");
  }

  fNstations = CbmMuchGeoScheme::Instance()->GetNStations();
  histFolder = fOutFolder.AddFolder("hist", "Histogramms");

  if (!fMcTracks) {
    LOG(error) << "No MC tracks found";
    return kERROR;
  }
  if (!fPoints) {
    LOG(error) << "No MC points found";
    return kERROR;
  }
  if (!CbmMuchGeoScheme::Instance()) {
    LOG(fatal) << "No CbmMuchGeoScheme found";
    return kFATAL;
  }
  if (fNstations == 0) {
    LOG(error) << "CbmMuchGeoScheme is not initialized";
    return kERROR;
  }
  for (Int_t i = 0; i < fNstations; i++) {
    CbmMuchStation* station = CbmMuchGeoScheme::Instance()->GetStation(i);
    if (!station) {
      LOG(fatal) << "Much station " << i << " doesn't exist";
      return kFATAL;
    }
  }
  fNevents.SetVal(0);
  histFolder->Add(&fNevents);

  InitCountingHistos();
  InitFractionHistos();
  Init2dSpatialDistributionHistos();
  InitRatioPieCharts();
  InitCanvases();

  gDirectory = oldDirectory;
  return kSUCCESS;
}

// -------------------------------------------------------------------------
void CbmMuchTransportQa::InitCountingHistos()
{

  fvUsNtra.clear();
  std::vector<TH1F*>& v = fvUsNtra;
  v.push_back(fhUsNtraAll = new TH1F("hUsNtraAll", "N tracks", BINS_STA));
  v.push_back(fhUsNtraPrim = new TH1F("hUsNtraPrim", "N primary tracks", BINS_STA));
  v.push_back(fhUsNtraSec = new TH1F("hUsNtraSec", "N secondary tracks", BINS_STA));
  v.push_back(fhUsNtraPr = new TH1F("hUsNtraPr", "N protons", BINS_STA));
  v.push_back(fhUsNtraPi = new TH1F("hUsNtraPi", "N pions", BINS_STA));
  v.push_back(fhUsNtraEl = new TH1F("hUsNtraEl", "N electrons", BINS_STA));
  v.push_back(fhUsNtraMu = new TH1F("hUsNtraMu", "N muons", BINS_STA));
  v.push_back(fhUsNtraKa = new TH1F("hUsNtraKa", "N kaons", BINS_STA));
  for (uint i = 0; i < fvUsNtra.size(); i++) {
    TH1F* h = fvUsNtra[i];
    h->SetStats(0);
    h->GetXaxis()->SetTitle("Station");
    histFolder->Add(h);
  }
}

// -------------------------------------------------------------------------
void CbmMuchTransportQa::InitFractionHistos()
{

  fvFraction.clear();
  std::vector<TH1F*>& v = fvFraction;
  v.push_back(fhNtracks = new TH1F("hNtracks", "N tracks per event", BINS_STA));
  v.push_back(fhFractionPrim = new TH1F("hFractionPrim", "Fraction of primary tracks", BINS_STA));
  v.push_back(fhFractionSec = new TH1F("hFractionSec", "Fraction of secondary tracks", BINS_STA));
  v.push_back(fhFractionPr = new TH1F("hFractionPr", "Fraction of protons", BINS_STA));
  v.push_back(fhFractionPi = new TH1F("hFractionPi", "Fraction of pions", BINS_STA));
  v.push_back(fhFractionEl = new TH1F("hFractionEl", "Fraction of electrons", BINS_STA));
  v.push_back(fhFractionMu = new TH1F("hFractionMu", "Fraction of muons", BINS_STA));
  v.push_back(fhFractionKa = new TH1F("hFractionKa", "Fraction of kaons", BINS_STA));

  for (uint i = 0; i < fvFraction.size(); i++) {
    TH1F* h = fvFraction[i];
    h->SetStats(0);
    h->GetXaxis()->SetTitle("Station");
    if (i == 0) { h->GetYaxis()->SetTitle("N tracks"); }
    else {
      h->GetYaxis()->SetTitle("%");
    }
    histFolder->Add(h);
  }
}

// -------------------------------------------------------------------------
void CbmMuchTransportQa::Init2dSpatialDistributionHistos()
{

  fvMcPointXY.resize(fNstations);
  fvMcPointPhiZ.resize(fNstations);
  fvMcPointRZ.resize(fNstations);
  gStyle->SetOptStat(0);

  for (Int_t i = 0; i < fNstations; i++) {
    CbmMuchStation* station = CbmMuchGeoScheme::Instance()->GetStation(i);
    Double_t rMax           = station->GetRmax();
    Double_t rMin           = station->GetRmin();

    fvMcPointXY[i]   = new TH2F(Form("hMcPointXY%i", i + 1), Form("MC point XY : Station %i; X; Y", i + 1), 100,
                              -1.2 * rMax, 1.2 * rMax, 100, -1.2 * rMax, 1.2 * rMax);
    fvMcPointPhiZ[i] = new TH2F(Form("hMcPointPhiZ%i", i + 1), Form("MC point Phi vs Z : Station %i; Z; Phi", i + 1),
                                100, station->GetZ() - station->GetTubeDz() - 5.,
                                station->GetZ() + station->GetTubeDz() + 5., 100, -200., 200.);

    float dR       = rMax - rMin;
    fvMcPointRZ[i] = new TH2F(Form("hMcPointRZ%i", i + 1), Form("MC point R vs Z : Station %i; Z; R", i + 1), 100,
                              station->GetZ() - station->GetTubeDz() - 5., station->GetZ() + station->GetTubeDz() + 5.,
                              100, rMin - 0.1 * dR, rMax + 0.1 * dR);
    histFolder->Add(fvMcPointXY[i]);
    histFolder->Add(fvMcPointPhiZ[i]);
    histFolder->Add(fvMcPointRZ[i]);
  }
}

// -------------------------------------------------------------------------
void CbmMuchTransportQa::InitRatioPieCharts()
{

  fvMcPointPRatio.resize(fNstations);
  fvMcPointPrimRatio.resize(fNstations);
  for (Int_t i = 0; i < fNstations; i++) {
    fvMcPointPRatio[i] =
      new CbmQaPie(Form("fvMcPointPRatio%i", i + 1), Form("McPoint Particle Ratios: Station %i", i + 1), 5);

    fvMcPointPrimRatio[i] =
      new CbmQaPie(Form("fvMcPointPrimRatio%i", i + 1), Form("McPoint Primary/Secondary Track: Station %i", i + 1), 2);

    histFolder->Add(fvMcPointPRatio[i]);
    histFolder->Add(fvMcPointPrimRatio[i]);
  }
}

// -------------------------------------------------------------------------
void CbmMuchTransportQa::InitCanvases()
{

  fCanvStationXY = new CbmQaCanvas("cMcPointXY", "Much: MC point XY", 2 * 400, 2 * 400);
  fCanvStationXY->Divide2D(fNstations);

  fCanvStationPhiZ = new CbmQaCanvas("cMcPointPhiZ", "Much: MC point Phi vs Z", 2 * 800, 2 * 400);
  fCanvStationPhiZ->Divide2D(fNstations);

  fCanvStationRZ = new CbmQaCanvas("cMcPointRZ", "Much: MC point R vs Z", 2 * 800, 2 * 400);
  fCanvStationRZ->Divide2D(fNstations);

  fCanvNtra = new CbmQaCanvas("cNparticles", "Much: Particle counts per event", 2 * 800, 2 * 400);
  fCanvNtra->Divide2D(8);

  fCanvStationPRatio = new CbmQaCanvas("cMcPointPRatios", "Much: MC particle ratios", 2 * 400, 2 * 400);
  fCanvStationPRatio->Divide2D(fNstations);

  fCanvStationPrimRatio =
    new CbmQaCanvas("cMcPointPrimRatios", "Much: MC primary/secondary track ratios", 2 * 400, 2 * 400);
  fCanvStationPrimRatio->Divide2D(fNstations);

  fOutFolder.Add(fCanvStationXY);
  fOutFolder.Add(fCanvStationPhiZ);
  fOutFolder.Add(fCanvStationRZ);
  fOutFolder.Add(fCanvNtra);
  fOutFolder.Add(fCanvStationPRatio);
  fOutFolder.Add(fCanvStationPrimRatio);
}

// -------------------------------------------------------------------------
InitStatus CbmMuchTransportQa::ReInit()
{
  DeInit();
  return Init();
}

// -------------------------------------------------------------------------
void CbmMuchTransportQa::SetParContainers()
{
  // Get run and runtime database

  // The code currently does not work,
  // CbmMuchGeoScheme::Instance() must be initialised outside.
  // - Sergey

  // FairRuntimeDb* db = FairRuntimeDb::instance();
  // if ( ! db ) Fatal("SetParContainers", "No runtime database");
  // Get MUCH geometry parameter container
  // CbmGeoMuchPar *fGeoPar = (CbmGeoMuchPar*)
  // db->getContainer("CbmGeoMuchPar");  TObjArray *stations =
  // fGeoPar->GetStations();
  //  TString geoTag;
  // CbmSetup::Instance()->GetGeoTag(ECbmModuleId::kMuch, geoTag);
  // bool mcbmFlag = geoTag.Contains("mcbm", TString::kIgnoreCase);
  // CbmMuchGeoScheme::Instance()->Init(stations, mcbmFlag);
}

// -------------------------------------------------------------------------
void CbmMuchTransportQa::Exec(Option_t*)
{

  fNevents.SetVal(fNevents.GetVal() + 1);
  LOG(debug) << "Event: " << fNevents.GetVal();

  const CbmMatch& sliceMatch = fTimeSlice->GetMatch();

  for (int iLink = 0; iLink < sliceMatch.GetNofLinks(); iLink++) {
    const CbmLink& eventLink = sliceMatch.GetLink(iLink);
    int nMuchPoints          = fPoints->Size(eventLink);
    int nMcTracks            = fMcTracks->Size(eventLink);

    // bitmask tells which stations were crossed by mc track
    std::vector<UInt_t> trackStaCross(nMcTracks, 0);

    for (Int_t ip = 0; ip < nMuchPoints; ip++) {
      CbmLink pointLink = eventLink;
      pointLink.SetIndex(ip);
      CbmMuchPoint* point = (CbmMuchPoint*) fPoints->Get(pointLink);
      Int_t stId          = CbmMuchAddress::GetStationIndex(point->GetDetectorID());
      UInt_t stMask       = (1 << stId);
      Int_t trackId       = point->GetTrackID();
      if (!point) {
        LOG(fatal) << "Much point " << ip << " doesn't exist";
        break;
      }  // Check if the point corresponds to a certain  MC Track
      if (trackId < 0 || trackId >= nMcTracks) {
        LOG(fatal) << "Much point " << ip << ": trackId " << trackId << " doesn't belong to [0," << nMcTracks - 1
                   << "]";
        break;
      }
      CbmLink trackLink = eventLink;
      trackLink.SetIndex(trackId);
      CbmMCTrack* mcTrack = (CbmMCTrack*) fMcTracks->Get(trackLink);
      if (!mcTrack) {
        LOG(fatal) << "MC track " << trackId << " doesn't exist";
        break;
      }

      Int_t motherId = mcTrack->GetMotherId();
      Int_t pdgCode  = mcTrack->GetPdgCode();
      if (pdgCode == 22 ||  // photons
          pdgCode == 2112)  // neutrons
      {
        continue;
      }

      if (!(trackStaCross[trackId] & stMask)) { FillCountingHistos(stId, motherId, pdgCode); }
      trackStaCross[trackId] |= stMask;
      Fill2dSpatialDistributionHistos(point, stId);
    }
  }
}

// -------------------------------------------------------------------------
void CbmMuchTransportQa::FillCountingHistos(Int_t stId, Int_t motherId, Int_t pdgCode)
{
  fhUsNtraAll->Fill(stId);
  if (motherId == -1) { fhUsNtraPrim->Fill(stId); }
  else {
    fhUsNtraSec->Fill(stId);
  }
  switch (abs(pdgCode)) {
    case 2212:  // proton
      fhUsNtraPr->Fill(stId);
      break;
    case 211:  // pion
      fhUsNtraPi->Fill(stId);
      break;
    case 11:  // electron
      fhUsNtraEl->Fill(stId);
      break;
    case 13:  // muon
      fhUsNtraMu->Fill(stId);
      break;
    case 321:  // kaon
      fhUsNtraKa->Fill(stId);
      break;
  }
}

// -------------------------------------------------------------------------
void CbmMuchTransportQa::Fill2dSpatialDistributionHistos(CbmMuchPoint* point, Int_t stId)
{

  TVector3 v1;  // in  position of the track
  TVector3 v2;  // out position of the track
  point->PositionIn(v1);
  point->PositionOut(v2);

  fvMcPointXY[stId]->Fill(v1.X(), v1.Y());
  fvMcPointXY[stId]->Fill(v2.X(), v2.Y());
  fvMcPointPhiZ[stId]->Fill(v1.Z(), v1.Phi() * TMath::RadToDeg());
  fvMcPointPhiZ[stId]->Fill(v2.Z(), v2.Phi() * TMath::RadToDeg());
  fvMcPointRZ[stId]->Fill(v1.Z(), v1.Perp());
  fvMcPointRZ[stId]->Fill(v2.Z(), v2.Perp());
}

// -------------------------------------------------------------------------
TFolder& CbmMuchTransportQa::GetQa()
{

  TDirectory* oldDirectory = gDirectory;
  fhNtracks->Reset();
  fhNtracks->Add(fhUsNtraAll, 1. / fNevents.GetVal());

  std::vector<Double_t> errors(fNstations, 0.);
  fhUsNtraAll->SetError(errors.data());

  for (uint i = 1; i < fvFraction.size(); i++) {
    fvFraction[i]->Divide(fvUsNtra[i], fhUsNtraAll);
    fvFraction[i]->Scale(100.);
  }
  MakePRatioPieCharts();
  MakePrimRatioPieCharts();
  DrawCanvases();

  gDirectory = oldDirectory;
  return fOutFolder;
}

// -------------------------------------------------------------------------
void CbmMuchTransportQa::DrawCanvases()
{

  for (Int_t i = 0; i < fNstations; i++) {
    fCanvStationXY->cd(i + 1);
    fvMcPointXY[i]->DrawCopy("colz", "");

    fCanvStationPhiZ->cd(i + 1);
    fvMcPointPhiZ[i]->DrawCopy("colz", "");

    fCanvStationRZ->cd(i + 1);
    fvMcPointRZ[i]->DrawCopy("colz", "");

    fCanvStationPRatio->cd(i + 1);
    fvMcPointPRatio[i]->DrawClone("nol <");

    TLegend* PRatioPieLeg = fvMcPointPRatio[i]->MakeLegend();
    PRatioPieLeg->SetY1(.56);
    PRatioPieLeg->SetY2(.86);

    fCanvStationPrimRatio->cd(i + 1);
    fvMcPointPrimRatio[i]->DrawClone("nol <");

    TLegend* PrimRatioPieLeg = fvMcPointPrimRatio[i]->MakeLegend();
    PrimRatioPieLeg->SetY1(.71);
    PrimRatioPieLeg->SetY2(.86);
    PrimRatioPieLeg->SetX1(.40);
    PrimRatioPieLeg->SetX2(.90);
  }

  double scale = (fNevents.GetVal() > 0) ? 1. / fNevents.GetVal() : 0;
  int i        = 1;

  fCanvNtra->cd(i++);
  fhNtracks->DrawCopy("colz", "");

  fCanvNtra->cd(i++);
  fhUsNtraPrim->DrawCopy("colz", "")->Scale(scale);

  fCanvNtra->cd(i++);
  fhUsNtraSec->DrawCopy("colz", "")->Scale(scale);

  fCanvNtra->cd(i++);
  fhUsNtraPr->DrawCopy("colz", "")->Scale(scale);

  fCanvNtra->cd(i++);
  fhUsNtraPi->DrawCopy("colz", "")->Scale(scale);

  fCanvNtra->cd(i++);
  fhUsNtraEl->DrawCopy("colz", "")->Scale(scale);

  fCanvNtra->cd(i++);
  fhUsNtraMu->DrawCopy("colz", "")->Scale(scale);

  fCanvNtra->cd(i++);
  fhUsNtraKa->DrawCopy("colz", "")->Scale(scale);
}

// -------------------------------------------------------------------------
void CbmMuchTransportQa::MakePRatioPieCharts()
{

  for (Int_t i = 0; i < fNstations; i++) {
    Double_t PRatios[]    = {fhFractionEl->GetBinContent(i + 1), fhFractionPr->GetBinContent(i + 1),
                          fhFractionPi->GetBinContent(i + 1), fhFractionMu->GetBinContent(i + 1),
                          fhFractionKa->GetBinContent(i + 1)};
    Int_t PRatiosColors[] = {4, 3, 2, 5, 6};

    fvMcPointPRatio[i]->SetEntryVal(0, PRatios[0]);
    fvMcPointPRatio[i]->SetEntryVal(1, PRatios[1]);
    fvMcPointPRatio[i]->SetEntryVal(2, PRatios[2]);
    fvMcPointPRatio[i]->SetEntryVal(3, PRatios[3]);
    fvMcPointPRatio[i]->SetEntryVal(4, PRatios[4]);
    fvMcPointPRatio[i]->SetEntryFillColor(0, PRatiosColors[0]);
    fvMcPointPRatio[i]->SetEntryFillColor(1, PRatiosColors[1]);
    fvMcPointPRatio[i]->SetEntryFillColor(2, PRatiosColors[2]);
    fvMcPointPRatio[i]->SetEntryFillColor(3, PRatiosColors[3]);
    fvMcPointPRatio[i]->SetEntryFillColor(4, PRatiosColors[4]);
    fvMcPointPRatio[i]->GetSlice(0)->SetTitle(Form("e:   %.1f %%", PRatios[0]));
    fvMcPointPRatio[i]->GetSlice(1)->SetTitle(Form("p:   %.1f %%", PRatios[1]));
    fvMcPointPRatio[i]->GetSlice(2)->SetTitle(Form("#pi:   %.1f %%", PRatios[2]));
    fvMcPointPRatio[i]->GetSlice(3)->SetTitle(Form("#mu:   %.1f %%", PRatios[3]));
    fvMcPointPRatio[i]->GetSlice(4)->SetTitle(Form("K:   %.1f %%", PRatios[4]));
    fvMcPointPRatio[i]->SetRadius(.33);
    fvMcPointPRatio[i]->SetLabelsOffset(-.1);
    fvMcPointPRatio[i]->SetLabelFormat("");
  }
}

void CbmMuchTransportQa::MakePrimRatioPieCharts()
{

  for (Int_t i = 0; i < fNstations; i++) {
    Double_t PrimRatios[]    = {fhFractionPrim->GetBinContent(i + 1), fhFractionSec->GetBinContent(i + 1)};
    Int_t PrimRatiosColors[] = {6, 4};

    fvMcPointPrimRatio[i]->SetEntryVal(0, PrimRatios[0]);
    fvMcPointPrimRatio[i]->SetEntryVal(1, PrimRatios[1]);
    fvMcPointPrimRatio[i]->SetEntryFillColor(0, PrimRatiosColors[0]);
    fvMcPointPrimRatio[i]->SetEntryFillColor(1, PrimRatiosColors[1]);
    fvMcPointPrimRatio[i]->GetSlice(0)->SetTitle(Form("Primary:   %.1f %%", PrimRatios[0]));
    fvMcPointPrimRatio[i]->GetSlice(1)->SetTitle(Form("Secondary: %.1f %%", PrimRatios[1]));
    fvMcPointPrimRatio[i]->SetRadius(.33);
    fvMcPointPrimRatio[i]->SetLabelsOffset(-.1);
    fvMcPointPrimRatio[i]->SetLabelFormat("");
  }
}

// -------------------------------------------------------------------------
void CbmMuchTransportQa::Finish()
{

  if (!FairRootManager::Instance() || !FairRootManager::Instance()->GetSink()) {
    LOG(error) << "No sink found";
    return;
  }
  FairSink* sink = FairRootManager::Instance()->GetSink();
  sink->WriteObject(&GetQa(), nullptr);
}
