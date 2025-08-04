/* Copyright (C) 2007-2021 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Evgeny Kryshen, Dominik Smith [committer], Sergey Gorbunov */

// -------------------------------------------------------------------------
// -----                   CbmMuchHitProducerQa source file             -----
// -----                   Modified since 21/06/2019 by Ekata Nandy (ekata@vecc.gov.in) -Inclusion of RPC detector type
// -----                   Modified 02/18 by Vikas Singhal             -----
// -----                   Created 16/11/07 by E. Kryshen              -----
// -------------------------------------------------------------------------
//   AUG 18 RELEASE VERSION

#include "CbmMuchHitFinderQa.h"

#include "CbmDefs.h"
#include "CbmDigiManager.h"
#include "CbmLink.h"
#include "CbmMCDataArray.h"
#include "CbmMCDataManager.h"
#include "CbmMCTrack.h"
#include "CbmMatch.h"
#include "CbmMuchAddress.h"
#include "CbmMuchCluster.h"
#include "CbmMuchDigi.h"
#include "CbmMuchGeoScheme.h"
#include "CbmMuchModuleGem.h"
#include "CbmMuchPixelHit.h"
#include "CbmMuchPoint.h"
#include "CbmQaCanvas.h"
#include "CbmTimeSlice.h"
#include "FairRootManager.h"
#include "TArrayI.h"
#include "TF1.h"
#include "TFile.h"
#include "TObjArray.h"
#include "TParameter.h"
#include "TPaveStats.h"
#include "TStyle.h"

#include <FairSink.h>
#include <FairTask.h>
#include <Logger.h>

#include <TClonesArray.h>
#include <TGenericClassInfo.h>
#include <TH1.h>
#include <TMathBase.h>
#include <TString.h>

#include <boost/exception/exception.hpp>
#include <boost/type_index/type_index_facade.hpp>

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

#include <stdio.h>

