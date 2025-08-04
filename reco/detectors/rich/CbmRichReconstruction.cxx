/* Copyright (C) 2012-2021 UGiessen/JINR-LIT, Giessen/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev [committer], Andrey Lebedev */

/**
 * \file CbmRichReconstruction.cxx
 *
 * \author Semen Lebedev
 * \date 2012
 **/

#include "CbmRichReconstruction.h"

#include "CbmRichProjectionProducerAnalytical.h"
#include "CbmRichProjectionProducerTGeo.h"
#include "CbmRichRing.h"
//#include "prototype/CbmRichProtProjectionProducer.h"

#include "CbmL1RichENNRingFinder.h"
#include "CbmL1RichENNRingFinderParallel.h"
#include "CbmRichRingFinderHough.h"
#include "CbmRichRingFinderIdeal.h"
#include "CbmRichTrackExtrapolationBase.h"
#include "CbmRichTrackExtrapolationIdeal.h"
#include "CbmRichTrackExtrapolationKF.h"
#include "CbmRichTrackExtrapolationLittrack.h"
#include "CbmRichTrackExtrapolationMirrorIdeal.h"
//#include "prototype/CbmRichProtRingFinderHough.h"

#include "CbmEvent.h"
#include "CbmGlobalTrack.h"
#include "CbmRichConverter.h"
#include "CbmRichGeoManager.h"
#include "CbmRichRingFitterCOP.h"
#include "CbmRichRingFitterCircle.h"
#include "CbmRichRingFitterEllipseMinuit.h"
#include "CbmRichRingFitterEllipseTau.h"
#include "CbmRichRingFitterRobustCOP.h"
#include "CbmRichRingFitterTAU.h"
#include "CbmRichRingTrackAssignClosestD.h"
#include "FairHit.h"
#include "FairRootManager.h"
#include "TClonesArray.h"
#include "TStopwatch.h"

#include <Logger.h>

#include <iomanip>
#include <iostream>

using std::fixed;
using std::right;
using std::setprecision;
using std::setw;

CbmRichReconstruction::CbmRichReconstruction() : FairTask("CbmRichReconstruction") {}

CbmRichReconstruction::~CbmRichReconstruction()
{
  if (nullptr != fRingFinder) delete fRingFinder;
  if (nullptr != fRingFitter) delete fRingFitter;
  if (nullptr != fTrackExtrapolation) delete fTrackExtrapolation;
  if (nullptr != fProjectionProducer) delete fProjectionProducer;
  if (nullptr != fRingTrackAssign) delete fRingTrackAssign;
}

InitStatus CbmRichReconstruction::Init()
{
  FairRootManager* manager = FairRootManager::Instance();
  if (nullptr == manager) LOG(fatal) << "CbmRichReconstruction::Init(): FairRootManager is nullptr.";

  fCbmEvents = dynamic_cast<TClonesArray*>(manager->GetObject("CbmEvent"));
  if (fCbmEvents == nullptr) {
    LOG(info) << GetName() << "::Init() CbmEvent NOT found \n";
  }
  else {
    LOG(info) << GetName() << "::Init() CbmEvent found";
  }

  if (fRunExtrapolation) {
    fRichTrackParamZ = new TClonesArray("FairTrackParam", 100);
    manager->Register("RichTrackParamZ", "RICH", fRichTrackParamZ, IsOutputBranchPersistent("RichTrackParamZ"));

    fGlobalTracks = static_cast<TClonesArray*>(manager->GetObject("GlobalTrack"));
    if (fGlobalTracks == nullptr) LOG(fatal) << "CbmRichReconstruction::Init(): No GlobalTrack array.";
  }

  if (fRunProjection) {
    if (!fRunExtrapolation) LOG(fatal) << "CbmRichReconstruction::Init(): fRunExtrapolation must be true.";
    fRichProjections = new TClonesArray("FairTrackParam");
    manager->Register("RichProjection", "RICH", fRichProjections, IsOutputBranchPersistent("RichProjection"));
  }

  fRichHits = static_cast<TClonesArray*>(manager->GetObject("RichHit"));
  if (fRichHits == nullptr) LOG(fatal) << "CbmRichReconstruction::Init(): No RichHit array.";

  fRichRings = new TClonesArray("CbmRichRing", 100);
  manager->Register("RichRing", "RICH", fRichRings, IsOutputBranchPersistent("RichRing"));

  // This was checked for v17a, v21a geometries. The offset was chosen that
  // the value for v17a is 260 cm and the value for v21a is 220 cm
  double offset        = 205.7331;
  fZTrackExtrapolation = CbmRichGeoManager::GetInstance().fGP->fMirrorZ + offset;
  LOG(info) << "CbmRichReconstruction::Init() fZTrackExtrapolation = " << fZTrackExtrapolation;
  if (fZTrackExtrapolation < 200. || fZTrackExtrapolation > 300.) {
    LOG(fatal) << "CbmRichReconstruction::Init() fZTrackExtrapolation = " << fZTrackExtrapolation
               << " The value of fZTrackExtrapolation is not correct. It must be in the range [200, 300] cm."
               << " Probably the RICH geometry is not correct or it is not supported.";
  }

  if (fRunExtrapolation) InitExtrapolation();
  if (fRunProjection) InitProjection();
  if (fRunFinder) InitFinder();
  if (fRunFitter) InitFitter();
  if (fRunTrackAssign) InitTrackAssign();

  return kSUCCESS;
}