using std::cout;
using std::endl;
using std::map;
using std::vector;
// -------------------------------------------------------------------------
CbmMuchHitFinderQa::CbmMuchHitFinderQa(const char* name, Int_t verbose)
  : FairTask(name, verbose)
  , fGeoScheme(CbmMuchGeoScheme::Instance())
  , fGeoFileName()
  , fFileName()
  , fVerbose(verbose)
  , fOutFolder("MuchHitFinderQA", "MuchHitFinderQA")
  , fhPointsInCluster()
  , fhDigisInCluster()
  , fhHitsPerCluster()
  , fNevents("nEvents", 0)
  , fSignalPoints("SignalPoints", 0)
  , fSignalHits("SignalHits", 0)
  , fPointsTotal("PointsTotal", 0)
  , fPointsUnderCounted("PointsUnderCounted", 0)
  , fPointsOverCounted("PointsOverCounted", 0)
{
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
CbmMuchHitFinderQa::~CbmMuchHitFinderQa() { DeInit(); }
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
void CbmMuchHitFinderQa::DeInit()
{
  fPoints      = nullptr;
  fDigiManager = nullptr;
  fClusters    = nullptr;
  fHits        = nullptr;
  fMCTracks    = nullptr;
  histFolder   = nullptr;
  fOutFolder.Clear();

  SafeDelete(fhPullX);
  SafeDelete(fhPullY);
  SafeDelete(fhPullT);
  SafeDelete(fhResidualX);
  SafeDelete(fhResidualY);
  SafeDelete(fhResidualT);

  for (uint i = 0; i < fhPointsInCluster.size(); i++) {
    SafeDelete(fhPointsInCluster[i]);
  }
  for (uint i = 0; i < fhDigisInCluster.size(); i++) {
    SafeDelete(fhDigisInCluster[i]);
  }
  for (uint i = 0; i < fhHitsPerCluster.size(); i++) {
    SafeDelete(fhHitsPerCluster[i]);
  }
  fhPointsInCluster.clear();
  fhDigisInCluster.clear();
  fhHitsPerCluster.clear();

  SafeDelete(fCanvPointsInCluster);
  SafeDelete(fCanvDigisInCluster);
  SafeDelete(fCanvHitsPerCluster);
  SafeDelete(fCanvPull);
  SafeDelete(fCanvResidual);

  fVerbose   = 0;
  fFlag      = 0;
  fNstations = 0;

  fNevents.SetVal(0);
  fSignalPoints.SetVal(0);
  fSignalHits.SetVal(0);
  fPointsTotal.SetVal(0);
  fPointsUnderCounted.SetVal(0);
  fPointsOverCounted.SetVal(0);
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
InitStatus CbmMuchHitFinderQa::Init()
{

  DeInit();

  fManager   = FairRootManager::Instance();
  fMcManager = dynamic_cast<CbmMCDataManager*>(fManager->GetObject("MCDataManager"));

  fTimeSlice = (CbmTimeSlice*) fManager->GetObject("TimeSlice.");

  if (fMcManager) {
    fMCTracks = fMcManager->InitBranch("MCTrack");
    fPoints   = fMcManager->InitBranch("MuchPoint");
  }

  fHits     = (TClonesArray*) fManager->GetObject("MuchPixelHit");
  fClusters = (TClonesArray*) fManager->GetObject("MuchCluster");
  // Reading Much Digis from CbmMuchDigiManager which are stored as vector
  fDigiManager = CbmDigiManager::Instance();

  /// Save old global file and folder pointer to avoid messing with FairRoot
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  TFile* f = new TFile(fGeoFileName, "R");
  LOG_IF(fatal, !f) << "Could not open file " << fGeoFileName;

  /// Restore old global file and folder pointer to avoid messing with FairRoot
  gFile      = oldFile;
  gDirectory = oldDir;

  TObjArray* stations = f->Get<TObjArray>("stations");
  LOG_IF(fatal, !stations) << "TObjArray stations not found in file " << fGeoFileName;
  fGeoScheme->Init(stations, fFlag);

  if (!fManager) {
    LOG(fatal) << "Can not find FairRootManager";
    return kFATAL;
  }
  if (!fDigiManager) {
    LOG(fatal) << "Can not find Much digi manager";
    return kFATAL;
  }
  if (!fGeoScheme) {
    LOG(fatal) << "Can not find Much geo scheme";
    return kFATAL;
  }
  if (!fMCTracks) {
    LOG(error) << "No MC tracks found";
    return kERROR;
  }
  if (!fPoints) {
    LOG(error) << "No MC points found";
    return kERROR;
  }
  if (!fHits) {
    LOG(error) << "No hits found";
    return kERROR;
  }
  if (!fClusters) {
    LOG(error) << "No hits found";
    return kERROR;
  }
  histFolder = fOutFolder.AddFolder("hist", "Histogramms");

  fNevents.SetVal(0);
  fSignalPoints.SetVal(0);
  fSignalHits.SetVal(0);
  fPointsTotal.SetVal(0);
  fPointsUnderCounted.SetVal(0);
  fPointsOverCounted.SetVal(0);

  histFolder->Add(&fNevents);
  histFolder->Add(&fSignalPoints);
  histFolder->Add(&fSignalHits);
  histFolder->Add(&fPointsTotal);
  histFolder->Add(&fPointsUnderCounted);
  histFolder->Add(&fPointsOverCounted);

  fDigiManager->Init();
  fNstations = fGeoScheme->GetNStations();
  LOG(debug) << "Init(): fNstations = " << fNstations;

  fhPointsInCluster.resize(fNstations);
  fhDigisInCluster.resize(fNstations);
  fhHitsPerCluster.resize(fNstations);

  for (Int_t i = 0; i < fNstations; i++) {

    fhPointsInCluster[i] =
      new TH1I(Form("hMCPointsInCluster%i", i + 1), Form("MC Points in Cluster : Station %i ", i + 1), 10, 0, 10);
    fhDigisInCluster[i] =
      new TH1I(Form("hDigisInCluster%i", i + 1), Form("Digis in Cluster : Station %i ", i + 1), 10, 0, 10);
    fhHitsPerCluster[i] =
      new TH1I(Form("hHitsPerCluster%i", i + 1), Form("Hits per Cluster : Station %i ", i + 1), 10, 0, 10);
    histFolder->Add(fhPointsInCluster[i]);
    histFolder->Add(fhDigisInCluster[i]);
    histFolder->Add(fhHitsPerCluster[i]);
  }
  gStyle->SetOptStat(1);

  //Pull Distribution
  fhPullX = new TH1D("hPullX", "Pull distribution X;(x_{RC} - x_{MC}) / dx_{RC}", 500, -5, 5);
  fhPullY = new TH1D("hPullY", "Pull distribution Y;(y_{RC} - y_{MC}) / dy_{RC}", 500, -5, 5);
  fhPullT = new TH1D("hPullT", "Pull distribution T;(t_{RC} - t_{MC}) / dt_{RC}", 120, -5, 5);

  //Residual Distribution
  fhResidualX = new TH1D("hResidualX", "Residual distribution X;(x_{RC} - x_{MC})(cm)", 500, -3, 3);
  fhResidualY = new TH1D("hResidualY", "Residual distribution Y;(y_{RC} - y_{MC})(cm)", 500, -3, 3);
  fhResidualT = new TH1D("hResidualT", "Residual distribution T;(t_{RC} - t_{MC})(ns)", 140, -17, 17);

  histFolder->Add(fhPullX);
  histFolder->Add(fhPullY);
  histFolder->Add(fhPullT);
  histFolder->Add(fhResidualX);
  histFolder->Add(fhResidualY);
  histFolder->Add(fhResidualT);

  fCanvPointsInCluster = new CbmQaCanvas("cMCPointsInCluster", "MC Points In Cluster", 2 * 400, 2 * 400);
  fCanvPointsInCluster->Divide2D(fNstations);

  fCanvDigisInCluster = new CbmQaCanvas("cDigisInCluster", "Digis In Cluster", 2 * 400, 2 * 400);
  fCanvDigisInCluster->Divide2D(fNstations);

  fCanvHitsPerCluster = new CbmQaCanvas("cHitsPerCluster", "Hits Per Cluster", 2 * 400, 2 * 400);
  fCanvHitsPerCluster->Divide2D(fNstations);

  fCanvPull = new CbmQaCanvas("cPull", "Pull Distribution", 3 * 600, 1 * 400);
  fCanvPull->Divide2D(3);

  fCanvResidual = new CbmQaCanvas("cResidual", "Residual Distribution", 3 * 600, 1 * 400);
  fCanvResidual->Divide2D(3);

  fOutFolder.Add(fCanvPointsInCluster);
  fOutFolder.Add(fCanvDigisInCluster);
  fOutFolder.Add(fCanvHitsPerCluster);
  fOutFolder.Add(fCanvPull);
  fOutFolder.Add(fCanvResidual);

  return kSUCCESS;
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
void CbmMuchHitFinderQa::SetParContainers()
{
  // Get run and runtime database
  // FairRuntimeDb* db = FairRuntimeDb::instance();
  // if ( ! db ) Fatal("SetParContainers", "No runtime database");
  // Get MUCH geometry parameter container
  // fGeoPar = (CbmGeoMuchPar*) db->getContainer("CbmGeoMuchPar");
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------x
void CbmMuchHitFinderQa::Exec(Option_t*)
{
  fNevents.SetVal(fNevents.GetVal() + 1);
  LOG(debug) << "Event: " << fNevents.GetVal();

  PullsQa();
  StatisticsQa();
  ClusterDeconvQa();
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
void CbmMuchHitFinderQa::FinishTask()
{

  if (fVerbose > 0) {
    printf(" CbmMuchHitFinderQa FinishTask\n");
  }

  gStyle->SetPaperSize(20, 20);

  for (Int_t i = 0; i < fNstations; i++) {
    fhPointsInCluster[i]->Scale(1. / fNevents.GetVal());
    fhDigisInCluster[i]->Scale(1. / fNevents.GetVal());
    fhHitsPerCluster[i]->Scale(1. / fNevents.GetVal());
  }

  std::vector<TH1D*> vResHistos;
  vResHistos.push_back(fhPullX);
  vResHistos.push_back(fhPullY);
  vResHistos.push_back(fhPullT);
  vResHistos.push_back(fhResidualX);
  vResHistos.push_back(fhResidualY);
  vResHistos.push_back(fhResidualT);

  for (UInt_t i = 0; i < vResHistos.size(); i++) {
    TH1D* histo = vResHistos[i];
    histo->Sumw2();
    if (histo->GetRMS() > 0.) {
      histo->Fit("gaus", "Q");
      auto f = histo->GetFunction("gaus");
      if (f) {
        f->SetLineWidth(3);
        f->SetLineColor(kRed);
      }
    }
    // histo->SetStats(kTRUE);
  }

  if (fVerbose > 0) {
    printf("===================================\n");
    printf("StatisticsQa:\n");
    printf("Total number of points: %i\n", fPointsTotal.GetVal());
    printf("Points overcounted: %i\n", fPointsOverCounted.GetVal());
    printf("Points undercounted: %i\n", fPointsUnderCounted.GetVal());
    printf("Signal points: %i\n", fSignalPoints.GetVal());
    printf("Signal hits: %i\n", fSignalHits.GetVal());
  }

  DrawCanvases();

  FairSink* sink = FairRootManager::Instance()->GetSink();
  sink->WriteObject(&fOutFolder, nullptr);
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
void CbmMuchHitFinderQa::DrawCanvases()
{
  for (Int_t i = 0; i < fNstations; i++) {
    fCanvPointsInCluster->cd(i + 1);
    fhPointsInCluster[i]->DrawCopy("", "");

    fCanvDigisInCluster->cd(i + 1);
    fhDigisInCluster[i]->DrawCopy("", "");

    fCanvHitsPerCluster->cd(i + 1);
    fhHitsPerCluster[i]->DrawCopy("", "");
  }

  std::vector<TH1D*> vPullHistos;
  vPullHistos.push_back(fhPullX);
  vPullHistos.push_back(fhPullY);
  vPullHistos.push_back(fhPullT);

  for (UInt_t i = 0; i < vPullHistos.size(); i++) {
    TH1D* histo = vPullHistos[i];
    fCanvPull->cd(i + 1);
    histo->Draw();  //necessary to create stats pointer
    fCanvPull->Update();
    TPaveStats* st = (TPaveStats*) histo->FindObject("stats");
    if (st) {
      st->SetX1NDC(0.621);
      st->SetX2NDC(0.940);
      st->SetY1NDC(0.657);
      st->SetY2NDC(0.929);
      st->SetOptStat(1110);
      st->SetOptFit(11);
      //st->SetTextSize(0.04);
    }
    histo->DrawCopy("", "");
    //version below only changes canvas but not hist folder
    //TH1* hClone = histo->DrawCopy("", "");
    //fCanvPull->Update();
    //TPaveStats *st = (TPaveStats*)hClone->FindObject("stats");
    //st->SetOptStat(1110);
    //st->SetOptFit(11);
  }

  std::vector<TH1D*> vResHistos;
  vResHistos.push_back(fhResidualX);
  vResHistos.push_back(fhResidualY);
  vResHistos.push_back(fhResidualT);

  for (UInt_t i = 0; i < vResHistos.size(); i++) {
    TH1D* histo = vResHistos[i];
    fCanvResidual->cd(i + 1);
    histo->Draw();  //necessary to create stats pointer
    fCanvResidual->Update();
    TPaveStats* st = (TPaveStats*) histo->FindObject("stats");
    if (st) {
      st->SetX1NDC(0.621);
      st->SetX2NDC(0.940);
      st->SetY1NDC(0.657);
      st->SetY2NDC(0.929);
      st->SetOptStat(1110);
      st->SetOptFit(11);
      //st->SetTextSize(0.04);
    }
    histo->DrawCopy("", "");
  }
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
void CbmMuchHitFinderQa::StatisticsQa()
{
  //  Bool_t verbose = (fVerbose>2);
  Int_t nClusters = fClusters->GetEntriesFast();
  TArrayI hitsInCluster;
  TArrayI pointsInCluster;
  hitsInCluster.Set(nClusters);
  pointsInCluster.Set(nClusters);
  LOG(debug) << " start Stat QA ";
  for (Int_t i = 0; i < nClusters; i++)
    hitsInCluster[i] = 0;
  for (Int_t i = 0; i < nClusters; i++)
    pointsInCluster[i] = 0;

  for (Int_t i = 0; i < fHits->GetEntriesFast(); i++) {
    CbmMuchPixelHit* hit = (CbmMuchPixelHit*) fHits->At(i);
    //cout<<" hit index "<<i<<" plane  id "<<hit->GetPlaneId()<<" x  "<<hit->GetX()<<"   y  "<<hit->GetY()<<"   z  "<<hit->GetZ()<<" cluster Id "<< hit->GetRefId()<<endl;

    //    cout<<" hit index "<<i<<" plane  id "<<hit->GetPlaneId()<<endl;
    Int_t clusterId = hit->GetRefId();
    hitsInCluster[clusterId]++;
  }

  for (Int_t i = 0; i < nClusters; i++) {
    CbmMuchCluster* cluster = (CbmMuchCluster*) fClusters->At(i);

    map<Int_t, Int_t> map_points;
    Int_t nDigis = cluster->GetNofDigis();

    auto address      = cluster->GetAddress();
    auto StationIndex = CbmMuchAddress::GetStationIndex(address);
    //cout<<" station index "<<StationIndex<<endl;

    fhDigisInCluster[StationIndex]->Fill(nDigis);

    if (fDigiManager->IsMatchPresent(ECbmModuleId::kMuch)) {
      for (Int_t digiId = 0; digiId < nDigis; digiId++) {
        Int_t index = cluster->GetDigi(digiId);
        //Access Match from CbmDigi only
        CbmMatch* match = (CbmMatch*) fDigiManager->GetMatch(ECbmModuleId::kMuch, index);
        if (!match) {
          LOG(fatal) << "CbmMuchHitFinderQa::StatisticsQa(): Match should be "
                        "present but is null.";
          return;
        }
        Int_t nPoints = match->GetNofLinks();
        for (Int_t iRefPoint = 0; iRefPoint < nPoints; iRefPoint++) {
          Int_t pointId       = match->GetLink(iRefPoint).GetIndex();
          map_points[pointId] = 1;
        }
      }
    }
    pointsInCluster[i] = map_points.size();
    map_points.clear();
  }

  for (Int_t i = 0; i < nClusters; i++) {
    // added
    CbmMuchCluster* cluster = (CbmMuchCluster*) fClusters->At(i);
    auto address            = cluster->GetAddress();
    auto StationIndex       = CbmMuchAddress::GetStationIndex(address);
    //cout<<" station index "<<StationIndex<<endl;
    /// end add

    Int_t nPts  = pointsInCluster[i];
    Int_t nHits = hitsInCluster[i];
    fhPointsInCluster[StationIndex]->Fill(nPts);
    fhHitsPerCluster[StationIndex]->Fill(nHits);
    if (nPts > nHits) fPointsUnderCounted.SetVal(fPointsUnderCounted.GetVal() + (nPts - nHits));
    if (nHits > nPts) fPointsOverCounted.SetVal(fPointsOverCounted.GetVal() + (nHits - nPts));
    fPointsTotal.SetVal(fPointsTotal.GetVal() + nPts);
  }
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
void CbmMuchHitFinderQa::PullsQa()
{
  Bool_t verbose = (fVerbose > 2);
  // Filling residuals and pulls for hits at the first layer
  for (Int_t i = 0; i < fHits->GetEntriesFast(); i++) {
    CbmMuchPixelHit* hit = (CbmMuchPixelHit*) fHits->At(i);
    // Select hits from the first station only
    Int_t iStation = CbmMuchAddress::GetStationIndex(hit->GetAddress());
    Int_t iLayer   = CbmMuchAddress::GetLayerIndex(hit->GetAddress());
    //    if((iStation !=0 && iStation !=1))continue;
    //    if((iStation !=0 && iStation !=1))continue;
    //         cout<<" PULLS QA STATION INDEX "<<iStation<<endl;
    //Earlier finding for only one station
    //if(!(iStation == 0)) continue;
    //    if(!(iStation == 3 && iLayer == 0)) continue;
    if (verbose) printf("   Hit %i, station %i, layer %i ", i, iStation, iLayer);

    Int_t clusterId   = hit->GetRefId();
    Bool_t hit_unique = 1;
    for (Int_t j = i + 1; j < fHits->GetEntriesFast(); j++) {
      CbmMuchPixelHit* hit1 = (CbmMuchPixelHit*) fHits->At(j);
      //if (hit1->GetStationNr()>stationNr) break;
      if (hit1->GetRefId() == clusterId) {
        hit_unique = 0;
        break;
      }
    }
    if (verbose) printf("hit_unique=%i", hit_unique);
    if (!hit_unique) {
      if (verbose) printf("\n");
      continue;
    }

    // Select hits with clusters formed by only one MC Point
    CbmMuchCluster* cluster = (CbmMuchCluster*) fClusters->At(clusterId);
    Bool_t point_unique     = 1;
    CbmLink link;

    for (Int_t digiId = 0; digiId < cluster->GetNofDigis(); digiId++) {
      Int_t index = cluster->GetDigi(digiId);
      //      printf("%i\n",index);
      CbmMuchDigi* digi = (CbmMuchDigi*) fDigiManager->Get<CbmMuchDigi>(index);
      //      cout<<" check 1"<<endl;
      if (!digi) {
        LOG(fatal) << "CbmMuchHitFinderQa::PullsQa(): Error, digi not found.";
        return;
      }
      if (index < 0) {
        LOG(fatal) << "CbmMuchHitFinderQa::PullsQa(): Error, index out of bounds.";
        return;
      }
      CbmMatch* match = (CbmMatch*) fDigiManager->GetMatch(ECbmModuleId::kMuch, index);
      if (!match) {
        LOG(fatal) << "CbmMuchHitFinderQa::PullsQa(): Error, match not found.";
        return;
      }
      // Not unique if the pad has several mcPoint references
      if (verbose) printf(" n=%i", match->GetNofLinks());
      if (match->GetNofLinks() == 0) {
        printf(" noise hit");
        point_unique = 0;
        break;
      }
      if (match->GetNofLinks() > 1) {
        point_unique = 0;
        break;
      }
      CbmLink currentLink = match->GetLink(0);

      CbmMuchModuleGem* module = (CbmMuchModuleGem*) fGeoScheme->GetModuleByDetId(digi->GetAddress());
      if (!module) {
        LOG(fatal) << "CbmMuchHitFinderQa::PullsQa(): Error, module not found.";
        return;
      }
      if (digiId == 0) {
        link = currentLink;
        continue;
      }
      // Not unique if mcPoint references differ for different digis
      if (!(currentLink == link)) {
        point_unique = 0;
        break;
      }
    }

    if (verbose) printf(" point_unique=%i", point_unique);
    if (!point_unique) {
      if (verbose) printf("\n");
      continue;
    }

    if (verbose) {
      printf(" file %i event %i pointId %i", link.GetFile(), link.GetEntry(), link.GetIndex());
    }
    CbmMuchPoint* point = (CbmMuchPoint*) fPoints->Get(link);

    Double_t xMC = 0.5 * (point->GetXIn() + point->GetXOut());
    Double_t yMC = 0.5 * (point->GetYIn() + point->GetYOut());
    Double_t tMC = point->GetTime();
    //    cout<<" MC point time "<<tMC<<" z "<<point->GetZ()<<endl;
    Double_t xRC = hit->GetX();
    Double_t yRC = hit->GetY();
    Double_t dx  = hit->GetDx();
    Double_t dy  = hit->GetDy();

    Double_t tRC = hit->GetTime();
    Double_t dt  = hit->GetTimeError();
    //    cout<<" Rec Hit time "<<tRC<<endl;

    if (dx < 1.e-10) {
      LOG(error) << "Anomalously small dx";
      continue;
    }
    if (dy < 1.e-10) {
      LOG(error) << "Anomalously small dy";
      continue;
    }
    fhPullX->Fill((xRC - xMC) / dx);
    fhPullY->Fill((yRC - yMC) / dy);
    fhPullT->Fill((tRC - tMC) / dt);
    fhResidualX->Fill((xRC - xMC));
    fhResidualY->Fill((yRC - yMC));
    fhResidualT->Fill((tRC - tMC));

    if (verbose) printf("\n");
  }
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
void CbmMuchHitFinderQa::ClusterDeconvQa()
{

  Int_t nClusters = fClusters->GetEntriesFast();
  vector<CbmLink> vPoints;

  {
    const CbmMatch& match = fTimeSlice->GetMatch();
    for (int iLink = 0; iLink < match.GetNofLinks(); iLink++) {
      CbmLink link    = match.GetLink(iLink);
      int nMuchPoints = fPoints->Size(link);
      for (Int_t iPoint = 0; iPoint < nMuchPoints; iPoint++) {
        link.SetIndex(iPoint);
        link.SetWeight(0.);
        if (IsSignalPoint(link)) {
          fSignalPoints.SetVal(fSignalPoints.GetVal() + 1);
        }
        vPoints.push_back(link);
      }
    }
  }

  std::sort(vPoints.begin(), vPoints.end());

  for (Int_t iCluster = 0; iCluster < nClusters; ++iCluster) {
    CbmMuchCluster* cluster = (CbmMuchCluster*) fClusters->At(iCluster);
    if (!cluster) {
      LOG(fatal) << "CbmMuchHitFinderQa::ClusterDeconvQa(): Error, cluster not found.";
      return;
    }
    Int_t nDigis = cluster->GetNofDigis();
    for (Int_t id = 0; id < nDigis; ++id) {
      Int_t iDigi     = cluster->GetDigi(id);
      CbmMatch* match = (CbmMatch*) fDigiManager->GetMatch(ECbmModuleId::kMuch, iDigi);
      if (!match) {
        LOG(fatal) << "CbmMuchHitFinderQa::ClusterDeconvQa(): Error, match not found.";
        return;
      }
      for (Int_t ip = 0; ip < match->GetNofLinks(); ++ip) {
        CbmLink pointLink = match->GetLink(ip);
        auto it           = find(vPoints.begin(), vPoints.end(), pointLink);
        assert(it != vPoints.end());
        if (it->GetWeight() > 0.) continue;
        it->SetWeight(1.);
        if (IsSignalPoint(pointLink)) {
          fSignalHits.SetVal(fSignalHits.GetVal() + 1);
        }
      }
    }
  }
}
// -------------------------------------------------------------------------

Bool_t CbmMuchHitFinderQa::IsSignalPoint(CbmLink pointLink)
{

  CbmMuchPoint* point = (CbmMuchPoint*) fPoints->Get(pointLink);
  if (!point) return kFALSE;
  Int_t iTrack      = point->GetTrackID();
  CbmMCTrack* track = (CbmMCTrack*) fMCTracks->Get(pointLink.GetFile(), pointLink.GetEntry(), iTrack);
  if (!track) return kFALSE;

  if (iTrack != 0 && iTrack != 1) return kFALSE;  // Signal tracks must be fist ones
  // Verify if it is a signal muon
  if (track->GetMotherId() < 0) {  // No mother for signal
    Int_t pdgCode = track->GetPdgCode();
    if (TMath::Abs(pdgCode) == 13) {  // If it is a muon
      return kTRUE;
    }
  }
  return kFALSE;
}

ClassImp(CbmMuchHitFinderQa)