void CbmRichReconstruction::Exec(Option_t* /*opt*/)
{
  TStopwatch timer;
  timer.Start();
  Int_t nEvents{0};
  Int_t nTrackProj{0};
  Int_t nGlobalTracks{0};

  if (fRichTrackParamZ != nullptr) fRichTrackParamZ->Delete();
  if (fRichProjections != nullptr) fRichProjections->Delete();
  if (fRichRings != nullptr) fRichRings->Delete();

  if (fCbmEvents == nullptr) {
    ProcessData(nullptr);
    nTrackProj += (fRichProjections ? fProjectionProducer->GetSuccessfullProj() : 0);
    nGlobalTracks += (fGlobalTracks ? fGlobalTracks->GetEntriesFast() : 0);
  }
  else {
    nEvents = fCbmEvents->GetEntriesFast();
    fNofEvents += nEvents;
    for (Int_t iEvent = 0; iEvent < nEvents; iEvent++) {
      CbmEvent* event = static_cast<CbmEvent*>(fCbmEvents->At(iEvent));
      ProcessData(event);
      nTrackProj += (fRichProjections ? fProjectionProducer->GetSuccessfullProj() : 0);
      nGlobalTracks += (fGlobalTracks ? event->GetNofData(ECbmDataType::kGlobalTrack) : 0);
    }
  }

  timer.Stop();
  std::stringstream logOut;
  logOut << setw(20) << left << GetName() << "[";
  logOut << fixed << setw(8) << setprecision(1) << right << timer.RealTime() * 1000. << " ms] ";
  logOut << "TS " << fNofTs;
  if (fCbmEvents) logOut << ", events " << nEvents;
  logOut << ", hits " << fRichHits->GetEntriesFast();
  logOut << ", rings " << fRichRings->GetEntriesFast();
  if (fRichProjections) logOut << ", trackProj " << nTrackProj << " / " << nGlobalTracks;
  LOG(info) << logOut.str();

  fNofTs++;
  fCalcTime[0] += timer.RealTime();
  fTotalNofHits += fRichHits->GetEntriesFast();
  fTotalNofRings += fRichRings->GetEntriesFast();
  fTotalNofTrackProj += nTrackProj;
  fTotalNofGlobalTracks += nGlobalTracks;
}

void CbmRichReconstruction::ProcessData(CbmEvent* event)
{
  TStopwatch timer;
  if (fRunExtrapolation) {
    timer.Start();
    RunExtrapolation(event);
    timer.Stop();
    fCalcTime[1] += timer.RealTime();
  }
  if (fRunProjection) {
    timer.Start();
    RunProjection(event);
    timer.Stop();
    fCalcTime[2] += timer.RealTime();
  }
  if (fRunFinder) {
    timer.Start();
    RunFinder(event);
    timer.Stop();
    fCalcTime[3] += timer.RealTime();
  }
  if (fRunFitter) {
    timer.Start();
    RunFitter(event);
    timer.Stop();
    fCalcTime[4] += timer.RealTime();
  }
  if (fRunTrackAssign) {
    timer.Start();
    RunTrackAssign(event);
    timer.Stop();
    fCalcTime[5] += timer.RealTime();
  }
}

void CbmRichReconstruction::InitExtrapolation()
{
  if (fExtrapolationName == "ideal") {
    fTrackExtrapolation = new CbmRichTrackExtrapolationIdeal();
  }
  else if (fExtrapolationName == "mirror_ideal") {
    fTrackExtrapolation = new CbmRichTrackExtrapolationMirrorIdeal();
  }
  else if (fExtrapolationName == "kf" || fExtrapolationName == "KF") {
    fTrackExtrapolation = new CbmRichTrackExtrapolationKF();
  }
  else if (fExtrapolationName == "lit" || fExtrapolationName == "littrack") {
    fTrackExtrapolation = new CbmRichTrackExtrapolationLittrack();
  }
  else {
    LOG(fatal) << fExtrapolationName << " is not correct name for extrapolation algorithm.";
  }
  fTrackExtrapolation->Init();
}

void CbmRichReconstruction::InitProjection()
{
  if (fProjectionName == "analytical") {
    fProjectionProducer = new CbmRichProjectionProducerAnalytical();
  }
  else if (fProjectionName == "TGeo" || fProjectionName == "tgeo") {
    fProjectionProducer = new CbmRichProjectionProducerTGeo();
  }
  else {
    LOG(fatal) << fFinderName << " is not correct name for projection producer algorithm.";
  }
  fProjectionProducer->Init();
}

void CbmRichReconstruction::InitFinder()
{
  if (fFinderName == "hough") {
    fRingFinder = new CbmRichRingFinderHough();
    static_cast<CbmRichRingFinderHough*>(fRingFinder)->SetUseAnnSelect(fUseHTAnnSelect);
    static_cast<CbmRichRingFinderHough*>(fRingFinder)->SetUseSubdivide(fUseHTSubdivide);
  }
  else if (fFinderName == "ideal") {
    fRingFinder = new CbmRichRingFinderIdeal();
  }
  else if (fFinderName == "enn") {
    fRingFinder = new CbmL1RichENNRingFinder(0);
  }
  else if ((fFinderName == "enn_parallel")) {
    fRingFinder = new CbmL1RichENNRingFinderParallel(0);
  }
  /*
  else if (fFinderName == "hough_prototype") {
     fRingFinder = new CbmRichProtRingFinderHough();
     }*/
  else {
    LOG(fatal) << fFinderName << " is not correct name for ring finder algorithm.";
  }

  fRingFinder->Init();
}

void CbmRichReconstruction::InitFitter()
{
  if (fFitterName == "circle_cop") {
    fRingFitter = new CbmRichRingFitterCOP();
  }
  else if (fFitterName == "circle_simple") {
    fRingFitter = new CbmRichRingFitterCircle();
  }
  else if (fFitterName == "circle_tau") {
    fRingFitter = new CbmRichRingFitterTAU();
  }
  else if (fFitterName == "circle_robust_cop") {
    fRingFitter = new CbmRichRingFitterRobustCOP();
  }
  else if (fFitterName == "ellipse_tau") {
    fRingFitter = new CbmRichRingFitterEllipseTau();
  }
  else if (fFitterName == "ellipse_minuit") {
    fRingFitter = new CbmRichRingFitterEllipseMinuit();
  }
  else {
    LOG(fatal) << fFitterName << " is not correct name for ring fitter algorithm.";
  }
  CbmRichConverter::Init();
}

void CbmRichReconstruction::InitTrackAssign()
{
  if (fTrackAssignName == "closest_distance") {
    fRingTrackAssign = new CbmRichRingTrackAssignClosestD();
  }
  else {
    LOG(fatal) << fTrackAssignName << " is not correct name for ring-track assignment algorithm.";
  }
  fRingTrackAssign->Init();
}

void CbmRichReconstruction::RunExtrapolation(CbmEvent* event)
{
  if (fRichTrackParamZ == nullptr) LOG(info) << "fRichTrackParamZ == nullptr";
  fTrackExtrapolation->DoExtrapolation(event, fGlobalTracks, fRichTrackParamZ, fZTrackExtrapolation);
}

void CbmRichReconstruction::RunProjection(CbmEvent* event)
{
  fProjectionProducer->DoProjection(event, fRichProjections);
}

void CbmRichReconstruction::RunFinder(CbmEvent* event)
{
  fRingFinder->DoFind(event, fRichHits, fRichProjections, fRichRings);
}

void CbmRichReconstruction::RunFitter(CbmEvent* event)
{
  const Int_t nofRings = event ? event->GetNofData(ECbmDataType::kRichRing) : fRichRings->GetEntriesFast();
  if (nofRings <= 0) return;
  for (Int_t iR0 = 0; iR0 < nofRings; iR0++) {
    Int_t iR          = event ? event->GetIndex(ECbmDataType::kRichRing, iR0) : iR0;
    CbmRichRing* ring = static_cast<CbmRichRing*>(fRichRings->At(iR));
    if (nullptr == ring) continue;
    CbmRichRingLight ringL;

    CbmRichConverter::CopyHitsToRingLight(ring, &ringL);
    fRingFitter->DoFit(&ringL);
    CbmRichConverter::CopyParamsToRing(&ringL, ring);
  }
}

void CbmRichReconstruction::RunTrackAssign(CbmEvent* event)
{
  fRingTrackAssign->DoAssign(event, fRichRings, fRichProjections);
}

void CbmRichReconstruction::Finish()
{
  std::cout << std::endl;
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Time slices     : " << fNofTs;
  LOG(info) << "Hits      / TS  : " << fixed << setprecision(2) << fTotalNofHits / Double_t(fNofTs);
  LOG(info) << "Rings     / TS  : " << fixed << setprecision(2) << fTotalNofRings / Double_t(fNofTs);
  if (fRichProjections)
    LOG(info) << "TrackProj / TS  : " << fixed << setprecision(2) << fTotalNofTrackProj / Double_t(fNofTs);
  LOG(info) << "Time      / TS  : " << fixed << setprecision(2) << 1000. * fCalcTime[0] / Double_t(fNofTs) << " ms";
  if (fCbmEvents) {
    LOG(info) << "Events          : " << fNofEvents;
    LOG(info) << "Events    / TS  : " << fixed << setprecision(2) << fNofEvents / Double_t(fNofTs);
    if (fNofEvents > 0) {
      LOG(info) << "Hits      / ev  : " << fixed << setprecision(2) << fTotalNofHits / Double_t(fNofEvents);
      LOG(info) << "Rings     / ev  : " << fixed << setprecision(2) << fTotalNofRings / Double_t(fNofEvents);
      if (fRichProjections)
        LOG(info) << "TrackProj / ev  : " << fixed << setprecision(2) << fTotalNofTrackProj / Double_t(fNofEvents);
      LOG(info) << "Time      / ev  : " << fixed << setprecision(2) << 1000. * fCalcTime[0] / Double_t(fNofEvents)
                << " ms";
    }
  }
  if (fRichProjections && fTotalNofGlobalTracks > 0)
    LOG(info) << "TrackProj / GTr : " << fixed << setprecision(2)
              << fTotalNofTrackProj / Double_t(fTotalNofGlobalTracks);
  TString eventOrTsStr    = fCbmEvents && fNofEvents > 0 ? "ev: " : "TS: ";
  Double_t eventOrTsValue = Double_t(fCbmEvents && fNofEvents > 0 ? fNofEvents : fNofTs);
  if (fCalcTime[0] != 0.) {
    LOG(info) << "===== Time by task (real time) ======";
    if (fTrackExtrapolation)
      LOG(info) << "TrackExtrapolation / " << eventOrTsStr << fixed << setprecision(2) << setw(9) << right
                << 1000. * fCalcTime[1] / eventOrTsValue << " ms [" << setw(5) << right
                << 100 * fCalcTime[1] / fCalcTime[0] << " %]";
    if (fProjectionProducer)
      LOG(info) << "TrackProjection    / " << eventOrTsStr << fixed << setprecision(2) << setw(9) << right
                << 1000. * fCalcTime[2] / eventOrTsValue << " ms [" << setw(5) << right
                << 100 * fCalcTime[2] / fCalcTime[0] << " %]";
    if (fRingFinder)
      LOG(info) << "RingFinder         / " << eventOrTsStr << fixed << setprecision(2) << setw(9) << right
                << 1000. * fCalcTime[3] / eventOrTsValue << " ms [" << setw(5) << right
                << 100 * fCalcTime[3] / fCalcTime[0] << " %]";
    if (fRingFitter)
      LOG(info) << "RingFitter         / " << eventOrTsStr << fixed << setprecision(2) << setw(9) << right
                << 1000. * fCalcTime[4] / eventOrTsValue << " ms [" << setw(5) << right
                << 100 * fCalcTime[4] / fCalcTime[0] << " %]";
    if (fRingTrackAssign)
      LOG(info) << "RingTrackAssign    / " << eventOrTsStr << fixed << setprecision(2) << setw(9) << right
                << 1000. * fCalcTime[5] / eventOrTsValue << " ms [" << setw(5) << right
                << 100 * fCalcTime[5] / fCalcTime[0] << " %]";
  }
  LOG(info) << "=====================================\n";
}

ClassImp(CbmRichReconstruction)
