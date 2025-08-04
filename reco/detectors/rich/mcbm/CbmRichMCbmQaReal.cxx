/* Copyright (C) 2019-2021 UGiessen/JINR-LIT, Giessen/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Adrian Amatus Weber, Semen Lebedev [committer] */

#include "CbmRichMCbmQaReal.h"

#include "CbmDigiManager.h"
#include "CbmDrawHist.h"
#include "CbmEvent.h"
#include "CbmGlobalTrack.h"
#include "CbmHistManager.h"
#include "CbmMatchRecoToMC.h"
#include "CbmRichConverter.h"
#include "CbmRichDigi.h"
#include "CbmRichDraw.h"
#include "CbmRichGeoManager.h"
#include "CbmRichHit.h"
#include "CbmRichMCbmSEDisplay.h"
#include "CbmRichPoint.h"
#include "CbmRichRing.h"
#include "CbmRichRingFinderHoughImpl.h"
#include "CbmRichUtil.h"
#include "CbmStsDigi.h"
#include "CbmTofDigi.h"
#include "CbmTofHit.h"
#include "CbmTofTracklet.h"
#include "CbmTrackMatchNew.h"
#include "CbmTrdDigi.h"
#include "CbmTrdTrack.h"
#include "CbmUtils.h"
#include "TCanvas.h"
#include "TClonesArray.h"
#include "TEllipse.h"
#include "TF1.h"
#include "TGeoBBox.h"
#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TH1.h"
#include "TH1D.h"
#include "TLatex.h"
#include "TLine.h"
#include "TMarker.h"
#include "TMath.h"
#include "TStyle.h"
#include "TSystem.h"

#include <TBox.h>
#include <TFile.h>
#include <TLegend.h>
//#include <CbmSetup.h>

#include <boost/assign/list_of.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using boost::assign::list_of;

//double RichZPos =  348.;
double RichZPos = 355.2;

CbmRichMCbmQaReal::CbmRichMCbmQaReal()
  : FairTask("CbmRichMCbmQaReal")
  , fBmonDigis(nullptr)
  , fRichHits(nullptr)
  , fRichRings(nullptr)
  , fTofHits(nullptr)
  , fTofTracks(nullptr)
  , fTSHeader(nullptr)
  , fCbmEvent(nullptr)
  , fHM(nullptr)
  , fXOffsetHisto(0.)
  , fTotRichMin(23.7)
  , fTotRichMax(30.0)
  , fEventNum(0)
  , fNofDrawnRings(0)
  , fNofDrawnRichTofEv(0)
  , fMaxNofDrawnEvents(100)
  , fTriggerRichHits(0)
  , fTriggerTofHits(0)
  , fOutputDir("result")
{
}

InitStatus CbmRichMCbmQaReal::Init()
{
  cout << "CbmRichMCbmQaReal::Init" << endl;

  FairRootManager* ioman = FairRootManager::Instance();
  if (nullptr == ioman)
    LOG(fatal) << "CbmRichMCbmQaReal::Init "
               << "RootManager not instantised!";

  fDigiMan = CbmDigiManager::Instance();
  fDigiMan->Init();

  if (!fDigiMan->IsPresent(ECbmModuleId::kRich))
    LOG(fatal) << "CbmRichMCbmQaReal::Init: "
               << "No Rich Digis!";

  if (!fDigiMan->IsPresent(ECbmModuleId::kTof))
    LOG(fatal) << "CbmRichMCbmQaReal::Init: "
               << "No Tof Digis!";

  fRichHits = (TClonesArray*) ioman->GetObject("RichHit");
  if (nullptr == fRichHits)
    LOG(fatal) << "CbmRichMCbmQaReal::Init: "
               << "No Rich Hits!";

  fRichRings = (TClonesArray*) ioman->GetObject("RichRing");
  if (nullptr == fRichRings)
    LOG(fatal) << "CbmRichMCbmQaReal::Init: "
               << "No Rich Rings!";

  fTofHits = (TClonesArray*) ioman->GetObject("TofHit");
  if (nullptr == fTofHits)
    LOG(fatal) << "CbmRichMCbmQaReal::Init: "
               << "No Tof Hits!";

  fTofTracks = (TClonesArray*) ioman->GetObject("TofTracks");
  if (nullptr == fTofTracks)
    LOG(warning) << "CbmRichMCbmQaReal::Init: "
                 << "No Tof Tracks!";

  fBmonDigis = ioman->InitObjectAs<std::vector<CbmTofDigi> const*>("BmonDigi");
  if (nullptr == fBmonDigis)
    LOG(warning) << "CbmRichMCbmQaReal::Init: "
                 << "No Bmon Digis!";

  fTSHeader = ioman->InitObjectAs<CbmTsEventHeader const*>("EventHeader.");
  if (nullptr == fTSHeader)
    LOG(warning) << "CbmRichMCbmQaReal::Init: "
                 << "No EventHeader!";

  fCbmEvent = dynamic_cast<TClonesArray*>(ioman->GetObject("CbmEvent"));
  if (nullptr == fCbmEvent)
    LOG(fatal) << "CbmRichMCbmQaReal::Init: "
               << "No Event!";

  InitHistograms();

  //--- Init Single Event display -----------------------------
  fSeDisplay = new CbmRichMCbmSEDisplay(fHM);
  fSeDisplay->SetRichHits(fRichHits);
  fSeDisplay->SetRichRings(fRichRings);
  fSeDisplay->SetTofTracks(fTofTracks);
  fSeDisplay->SetTotRich(fTotRichMin, fTotRichMax);
  fSeDisplay->SetMaxNofDrawnEvents(fMaxNofDrawnEvents);
  fSeDisplay->SetOutDir(fOutputDir);
  fSeDisplay->SetLELimits(-60.0, 200.0);
  fSeDisplay->XOffsetHistos(fXOffsetHisto);
  //-----------------------------------------------------------


  //--- Init Single Event display for Correlated Tracks/Rings---
  fSeDsply_TR = new CbmRichMCbmSEDisplay(fHM);
  fSeDsply_TR->SetRichHits(fRichHits);
  fSeDsply_TR->SetRichRings(fRichRings);
  fSeDsply_TR->SetTofTracks(fTofTracks);
  fSeDsply_TR->SetTotRich(fTotRichMin, fTotRichMax);
  fSeDsply_TR->SetMaxNofDrawnEvents(fMaxNofDrawnEvents);
  fSeDsply_TR->SetOutDir(fOutputDir);
  fSeDsply_TR->XOffsetHistos(fXOffsetHisto);
  fSeDsply_TR->SetLELimits(-60.0, 200.0);
  fSeDsply_TR->SetCanvasDir("SE_Corr");
  //-----------------------------------------------------------


  return kSUCCESS;
}

void CbmRichMCbmQaReal::InitHistograms()
{
  fHM = new CbmHistManager();

  fHM->Create1<TH1D>("fhNofEvents", "fhNofEvents;Entries", 1, 0.5, 1.5);
  fHM->Create1<TH1D>("fhNofCbmEvents", "fhNofCbmEvents;Entries", 1, 0.5, 1.5);
  fHM->Create1<TH1D>("fhNofCbmEventsRing", "fhNofCbmEventsRing;Entries", 1, 0.5, 1.5);

  fHM->Create1<TH1D>("fhHitsInTimeslice", "fhHitsInTimeslice;Timeslice;#Hits", 200, 1, 200);

  // nof objects per timeslice
  fHM->Create1<TH1D>("fhNofRichDigisInTimeslice", "fhNofRichDigisInTimeslice;# RICH digis / timeslice;Entries", 100,
                     -0.5, 999.5);
  fHM->Create1<TH1D>("fhNofRichHitsInTimeslice", "fhNofRichHitsInTimeslice;# RICH hits / timeslice;Entries", 100, -0.5,
                     999.5);
  fHM->Create1<TH1D>("fhNofRichRingsInTimeslice", "fhNofRichRingsInTimeslice;# RICH rings / timeslice;Entries", 10,
                     -0.5, 9.5);

  // RICH hits
  fHM->Create2<TH2D>("fhRichHitXY", "fhRichHitXY;RICH hit X [cm];RICH hit Y [cm];Entries", 67, -20.1 + fXOffsetHisto,
                     20.1 + fXOffsetHisto, 84, -25.2, 25.2);
  fHM->Create2<TH2D>("fhRichDigiPixelRate", "fhRichDigiPixelRate;RICH Digi X [cm];RICH Digi Y [cm];Hz", 67,
                     -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 84, -25.2, 25.2);


  // RICH digis, the limits of log histograms are set in Exec method
  fHM->Create1<TH1D>("fhRichDigisTimeLog", "fhNofRichDigisTimeLog;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhRichDigisTimeLogZoom", "fhNofRichDigisTimeLogZoom;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhRichDigisTimeLogZoom2", "fhNofRichDigisTimeLogZoom2;Time [ns];Entries", 400, 0., 0.);

  fHM->Create1<TH1D>("fhRichRingsTimeLog", "fhNofRichRingsTimeLog;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhRichRingsTimeLogZoom", "fhNofRichRingsTimeLogZoom;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhRichRingsTimeLogZoom2", "fhNofRichRingsTimeLogZoom2;Time [ns];Entries", 400, 0., 0.);

  //TOF
  fHM->Create1<TH1D>("fhTofDigisTimeLog", "fhTofDigisTimeLog;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhTofDigisTimeLogZoom", "fhTofDigisTimeLogZoom;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhTofDigisTimeLogZoom2", "fhTofDigisTimeLogZoom2;Time [ns];Entries", 400, 0., 0.);

  //STS
  fHM->Create1<TH1D>("fhStsDigisTimeLog", "fhStsDigisTimeLog;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhStsDigisTimeLogZoom", "fhStsDigisTimeLogZoom;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhStsDigisTimeLogZoom2", "fhStsDigisTimeLogZoom2;Time [ns];Entries", 400, 0., 0.);

  //Bmon
  fHM->Create1<TH1D>("fhBmonDigisTimeLog", "fhBmonDigisTimeLog;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhBmonDigisTimeLogZoom", "fhBmonDigisTimeLogZoom;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhBmonDigisTimeLogZoom2", "fhBmonDigisTimeLogZoom2;Time [ns];Entries", 400, 0., 0.);

  //TRD1D
  fHM->Create1<TH1D>("fhTrd1dDigisTimeLog", "fhTrd1dDigisTimeLog;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhTrd1dDigisTimeLogZoom", "fhTrd1dDigisTimeLogZoom;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhTrd1dDigisTimeLogZoom2", "fhTrd1dDigisTimeLogZoom2;Time [ns];Entries", 400, 0., 0.);

  //TRD2D
  fHM->Create1<TH1D>("fhTrd2dDigisTimeLog", "fhhTrd2dDigisTimeLog;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhTrd2dDigisTimeLogZoom", "fhhTrd2dDigisTimeLogZoom;Time [ns];Entries", 400, 0., 0.);
  fHM->Create1<TH1D>("fhTrd2dDigisTimeLogZoom2", "fhhTrd2dDigisTimeLogZoom2;Time [ns];Entries", 400, 0., 0.);

  //ToT
  fHM->Create1<TH1D>("fhRichDigisToT", "fhRichDigisToT;ToT [ns];Entries", 601, 9.975, 40.025);
  fHM->Create1<TH1D>("fhRichHitToT", "fhRichHitToT;ToT [ns];Entries", 601, 9.975, 40.025);
  fHM->Create1<TH1D>("fhRichHitToTEvent", "fhRichHitToTEvent;ToT [ns];Entries", 601, 9.975, 40.025);
  fHM->Create2<TH2D>("fhRichHitXYEvent", "fhRichHitXYEvent;RICH hit X [cm];RICH hit Y [cm];Entries", 67,
                     -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 84, -25.2, 25.2);


  // RICH rings
  fHM->Create2<TH2D>("fhRichRingXY", "fhRichRingXY;Ring center X [cm];Ring center Y [cm];Entries", 67,
                     -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 84, -25.2, 25.2);
  fHM->Create2<TH2D>("fhRichRingXY_goodTrack", "fhRichRingXY_goodTrack;Ring center X [cm];Ring center Y [cm];Entries",
                     67, -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 84, -25.2, 25.2);
  fHM->Create2<TH2D>("fhRichRing_goodTrackXY", "fhRichRing_goodTrackXY;Track center X [cm];Track center Y [cm];Entries",
                     67, -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 84, -25.2, 25.2);

  fHM->Create2<TH2D>("fhRichRingRadiusY", "fhRichRingRadiusY;Ring Radius [cm]; Y position[cm];Entries", 70, -0.05, 6.95,
                     84, -25.2, 25.2);
  fHM->Create2<TH2D>("fhRichHitsRingRadius", "fhRichHitsRingRadius;#Rich Hits/Ring; Ring Radius [cm];Entries", 50, -0.5,
                     49.5, 70, -0.05, 6.95);

  fHM->Create1<TH1D>("fhRichRingRadius", "fhRichRingRadius;Ring radius [cm];Entries", 100, 0., 7.);
  fHM->Create1<TH1D>("fhNofHitsInRing", "fhNofHitsInRing;# hits in ring;Entries", 50, -0.5, 49.5);

  fHM->Create1<TH1D>("fhRichRingRadius_goodRing", "fhRichRingRadius_goodRing;Ring radius [cm];Entries", 100, 0., 7.);
  fHM->Create1<TH1D>("fhNofHitsInRing_goodRing", "fhNofHitsInRing_goodRing;# hits in ring;Entries", 50, -0.5, 49.5);

  //Tof Rich correlation
  fHM->Create2<TH2D>("fhTofRichX", "fhTofRichX;Rich Hit X [cm];TofHit X [cm];Entries", 67, -20.1 + fXOffsetHisto,
                     20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofRichX_stack1", "fhTofRichX_stack1;Rich Hit X [cm];TofHit X [cm];Entries", 67,
                     -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofRichX_stack2", "fhTofRichX_stack2;Rich Hit X [cm];TofHit X [cm];Entries", 67,
                     -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofRichX_stack3", "fhTofRichX_stack3;Rich Hit X [cm];TofHit X [cm];Entries", 67,
                     -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofRichY", "fhTofRichY;Rich Hit Y [cm];TofHit Y [cm];Entries", 84, -25.2, 25.2, 200, -80, 80);
  fHM->Create2<TH2D>("fhTofRichY_stack1", "fhTofRichY_stack1;Rich Hit Y [cm];TofHit Y [cm];Entries", 84, -25.2, 25.2,
                     200, -80, 80);
  fHM->Create2<TH2D>("fhTofRichY_stack2", "fhTofRichY_stack2;Rich Hit Y [cm];TofHit Y [cm];Entries", 84, -25.2, 25.2,
                     200, -80, 80);
  fHM->Create2<TH2D>("fhTofRichY_stack3", "fhTofRichY_stack3;Rich Hit Y [cm];TofHit Y [cm];Entries", 84, -25.2, 25.2,
                     200, -80, 80);
  //fHM->Create2<TH2D>("fhTofRichRingHitX","fhTofRichRingHitX;Rich Ring Hit X [cm];TofHit X [cm];Entries", 67, -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto,  400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackRichHitX", "fhTofTrackRichHitX;RICH Hit X [cm];TofTrack X [cm];Entries", 67,
                     -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackRichHitY", "fhTofTrackRichHitY;RICH Hit Y [cm];TofTrack Y [cm];Entries", 84, -25.2,
                     25.2, 200, -80, 80);


  fHM->Create2<TH2D>("fhTofTrackHitRichHitX_oBetacuts_dtime",
                     "fhTofTrackHitRichHitX_oBetacuts_dtime;Rich Hit X "
                     "[cm];TofHit X [cm];Entries",
                     67, -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackHitRichHitY_oBetacuts_dtime",
                     "fhTofTrackHitRichHitY_oBetacuts_dtime;Rich Hit Y "
                     "[cm];TofHit Y [cm];Entries",
                     84, -25.2, 25.2, 200, -80, 80);


  fHM->Create2<TH2D>("fhTofTrackRichHitX_cuts", "fhTofTrackRichHitX_cuts;RICH Hit X [cm];TofTrack X [cm];Entries", 67,
                     -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackRichHitY_cuts", "fhTofTrackRichHitY_cuts;RICH Hit Y [cm];TofTrack Y [cm];Entries", 84,
                     -25.2, 25.2, 200, -80, 80);

  fHM->Create2<TH2D>("fhTofTrackRichHitX_oBetacuts",
                     "fhTofTrackRichHitX_oBetacuts;RICH Hit X [cm];TofTrack X [cm];Entries", 67, -20.1 + fXOffsetHisto,
                     20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackRichHitY_oBetacuts",
                     "fhTofTrackRichHitY_oBetacuts;RICH Hit Y [cm];TofTrack Y [cm];Entries", 84, -25.2, 25.2, 200, -80,
                     80);
  fHM->Create1<TH1D>("fhTofTrackRichHitTime_oBetacuts", "fhTofTrackRichHitTime_oBetacuts;#Delta Time [ns];Entries", 280,
                     -70., 70.);

  fHM->Create2<TH2D>("fhTofTrackRichHitX_uBetacuts",
                     "fhTofTrackRichHitX_uBetacuts;RICH Hit X [cm];TofTrack X [cm];Entries", 67, -20.1 + fXOffsetHisto,
                     20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackRichHitY_uBetacuts",
                     "fhTofTrackRichHitY_uBetacuts;RICH Hit Y [cm];TofTrack Y [cm];Entries", 84, -25.2, 25.2, 200, -80,
                     80);

  fHM->Create2<TH2D>("fhTofTrackRichHitX_oBetacuts_dtime",
                     "fhTofTrackRichHitX_oBetacuts_dtime;RICH Hit X "
                     "[cm];TofTrack X [cm];Entries",
                     67, -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackRichHitY_oBetacuts_dtime",
                     "fhTofTrackRichHitY_oBetacuts_dtime;RICH Hit Y "
                     "[cm];TofTrack Y [cm];Entries",
                     84, -25.2, 25.2, 200, -80, 80);

  fHM->Create2<TH2D>("fhTofTrackRichHitX_oBetacuts_dtime_4",
                     "fhTofTrackRichHitX_oBetacuts_dtime_4;RICH Hit X "
                     "[cm];TofTrack X [cm];Entries",
                     67, -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackRichHitY_oBetacuts_dtime_4",
                     "fhTofTrackRichHitY_oBetacuts_dtime_4;RICH Hit Y "
                     "[cm];TofTrack Y [cm];Entries",
                     84, -25.2, 25.2, 200, -80, 80);

  fHM->Create2<TH2D>("fhTofTrackRichHitX_oBetacuts_dtime_6",
                     "fhTofTrackRichHitX_oBetacuts_dtime_6;RICH Hit X "
                     "[cm];TofTrack X [cm];Entries",
                     67, -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackRichHitY_oBetacuts_dtime_6",
                     "fhTofTrackRichHitY_oBetacuts_dtime_6;RICH Hit Y "
                     "[cm];TofTrack Y [cm];Entries",
                     84, -25.2, 25.2, 200, -80, 80);

  fHM->Create2<TH2D>("fhTofTrackRichHitX_oBetacuts_dtime_8",
                     "fhTofTrackRichHitX_oBetacuts_dtime_8;RICH Hit X "
                     "[cm];TofTrack X [cm];Entries",
                     67, -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackRichHitY_oBetacuts_dtime_8",
                     "fhTofTrackRichHitY_oBetacuts_dtime_8;RICH Hit Y "
                     "[cm];TofTrack Y [cm];Entries",
                     84, -25.2, 25.2, 200, -80, 80);

  fHM->Create2<TH2D>("fhTofTrackRichHitX_oBetacuts_dtime_10",
                     "fhTofTrackRichHitX_oBetacuts_dtime_10;RICH Hit X "
                     "[cm];TofTrack X [cm];Entries",
                     67, -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackRichHitY_oBetacuts_dtime_10",
                     "fhTofTrackRichHitY_oBetacuts_dtime_10;RICH Hit Y "
                     "[cm];TofTrack Y [cm];Entries",
                     84, -25.2, 25.2, 200, -80, 80);

  fHM->Create2<TH2D>("fhTofTrackRichRingHitX", "fhTofTrackRichRingHitX;RICH Ring Hit X [cm];TofTrack X [cm];Entries",
                     67, -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackRichRingHitY", "fhTofTrackRichRingHitY;RICH Ring Hit Y [cm];TofTrack Y [cm];Entries",
                     84, -25.2, 25.2, 200, -80, 80);

  fHM->Create2<TH2D>("fhTofHitRichRingHitX", "fhTofHitRichRingHitX;RICH Ring Hit X [cm];Tof Hit X [cm];Entries", 67,
                     -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofHitRichRingHitY", "fhTofHitRichRingHitY;RICH Ring Hit Y [cm];Tof Hit Y [cm];Entries", 84,
                     -25.2, 25.2, 200, -80, 80);

  //Tof Rich correlation
  fHM->Create2<TH2D>("fhTofRichX_zoomed", "fhTofRichX_zoomed;Rich Hit X [cm];TofHit X [cm];Entries", 27,
                     -8.1 + fXOffsetHisto, 8.1 + fXOffsetHisto, 180, -15, 75);
  fHM->Create2<TH2D>("fhTofRichY_zoomed", "fhTofRichY_zoomed;Rich Hit Y [cm];TofHit Y [cm];Entries", 14, 7.8, 16.2, 30,
                     -5., 25);

  fHM->Create2<TH2D>("fhClosTrackRingX", "fhClosTrackRingX;Rich Ring center X [cm];Tof Track X [cm];Entries", 67,
                     -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhClosTrackRingY", "fhClosTrackRingY;Rich Ring center Y [cm];Tof Track Y [cm];Entries", 84, -25.2,
                     25.2, 200, -80, 80);

  fHM->Create2<TH2D>("fhTofRichRingX", "fhTofRichRingX;Rich Ring Center X [cm];TofHit X [cm];Entries", 100,
                     -20 + fXOffsetHisto, 20 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofRichRingY", "fhTofRichRingY;Ring Ring Center Y [cm];TofHit Y [cm];Entries", 125, -25, 25,
                     200, -80, 80);
  fHM->Create2<TH2D>("fhTofRichRingXZ", "fhTofRichXZ; Z [cm];Hit/Ring X [cm];Entries", 140, 230, 370, 400, -50, 110);

  fHM->Create2<TH2D>("fhTofTrackRichRingXY", "fhTofTrackRichRingXY; X [cm]; Y [cm];Entries", 100, -20 + fXOffsetHisto,
                     20 + fXOffsetHisto, 120, -10,
                     20);  //1bin == 2mm
  fHM->Create2<TH2D>("fhTofClosTrackRichRingXY", "fhTofClosTrackRichRingXY; X [cm]; Y [cm];Entries", 100,
                     -20 + fXOffsetHisto, 20 + fXOffsetHisto, 120, -10,
                     20);  //1bin == 2mm

  fHM->Create3<TH3D>("fhTofXYZ", "fhTofXYZ;Tof Hit X [cm];TofHit Z [cm];Tof Hit Y [cm];Entries", 100, -20, 20, 141,
                     230., 370., 100, -20, 20);
  fHM->Create2<TH2D>("fhTofHitsXY", "fhTofHitsXY;Tof Hit X [cm];Tof Hit Y [cm];Entries", 100, -20, 20, 200, -80, 80);

  for (auto moduleIds = 0; moduleIds < 5; moduleIds++) {
    fHM->Create2<TH2D>(Form("fhTofHitsXY_%u", moduleIds),
                       Form("fhTofHitsXY_%u;Tof Hit X [cm];Tof Hit Y [cm];Entries", moduleIds), 200, -100, 100, 200,
                       -80, 80);
    fHM->Create2<TH2D>(Form("fhTofHitsZX_%u", moduleIds),
                       Form("fhTofHitsZX_%u;Tof Hit X [cm];Tof Hit Y [cm];Entries", moduleIds), 200, 200, 400, 200,
                       -100, 100);
    fHM->Create2<TH2D>(Form("fhTofRichX_%u", moduleIds),
                       Form("fhTofRichX_%u;Rich Hit X [cm];TofHit X [cm];Entries", moduleIds), 67,
                       -20.1 + fXOffsetHisto, 20.1 + fXOffsetHisto, 400, -50, 110);
    fHM->Create2<TH2D>(Form("fhTofRichY_%u", moduleIds),
                       Form("fhTofRichY_%u;Rich Hit Y [cm];TofHit Y [cm];Entries", moduleIds), 84, -25.2, 25.2, 200,
                       -80, 80);
    fHM->Create2<TH2D>(Form("fhTofRichHitsResidual_%u", moduleIds),
                       Form("fhTofRichHitsResidual_%u;Rich-Tof hit X [cm];Rich-Tof hit  Y [cm];Entries", moduleIds),
                       150, -15.0, 15.0, 150, -15.0, 15.0);
  }

  fHM->Create1<TH1D>("fhTofHitsZ", "fhTofHitsZ;Z [cm];Entries", 350, -0.5, 349.5);
  fHM->Create2<TH2D>("fhTofHitsXZ", "fhTofHitsXZ;Z [cm];X [cm];Entries", 350, -0.5, 349.5, 400, -50, 110);
  //Tof Tracks
  fHM->Create1<TH1D>("fhTofTracksPerEvent", "fhTofTracksPerEvent;NofTracks/Event;Entries", 20, -0.5, 19.5);
  fHM->Create1<TH1D>("fhTofTracksPerRichEvent", "fhTofTracksPerRichEvent;NofTracks/RichEvent;Entries", 20, -0.5, 19.5);
  fHM->Create2<TH2D>("fhTofTracksXY", "fhTofTracksXY;X[cm];Y[cm];NofTracks/cm^2", 250, -100, 150, 300, -150,
                     150);  //50 , -20 + fXOffsetHisto, 30 + fXOffsetHisto, 180,-90,90); // projected in RICH Plane

  fHM->Create2<TH2D>("fhTofTracksXY_Target", "fhTofTracksXY_Target;X[cm];Y[cm];NofTracks/cm^2", 100, -50, 50, 100, -50,
                     50);  //50 , -20 + fXOffsetHisto, 30 + fXOffsetHisto, 180,-90,90); // projected in Z=0
  fHM->Create2<TH2D>("fhGoodRingsXY_TargetPos", "fhGoodRingsXY_TargetPos;X[cm];Y[cm];NofTracks/cm^2", 100, -50, 50, 100,
                     -50,
                     50);  //50 , -20 + fXOffsetHisto, 30 + fXOffsetHisto, 180,-90,90); // projected in Z=0

  fHM->Create2<TH2D>("fhTofTrackRichRingX", "fhTofTrackRichRingX;Rich Ring Center X [cm];TofTrack X [cm];Entries", 100,
                     -20 + fXOffsetHisto, 20 + fXOffsetHisto, 400, -50, 110);
  fHM->Create2<TH2D>("fhTofTrackRichRingY", "fhTofTrackRichRingY;Ring Ring Center Y [cm];TofTrack Y [cm];Entries", 125,
                     -25, 25, 200, -80, 80);

  fHM->Create2<TH2D>("fhTofTracksXYRICH", "fhTofTracksXYRICH;X[cm];Y[cm];NofTracks/cm^2", 50, -20 + fXOffsetHisto,
                     30 + fXOffsetHisto, 180, -90,
                     90);  // projected in RICH Plane
  fHM->Create1<TH1D>("fhNofTofTracks", "fhNofTofTracks;X[cm];Y[cm];NofTracks", 4, 0,
                     4);  // 1: All 2: left; 3: right; 4: RICH

  fHM->Create1<TH1D>("fhRingTrackDistance", "fhRingTrackDistance;RingTrackDistance [cm];Entries", 31, -0.25, 14.75);
  fHM->Create1<TH1D>("fhRingTrackDistance_X", "fhRingTrackDistance_X;RingTrackDistance X [cm];Entries", 100, -25.0,
                     25.0);
  fHM->Create1<TH1D>("fhRingTrackDistance_Y", "fhRingTrackDistance_Y;RingTrackDistance Y [cm];Entries", 100, -25.0,
                     25.0);
  fHM->Create2<TH2D>("fhRingTrackDistance_XY",
                     "fhRingTrackDistance_XY;RingTrackDistance X [cm]; RingTrackDistance Y [cm]; Entries", 100, -25.0,
                     25.0, 100, -25.0, 25.0);

  fHM->Create1<TH1D>("fhTrackRingDistance", "fhTrackRingDistance;TrackRingDistance [cm];Entries", 31, -0.5, 30.5);
  fHM->Create1<TH1D>("fhTrackRingDistanceOnTarget", "fhTrackRingDistanceOnTarget;TrackRingDistance [cm];Entries", 31,
                     -0.5, 30.5);
  fHM->Create1<TH1D>("fhTrackRingDistanceOffTarget", "fhTrackRingDistanceOffTarget;TrackRingDistance [cm];Entries", 31,
                     -0.5, 30.5);
  fHM->Create2<TH2D>("fhTrackRingDistanceVSRingradius",
                     "fhTrackRingDistanceVSRingradius;TrackRingDistance "
                     "[cm];RingRadius [cm];Entries",
                     81, -0.5, 80.5, 100, 0., 7.);

  fHM->Create2<TH2D>("fhTrackRingDistanceVSRingChi2",
                     "fhTrackRingDistanceVSRingChi2;TrackRingDistance [cm];\\Chi^2;Entries", 60, 0, 30, 101, 0., 10.1);
  fHM->Create2<TH2D>("fhTrackRingDistanceVSRingChi2_goodRing",
                     "fhTrackRingDistanceVSRingChi2_goodRing;TrackRingDistance "
                     "[cm];\\Chi^2;Entries",
                     40, 0, 10.0, 101, 0., 10.1);

  fHM->Create1<TH1D>("fhTrackRingDistance_corr", "fhTrackRingDistance_corr;TrackRingDistance [cm];Entries", 31, -0.5,
                     30.5);

  fHM->Create1<TH1D>("fhTofBetaTracksWithHitsNoRing", "fhTofBetaTracksWithHitsNoRing; \\beta;Entries", 301, -1.505,
                     1.505);
  fHM->Create1<TH1D>("fhTofBetaTracksWithHits", "fhTofBetaTracksWithHits; \\beta;Entries", 151, -0.005, 1.505);
  fHM->Create1<TH1D>("fhTofBetaTracksNoRing", "fhTofBetaTracksNoRing; \\beta;Entries", 151, -0.005, 1.505);
  fHM->Create1<TH1D>("fhTofBetaTrackswithClosestRingInRange", "fhTofBetaTrackswithClosestRingInRange; \\beta;Entries",
                     301, -1.505, 1.505);

  fHM->Create1<TH1D>("fhRichRingBeta", "fhRichRingBeta; \\beta;Entries", 151, -0.005, 1.505);
  fHM->Create1<TH1D>("fhRichRingBeta_GoodRing", "fhRichRingBeta_GoodRing; \\beta;Entries", 151, -0.005, 1.505);

  fHM->Create1<TH1D>("fhTofBetaRing", "fhTofBetaRing; \\beta;Entries", 301, -1.505, 1.505);
  fHM->Create1<TH1D>("fhTofBetaAll", "fhTofBetaAll; \\beta;Entries", 301, -1.505, 1.505);
  fHM->Create2<TH2D>("fhTofBetaVsRadius", "fhTofBetaVsRadius; \\beta;ring radius [cm];Entries", 301, -1.505, 1.505, 100,
                     0., 7.);
  fHM->Create2<TH2D>("fhTofBetaRingDist", "fhTofBetaRingDist; \\beta;ring Dist [cm];Entries", 301, -1.505, 1.505, 100,
                     0., 20.);
  fHM->Create1<TH1D>("fhTofBetaAllFullAcc", "fhTofBetaAllFullAcc; \\beta;Entries", 301, -1.505, 1.505);

  fHM->Create1<TH1D>("fhRingDeltaTime", "fhRingDeltaTime; \\Delta Time/ns;Entries", 101, -10.1, 10.1);
  fHM->Create1<TH1D>("fhRingToTs", "fhRingToTs; ToT/ns;Entries", 601, 9.975, 40.025);
  fHM->Create1<TH1D>("fhRingLE", "fhRingLE;LE/ns;Entries", 261, -60.5, 200.5);
  fHM->Create1<TH1D>("fhGoodRingLE", "fhGoodRingLE;LE/ns;Entries", 261, -60.5, 200.5);
  fHM->Create1<TH1D>("fhRingNoClTrackLE", "fhRingNoClTrackLE;LE/ns;Entries", 261, -60.5, 200.5);
  fHM->Create1<TH1D>("fhRingClTrackFarAwayLE", "fhRingClTrackFarAwayLE;LE/ns;Entries", 231, -30.5, 200.5);
  fHM->Create2<TH2D>("fhRingLEvsToT", "fhRingLEvsToT;LE/ns;ToT/ns;Entries", 261, -60.5, 200.5, 601, 9.975, 40.025);

  fHM->Create1<TH1D>("fhInnerRingDeltaTime", "fhInnerRingDeltaTime; \\Delta Time/ns;Entries", 101, -10.1, 10.1);
  fHM->Create1<TH1D>("fhInnerRingToTs", "fhInnerRingToTs; ToT/ns;Entries", 601, 9.975, 40.025);
  fHM->Create1<TH1D>("fhInnerRingLE", "fhInnerRingLE;LE/ns;Entries", 261, -60.5, 200.5);
  fHM->Create1<TH1D>("fhInnerGoodRingLE", "fhInnerGoodRingLE;LE/ns;Entries", 261, -60.5, 200.5);
  fHM->Create1<TH1D>("fhInnerRingNoClTrackLE", "fhInnerRingNoClTrackLE;LE/ns;Entries", 261, -60.5, 200.5);
  fHM->Create1<TH1D>("fhInnerRingClTrackFarAwayLE", "fhInnerRingClTrackFarAwayLE;LE/ns;Entries", 261, -60.5, 200.5);
  fHM->Create1<TH1D>("fhInnerRingFlag", "fhInnerRingFlag;Has|HasNot;Entries", 2, -0.5, 1.5);
  fHM->Create1<TH1D>("fhNofInnerHits", "fhNofInnerHits;#Hits;Entries", 31, -0.5, 30.5);

  fHM->Create1<TH1D>("fhDiRICHsInRegion", "fhNofInnerHits;#Hits;DiRICH", 4096, 28672, 32767);

  fHM->Create1<TH1D>("fhBlobTrackDistX", "fhBlobTrackDistX; |TofTrackX - MAPMT center X| [cm];Entries", 30, -0.5, 29.5);
  fHM->Create1<TH1D>("fhBlobTrackDistY", "fhBlobTrackDistY; |TofTrackY - MAPMT center Y| [cm];Entries", 30, -0.5, 29.5);
  fHM->Create1<TH1D>("fhBlobTrackDist", "fhBlobTrackDist; |TofTrack - MAPMT center dist| [cm];Entries", 30, -0.5, 29.5);

  fHM->Create1<TH1D>("fhNofBlobEvents", "fhNofBlobEvents;;#Events with min. one Blob", 1, 0.5, 1.5);
  fHM->Create1<TH1D>("fhNofBlobsInEvent", "fhNofBlobsInEvent;#Blobs in Event;Entries", 36, 0.5, 36.5);

  fHM->Create1<TH1D>("fhRichDigisConsecTime", "fhRichDigisConsecTime;consecutive time [ns];Entries", 500, -0.5, 499.5);
  fHM->Create1<TH1D>("fhRichDigisConsecTimeTOT", "fhRichDigisConsecTimeTOT;consecutive time [ns];Entries", 500, -0.5,
                     499.5);

  fHM->Create1<TH1D>("fhNofHitsInGoodRing", "fhNofHitsInGoodRing;# hits in ring;Entries", 50, -0.5, 49.5);
  fHM->Create1<TH1D>("fhTracksWithRings", "fhTracksWithRings;Scenarios;Entries", 5, -0.5, 4.5);

  fHM->Create1<TH1D>("fhRichRingChi2", "fhRichRingChi2;\\Chi^2;Entries", 101, 0., 10.1);
  fHM->Create1<TH1D>("fhRichRingChi2_goodRing", "fhRichRingChi2_goodRing;\\Chi^2;Entries", 101, 0., 10.1);

  fHM->Create2<TH2D>("fhTofTracksXYRICH_Accectance", "fhTofTracksXYRICH_Accectance;X[cm];Y[cm];NofTracks/cm^2", 50,
                     -20 + fXOffsetHisto, 30 + fXOffsetHisto, 180, -90,
                     90);  // projected in RICH Plane

  fHM->Create1<TH1D>("fhHitTimeEvent", "fhHitTimeEvent;time [ns];Entries", 300, -100., 200);

  for (auto i = 0; i < 5; ++i)
    fHM->Create2<TH2D>(Form("fhTofHitXZ_Station_%u", i), Form("fhTofHitXZ_Station_%u;Z [cm];X [cm];Entries", i), 350,
                       -0.5, 349.5, 400, -50, 110);

  fHM->Create1<TH1D>("fhBmonDigiMultiplicity", "fhBmonDigiMultiplicity;multiplicity; Entries", 16, -1.5, 14.5);
  fHM->Create1<TH1D>("fhBmonDigiTime", "fhBmonDigiTime;time [ns]; Entries", 500, 0., 100.);
  fHM->Create1<TH1D>("fhBmonDigiTimeEvent", "fhBmonDigiTimeEvent;time [ns]; Entries", 500, 0., 100.);

  //Hit Time Plots
  if (fDoTimePlots) {
    fHM->Create1<TH1D>("fhHitTimeMeanRichHit", "fhHitTimeMeanRichHit;time [ns];Entries", 600, -30., 30);
    fHM->Create1<TH1D>("fhHitTimeMeanRichHitVsEvent", "fhHitTimeMeanRichHitVsEvent;time [ns];Entries", 300, -100., 200);
    fHM->Create1<TH1D>("fhHitTimeMeanTofHitVsEvent", "fhHitTimeMeanTofHitVsEvent;time [ns];Entries", 300, -100., 200);
    fHM->Create1<TH1D>("fhHitTimeMeanRichHitVsMeanTof", "fhHitTimeMeanRichHitVsMeanTof;time [ns];Entries", 300, -100.,
                       200);

    for (auto i = 0; i < 72; ++i)
      fHM->Create1<TH1D>(Form("/HitTime/fhHitTimeEvent_%u", i),
                         Form("/HitTime/fhHitTimeEvent_%u; time [ns];Entries", i), 300, -100, 200);

    for (auto i = 0; i < 72; ++i)
      fHM->Create1<TH1D>(Form("/HitTime/fhHitTimeMeanRichHit_%u", i),
                         Form("/HitTime/fhHitTimeMeanRichHit_%u; time [ns];Entries", i), 300, -30, 30);

    for (auto i = 0; i < 72; ++i)
      fHM->Create2<TH2D>(Form("/HitTime_2D/fhHitTimeEvent_%u", i),
                         Form("/HitTime_2D/fhHitTimeEvent_%u; time [ns], ToT [ns];Entries", i), 300, -100, 200, 30, 15,
                         30);

    for (auto i = 0; i < 33; ++i)
      fHM->Create1<TH1D>(Form("/HitTime_chnl/fhHitTimeEvent_chnl_%u", i),
                         Form("/HitTime_chnl/fhHitTimeEvent_chnl_%u; time [ns];Entries", i), 300, -100, 200);
  }  // End Hit Time Plots
}


void CbmRichMCbmQaReal::Exec(Option_t* /*option*/)
{
  fEventNum++;
  fHM->H1("fhNofEvents")->Fill(1);
  cout << "CbmRichMCbmQaReal, event No. " << fEventNum << endl;

  uint64_t tsStartTime = 0;
  if (nullptr != fTSHeader) tsStartTime = fTSHeader->GetTsStartTime();

  if (fDigiHitsInitialized == false) {
    auto nOfCbmEvent = fCbmEvent->GetEntriesFast();
    if (nOfCbmEvent > 0) {
      CbmEvent* ev = static_cast<CbmEvent*>(fCbmEvent->At(0));
      /*if ((fDigiMan->GetNofDigis(ECbmModuleId::kRich)> 10) &&
             //(fDigiMan->GetNofDigis(ECbmModuleId::kSts) > 30) ||
             //(fRichRings->GetEntriesFast()> 0) ||
             (fDigiMan->GetNofDigis(ECbmModuleId::kTof) > 10)
            )*/
      if (ev != nullptr) {
        double minTime = ev->GetStartTime();  //std::numeric_limits<double>::max();
        /* for (int i = 0; i < fDigiMan->GetNofDigis(ECbmModuleId::kRich); i++) {
                    const CbmRichDigi* richDigi = fDigiMan->Get<CbmRichDigi>(i);
                    // fHM->H1("fhRichDigisToT")->Fill(richDigi->GetToT());
                    if (richDigi->GetTime() < minTime) minTime = richDigi->GetTime();
                }*/

        double dT      = 40e9;
        double dTZoom1 = 0.8e9;
        double dTZoom2 = 0.008e9;
        fHM->H1("fhRichDigisTimeLog")->GetXaxis()->SetLimits(minTime, minTime + dT);
        fHM->H1("fhRichDigisTimeLogZoom")->GetXaxis()->SetLimits(minTime, minTime + dTZoom1);
        fHM->H1("fhRichDigisTimeLogZoom2")->GetXaxis()->SetLimits(minTime, minTime + dTZoom2);

        fHM->H1("fhRichRingsTimeLog")->GetXaxis()->SetLimits(minTime, minTime + dT);
        fHM->H1("fhRichRingsTimeLogZoom")->GetXaxis()->SetLimits(minTime, minTime + dTZoom1);
        fHM->H1("fhRichRingsTimeLogZoom2")->GetXaxis()->SetLimits(minTime, minTime + dTZoom2);

        fHM->H1("fhTofDigisTimeLog")->GetXaxis()->SetLimits(minTime, minTime + dT);
        fHM->H1("fhTofDigisTimeLogZoom")->GetXaxis()->SetLimits(minTime, minTime + dTZoom1);
        fHM->H1("fhTofDigisTimeLogZoom2")->GetXaxis()->SetLimits(minTime, minTime + dTZoom2);

        fHM->H1("fhStsDigisTimeLog")->GetXaxis()->SetLimits(minTime, minTime + dT);
        fHM->H1("fhStsDigisTimeLogZoom")->GetXaxis()->SetLimits(minTime, minTime + dTZoom1);
        fHM->H1("fhStsDigisTimeLogZoom2")->GetXaxis()->SetLimits(minTime, minTime + dTZoom2);

        fHM->H1("fhTrd1dDigisTimeLog")->GetXaxis()->SetLimits(minTime, minTime + dT);
        fHM->H1("fhTrd1dDigisTimeLogZoom")->GetXaxis()->SetLimits(minTime, minTime + dTZoom1);
        fHM->H1("fhTrd1dDigisTimeLogZoom2")->GetXaxis()->SetLimits(minTime, minTime + dTZoom2);

        fHM->H1("fhTrd2dDigisTimeLog")->GetXaxis()->SetLimits(minTime, minTime + dT);
        fHM->H1("fhTrd2dDigisTimeLogZoom")->GetXaxis()->SetLimits(minTime, minTime + dTZoom1);
        fHM->H1("fhTrd2dDigisTimeLogZoom2")->GetXaxis()->SetLimits(minTime, minTime + dTZoom2);

        fHM->H1("fhBmonDigisTimeLog")->GetXaxis()->SetLimits(minTime, minTime + dT);
        fHM->H1("fhBmonDigisTimeLogZoom")->GetXaxis()->SetLimits(minTime, minTime + dTZoom1);
        fHM->H1("fhBmonDigisTimeLogZoom2")->GetXaxis()->SetLimits(minTime, minTime + dTZoom2);

        fTSMinTime           = tsStartTime;
        fDigiHitsInitialized = true;
      }
    }
  }  // if (fDigiHitsInitialized == false)

  if (fDigiHitsInitialized == true) {

    double TsTimeAfterStart = static_cast<double>(tsStartTime - fTSMinTime);
    int nofRichDigis        = fDigiMan->GetNofDigis(ECbmModuleId::kRich);
    fHM->H1("fhNofRichDigisInTimeslice")->Fill(nofRichDigis);
    for (int i = 0; i < nofRichDigis; i++) {
      const CbmRichDigi* digi = fDigiMan->Get<CbmRichDigi>(i);
      fHM->H1("fhRichDigisTimeLog")->Fill(digi->GetTime() + TsTimeAfterStart);
      fHM->H1("fhRichDigisTimeLogZoom")->Fill(digi->GetTime() + TsTimeAfterStart);
      fHM->H1("fhRichDigisTimeLogZoom2")->Fill(digi->GetTime() + TsTimeAfterStart);
    }

    int nofRichRings = fRichRings->GetEntriesFast();
    for (int i = 0; i < nofRichRings; i++) {
      CbmRichRing* ring = static_cast<CbmRichRing*>(fRichRings->At(i));
      fHM->H1("fhRichRingsTimeLog")->Fill(ring->GetTime() + TsTimeAfterStart);
      fHM->H1("fhRichRingsTimeLogZoom")->Fill(ring->GetTime() + TsTimeAfterStart);
      fHM->H1("fhRichRingsTimeLogZoom2")->Fill(ring->GetTime() + TsTimeAfterStart);
    }

    int nofTofDigis = fDigiMan->GetNofDigis(ECbmModuleId::kTof);
    for (int i = 0; i < nofTofDigis; i++) {
      const CbmTofDigi* digi = fDigiMan->Get<CbmTofDigi>(i);
      fHM->H1("fhTofDigisTimeLog")->Fill(digi->GetTime() + TsTimeAfterStart);
      fHM->H1("fhTofDigisTimeLogZoom")->Fill(digi->GetTime() + TsTimeAfterStart);
      fHM->H1("fhTofDigisTimeLogZoom2")->Fill(digi->GetTime() + TsTimeAfterStart);
    }

    if (fDigiMan->IsPresent(ECbmModuleId::kSts)) {
      int nofStsDigis = fDigiMan->GetNofDigis(ECbmModuleId::kSts);
      for (int i = 0; i < nofStsDigis; i++) {
        const CbmStsDigi* digi = fDigiMan->Get<CbmStsDigi>(i);
        fHM->H1("fhStsDigisTimeLog")->Fill(digi->GetTime() + TsTimeAfterStart);
        fHM->H1("fhStsDigisTimeLogZoom")->Fill(digi->GetTime() + TsTimeAfterStart);
        fHM->H1("fhStsDigisTimeLogZoom2")->Fill(digi->GetTime() + TsTimeAfterStart);
      }
    }

    if (fDigiMan->IsPresent(ECbmModuleId::kTrd)) {
      int nofTrd1dDigis = fDigiMan->GetNofDigis(ECbmModuleId::kTrd);
      for (int i = 0; i < nofTrd1dDigis; i++) {
        const CbmTrdDigi* digi = fDigiMan->Get<CbmTrdDigi>(i);

        CbmTrdDigi::eCbmTrdAsicType trdDigiType = digi->GetType();
        if (trdDigiType == CbmTrdDigi::eCbmTrdAsicType::kSPADIC) {
          fHM->H1("fhTrd1dDigisTimeLog")->Fill(digi->GetTime() + TsTimeAfterStart);
          fHM->H1("fhTrd1dDigisTimeLogZoom")->Fill(digi->GetTime() + TsTimeAfterStart);
          fHM->H1("fhTrd1dDigisTimeLogZoom2")->Fill(digi->GetTime() + TsTimeAfterStart);
        }
        else if (trdDigiType == CbmTrdDigi::eCbmTrdAsicType::kFASP) {
          fHM->H1("fhTrd2dDigisTimeLog")->Fill(digi->GetTime() + TsTimeAfterStart);
          fHM->H1("fhTrd2dDigisTimeLogZoom")->Fill(digi->GetTime() + TsTimeAfterStart);
          fHM->H1("fhTrd2dDigisTimeLogZoom2")->Fill(digi->GetTime() + TsTimeAfterStart);
        }
      }
    }

    {
      Int_t nrBmonDigis = 0;
      if (fBmonDigis) nrBmonDigis = fBmonDigis->size();
      //else if ( fBmonDigiArr ) nrBmonDigis = fBmonDigiArr->GetEntriesFast();
      LOG(debug) << "BmonDigis: " << nrBmonDigis;

      for (Int_t iBmon = 0; iBmon < nrBmonDigis; ++iBmon) {
        const CbmTofDigi* BmonDigi = nullptr;
        if (fBmonDigis) BmonDigi = &(fBmonDigis->at(iBmon));
        //else if ( fBmonDigiArr ) BmonDigi = dynamic_cast<const CbmTofDigi*>(fBmonDigiArr->At(iBmon));
        assert(BmonDigi);
        fHM->H1("fhBmonDigisTimeLog")->Fill(BmonDigi->GetTime() + TsTimeAfterStart);
        fHM->H1("fhBmonDigisTimeLogZoom")->Fill(BmonDigi->GetTime() + TsTimeAfterStart);
        fHM->H1("fhBmonDigisTimeLogZoom2")->Fill(BmonDigi->GetTime() + TsTimeAfterStart);
      }
    }
  }

  int nofRichHits = fRichHits->GetEntriesFast();
  fHM->H1("fhNofRichHitsInTimeslice")->Fill(nofRichHits);
  fHM->H1("fhHitsInTimeslice")->Fill(fEventNum, nofRichHits);

  {
    int nofRichDigis = fDigiMan->GetNofDigis(ECbmModuleId::kRich);
    for (int i = 0; i < nofRichDigis; i++) {
      const CbmRichDigi* richDigi = fDigiMan->Get<CbmRichDigi>(i);
      fHM->H1("fhRichDigisToT")->Fill(richDigi->GetToT());

      if (i > 0) {
        const CbmRichDigi* richDigi_prev = fDigiMan->Get<CbmRichDigi>(i - 1);
        fHM->H1("fhRichDigisConsecTime")->Fill(richDigi->GetTime() - richDigi_prev->GetTime());
        if (doToT(richDigi) && doToT(richDigi_prev))
          fHM->H1("fhRichDigisConsecTimeTOT")->Fill(richDigi->GetTime() - richDigi_prev->GetTime());
      }

      //fHM->H2("fhRichDigiPixelRate")->Fill(richDigi->GetX(),richDigi->GetY());
    }
  }

  Double_t zPos_tmp = 0.;
  for (int iH = 0; iH < nofRichHits; iH++) {
    CbmRichHit* richHit = static_cast<CbmRichHit*>(fRichHits->At(iH));
    fHM->H2("fhRichHitXY")->Fill(richHit->GetX(), richHit->GetY());
    fHM->H1("fhRichHitToT")->Fill(richHit->GetToT());
    zPos_tmp = ((zPos_tmp * iH) + richHit->GetZ()) / (iH + 1);
    //printf("HitToT: %f \n", richHit->GetToT());
  }
  if (nofRichHits > 0) RichZPos = zPos_tmp;

  //std::cout<<"[INFO] Mean Z-Position of RICH Hits: "<<RichZPos<<std::endl;
  //CBMEVENT
  auto fNCbmEvent = fCbmEvent->GetEntriesFast();

  for (int i = 0; i < fNCbmEvent; i++) {
    CbmEvent* ev = static_cast<CbmEvent*>(fCbmEvent->At(i));
    if (fTriggerRichHits != 0 && (Int_t(ev->GetNofData(ECbmDataType::kRichHit)) < fTriggerRichHits)) continue;
    fHM->H1("fhNofCbmEvents")->Fill(1);
    //if (fTriggerTofHits != 0 && (ev->GetNofData(ECbmDataType::kTofHit) < fTriggerTofHits)) continue;

    //if (ev->GetNofData(ECbmDataType::kTofHit)  > (fTriggerTofHits+10) ) continue;

    std::vector<int> ringIndx;
    std::vector<int> evRichHitIndx;
    std::array<uint32_t, 36> pmtHits;
    for (auto& a : pmtHits)
      a = 0;

    fEventPnt          = ev;
    fCbmEventStartTime = ev->GetStartTime();

    // Scan Event to find first Digi that triggered.
    //         std::cout<<"Sts Digis:"<< ev->GetNofData(kStsDigi)<<std::endl;
    //         std::cout<<"Much Digis:"<< ev->GetNofData(kMuchDigi)<<std::endl;
    //         std::cout<<"Tof Digis:"<< ev->GetNofData(kTofDigi)<<std::endl;
    //         std::cout<<"Rich Digis:"<< ev->GetNofData(kRichDigi)<<std::endl;
    //         std::cout<<"Psd Digis:"<< ev->GetNofData(kPsdDigi)<<std::endl;
    //        unsigned int flagRich = 0;
    Double_t startTime = std::numeric_limits<Double_t>::max();

    fHM->H1("fhBmonDigiMultiplicity")->Fill(ev->GetNofData(ECbmDataType::kBmonDigi));
    //std::cout<<ev->GetNofData(ECbmDataType::kBmonDigi)<<std::endl;
    for (size_t j = 0; j < ev->GetNofData(ECbmDataType::kBmonDigi); j++) {
      auto iBmonDigi             = ev->GetIndex(ECbmDataType::kBmonDigi, j);
      const CbmTofDigi* BmonDigi = nullptr;
      if (fBmonDigis) BmonDigi = &(fBmonDigis->at(iBmonDigi));
      assert(BmonDigi);
      fHM->H1("fhBmonDigiTimeEvent")->Fill(BmonDigi->GetTime() - fCbmEventStartTime);
    }

    //std::cout<<"NofBmonDigis in TS: "<< fBmonDigis->size() <<std::endl;
    Double_t minT0TimeDiff = std::numeric_limits<Double_t>::max();
    //Double_t T0Time        = std::numeric_limits<Double_t>::max();
    for (uint32_t j = 0; j < fBmonDigis->size(); j++) {
      const CbmTofDigi* BmonDigi = nullptr;
      if (fBmonDigis) BmonDigi = &(fBmonDigis->at(j));
      assert(BmonDigi);
      Double_t timeDiffBmon = BmonDigi->GetTime() - fCbmEventStartTime;
      if (std::fabs(timeDiffBmon) < std::fabs(minT0TimeDiff)) {
        minT0TimeDiff = timeDiffBmon;
        //T0Time        = BmonDigi->GetTime();
      }
    }
    //std::cout<<"BmonDigistime: "<< T0Time << "  EventStartTime:" << fCbmEventStartTime <<"   Time DIff: "<< minT0TimeDiff <<std::endl;
    fHM->H1("fhBmonDigiTime")->Fill(minT0TimeDiff);

    Double_t meanRichHitTime = 0.;
    if (fDoTimePlots) {
      for (size_t j = 0; j < ev->GetNofData(ECbmDataType::kRichHit); j++) {
        auto iRichHit       = ev->GetIndex(ECbmDataType::kRichHit, j);
        CbmRichHit* richHit = static_cast<CbmRichHit*>(fRichHits->At(iRichHit));
        meanRichHitTime += richHit->GetTime();
      }
      if (ev->GetNofData(ECbmDataType::kRichHit) > 0) meanRichHitTime /= ev->GetNofData(ECbmDataType::kRichHit);

      Double_t meanTofHitTime = 0.;
      for (size_t j = 0; j < ev->GetNofData(ECbmDataType::kTofHit); j++) {
        auto iTofHit      = ev->GetIndex(ECbmDataType::kTofHit, j);
        CbmTofHit* tofHit = static_cast<CbmTofHit*>(fTofHits->At(iTofHit));
        meanTofHitTime += tofHit->GetTime();
      }
      if (ev->GetNofData(ECbmDataType::kTofHit) > 0) meanTofHitTime /= ev->GetNofData(ECbmDataType::kTofHit);

      fHM->H1("fhHitTimeMeanRichHitVsEvent")->Fill(meanRichHitTime - fCbmEventStartTime);
      fHM->H1("fhHitTimeMeanTofHitVsEvent")->Fill(meanTofHitTime - fCbmEventStartTime);
      fHM->H1("fhHitTimeMeanRichHitVsMeanTof")->Fill(meanRichHitTime - meanTofHitTime);
    }

    for (size_t j = 0; j < ev->GetNofData(ECbmDataType::kRichHit); j++) {
      auto iRichHit = ev->GetIndex(ECbmDataType::kRichHit, j);
      evRichHitIndx.push_back(iRichHit);
      CbmRichHit* richHit = static_cast<CbmRichHit*>(fRichHits->At(iRichHit));
      uint32_t pmtId      = (((richHit->GetAddress()) >> 24) & 0xF) * 9 + (((richHit->GetAddress()) >> 20) & 0xF);
      fHM->H1("fhHitTimeEvent")->Fill(richHit->GetTime() - fCbmEventStartTime);

      if (fDoTimePlots) {
        uint32_t DiRichId = (((richHit->GetAddress()) >> 24) & 0xF) * 18 + (((richHit->GetAddress()) >> 20) & 0xF) * 2
                            + (((richHit->GetAddress()) >> 16) & 0xF);

        fHM->H1(Form("/HitTime/fhHitTimeEvent_%u", DiRichId))->Fill(richHit->GetTime() - fCbmEventStartTime);
        fHM->H1(Form("/HitTime/fhHitTimeMeanRichHit_%u", DiRichId))->Fill(richHit->GetTime() - meanRichHitTime);
        fHM->H1("fhHitTimeMeanRichHit")->Fill(richHit->GetTime() - meanRichHitTime);

        fHM->H2(Form("/HitTime_2D/fhHitTimeEvent_%u", DiRichId))
          ->Fill(richHit->GetTime() - fCbmEventStartTime, richHit->GetToT());
        if (DiRichId == 52)
          fHM->H1(Form("/HitTime_chnl/fhHitTimeEvent_chnl_%u", richHit->GetAddress() & 0xFFFF))
            ->Fill(richHit->GetTime() - fCbmEventStartTime);
      }

      fHM->H1("fhRichHitToTEvent")->Fill(richHit->GetToT());
      fHM->H2("fhRichHitXYEvent")->Fill(richHit->GetX(), richHit->GetY());
      //Blob finder
      pmtHits[pmtId]++;

      //std::cout<<"\t\t *  "<<i<<". Event, Hit "<< j <<": "<< iRichHit <<"\t " << std::fixed << std::setprecision(5) << richHit->GetTime() <<std::endl;
      if (richHit->GetTime() < startTime) {
        startTime = richHit->GetTime(); /*flagRich = 1;*/
      }
      int nofRichRings2 = fRichRings->GetEntriesFast();
      for (int l = 0; l < nofRichRings2; l++) {
        CbmRichRing* ring = static_cast<CbmRichRing*>(fRichRings->At(l));
        auto NofRingHits  = ring->GetNofHits();
        for (int m = 0; m < NofRingHits; m++) {
          auto RingHitIndx = ring->GetHit(m);
          if (RingHitIndx == iRichHit) {
            Bool_t used = false;
            for (auto check : ringIndx) {
              if (check == l) {
                used = true;
                break;
              }
            }
            if (used == false) ringIndx.push_back(l);
            break;
          }
        }
      }

      for (size_t k = 0; k < ev->GetNofData(ECbmDataType::kTofHit); k++) {
        auto iTofHit      = ev->GetIndex(ECbmDataType::kTofHit, k);
        CbmTofHit* tofHit = static_cast<CbmTofHit*>(fTofHits->At(iTofHit));
        if (tofHit->GetTime() < startTime) {
          startTime = tofHit->GetTime(); /* flagRich = 0;*/
        }
        if (tofHit->GetZ() < 2.) continue;  // Cut Bmon away!

        auto TofModulId   = (tofHit->GetAddress() >> 4) & 0x3F;
        auto TofModulType = (tofHit->GetAddress() >> 11) & 0xF;
        //std::cout<<std::hex<<tofHit->GetAddress()<<std::dec<<"  Type: "<<TofModulType<<"  Id: "<<TofModulId<<std::endl;
        if (TofModulType != 0) continue;
        //fHM->H2("fhTofRichX")->Fill(richHit->GetX(), tofHit->GetX());

        fHM->H2(Form("fhTofHitXZ_Station_%u", TofModulId))->Fill(tofHit->GetZ(), tofHit->GetX());

        //Befor tof_v20b
        /*if (tofHit->GetZ()> 230. && tofHit->GetZ() < 250) fHM->H2("fhTofRichX_stack1")->Fill(richHit->GetX(),tofHit->GetX());
                if (tofHit->GetZ()> 250. && tofHit->GetZ() < 265) fHM->H2("fhTofRichX_stack2")->Fill(richHit->GetX(),tofHit->GetX());
                if (tofHit->GetZ()> 265. && tofHit->GetZ() < 285) fHM->H2("fhTofRichX_stack3")->Fill(richHit->GetX(),tofHit->GetX());
                */
        if (TofModulId == 0) {  //(tofHit->GetZ() > 230. && tofHit->GetZ() < 255)
          fHM->H2("fhTofRichX_stack1")->Fill(richHit->GetX(), tofHit->GetX());
          fHM->H2("fhTofRichY_stack1")->Fill(richHit->GetY(), tofHit->GetY());
          fHM->H2("fhTofRichX")->Fill(richHit->GetX(), tofHit->GetX());
          fHM->H2("fhTofRichY")->Fill(richHit->GetY(), tofHit->GetY());
          fHM->H2("fhTofHitsXY")->Fill(tofHit->GetX(), tofHit->GetY());
        }
        if (TofModulId == 1) {  //(tofHit->GetZ() >= 255. && tofHit->GetZ() < 272)
          fHM->H2("fhTofRichX_stack2")->Fill(richHit->GetX(), tofHit->GetX());
          fHM->H2("fhTofRichY_stack2")->Fill(richHit->GetY(), tofHit->GetY());
          fHM->H2("fhTofRichX")->Fill(richHit->GetX(), tofHit->GetX());
          fHM->H2("fhTofRichY")->Fill(richHit->GetY(), tofHit->GetY());
          fHM->H2("fhTofHitsXY")->Fill(tofHit->GetX(), tofHit->GetY());
        }
        if (TofModulId == 2) {  //(tofHit->GetZ() >= 272. && tofHit->GetZ() < 290)
          fHM->H2("fhTofRichX_stack3")->Fill(richHit->GetX(), tofHit->GetX());
          fHM->H2("fhTofRichY_stack3")->Fill(richHit->GetY(), tofHit->GetY());
          fHM->H2("fhTofRichX")->Fill(richHit->GetX(), tofHit->GetX());
          fHM->H2("fhTofRichY")->Fill(richHit->GetY(), tofHit->GetY());
          fHM->H2("fhTofHitsXY")->Fill(tofHit->GetX(), tofHit->GetY());
        }

        //fHM->H2("fhTofRichX_zoomed")->Fill(richHit->GetX(), tofHit->GetX());
        //fHM->H2("fhTofRichY_zoomed")->Fill(richHit->GetY(), tofHit->GetY());

        //fHM->H2("fhTofHitsXY")->Fill(tofHit->GetX(), tofHit->GetY());

        fHM->H2(Form("fhTofHitsXY_%u", TofModulId))->Fill(tofHit->GetX(), tofHit->GetY());
        fHM->H2(Form("fhTofHitsZX_%u", TofModulId))->Fill(tofHit->GetZ(), tofHit->GetX());
        fHM->H2(Form("fhTofRichX_%u", TofModulId))->Fill(richHit->GetX(), tofHit->GetX());
        fHM->H2(Form("fhTofRichY_%u", TofModulId))->Fill(richHit->GetY(), tofHit->GetY());
        TVector3 hitExtr = extrapolate(tofHit, richHit->GetZ());
        fHM->H2(Form("fhTofRichHitsResidual_%u", TofModulId))
          ->Fill(richHit->GetX() - hitExtr.X(), richHit->GetY() - hitExtr.Y());
      }
    }

    //std::cout<<"Diff in StartTime DigiToHit: "<< startTime - fCbmEventStartTime << "\t" <<flagRich<<std::endl;
    fCbmEventStartTime = startTime;

    if (bSeDisplayRingOnly) {
      if (ringIndx.size() > 0) fSeDisplay->DrawEvent(ev, ringIndx, 1);
    }
    else {
      fSeDisplay->DrawEvent(ev, ringIndx, 1);
    }

    //       std::cout<<DrawCbmEvent<<std::endl;
    // for (int j=0;j<ev->GetNofData(ECbmDataType::kRichDigi);j++){
    //     auto iRichDigi = ev->GetIndex(ECbmDataType::kRichDigi, j);
    //     CbmRichDigi* richDigi = static_cast<CbmRichDigi*>(fRichDigis->At(iRichDigi));
    //       std::cout<<"\t\t *  "<<i<<". Event, Digi "<< j <<": "<< iRichDigi <<"\t " << std::fixed << std::setprecision(5) <<  richDigi->GetTime()<<std::endl;
    // }

    //std::vector<int> evTofTrack;

    //*** Tracks in CbmEvent -> Here Trigger is on! This is seen in Track Position
    //std::cout<<"TRACKS in CbmEvent: "<<ev->GetNofData(ECbmDataType::kTofTrack)<<std::endl;
    auto noftofTracks = ev->GetNofData(ECbmDataType::kTofTrack);
    fHM->H1("fhTofTracksPerEvent")->Fill(noftofTracks);

    for (size_t j = 0; j < noftofTracks; j++) {
      auto iTofTrack         = ev->GetIndex(ECbmDataType::kTofTrack, j);
      CbmTofTracklet* tTrack = static_cast<CbmTofTracklet*>(fTofTracks->At(iTofTrack));
      if (tTrack == nullptr) continue;

      fHM->H1("fhTracksWithRings")->Fill(0);
      fHM->H2("fhTofTracksXY")->Fill(tTrack->GetFitX(RichZPos), tTrack->GetFitY(RichZPos));
      fHM->H2("fhTofTracksXY_Target")->Fill(tTrack->GetFitX(0.), tTrack->GetFitY(0.));
      fHM->H1("fhNofTofTracks")->Fill(0.5);  // 1: All 2: left; 3: right; 4: RICH
      fHM->H1("fhTofBetaAllFullAcc")->Fill(getBeta(tTrack));
      //std::cout<<"beta Track "<< j <<": "<<getBeta(tTrack)<<std::endl;

      if (tTrack->GetFitX(RichZPos) > -10. && tTrack->GetFitX(RichZPos) < +10. && tTrack->GetFitY(RichZPos) > -25.
          && tTrack->GetFitY(RichZPos) < +25. && isOnTarget(tTrack)) {
        //Track in  RICH
        fTracksinRich++;
        Int_t goodHit = 0;
        for (size_t k = 0; k < ev->GetNofData(ECbmDataType::kRichHit); k++) {
          auto iRichHit       = ev->GetIndex(ECbmDataType::kRichHit, k);
          CbmRichHit* richHit = static_cast<CbmRichHit*>(fRichHits->At(iRichHit));
          if (richHit == nullptr) continue;
          if (std::fabs(richHit->GetY() - tTrack->GetFitY(RichZPos)) < 5.
              && std::fabs(richHit->GetX() - tTrack->GetFitX(RichZPos)) < 9.)
            goodHit++;
        }
        if (goodHit > 0) fTracksinRichWithRichHits[0]++;
        if (goodHit > 5) fTracksinRichWithRichHits[1]++;
        if (goodHit > 10) fTracksinRichWithRichHits[2]++;
        if (goodHit > 15) fTracksinRichWithRichHits[3]++;
      }

      if (isAccmRICH(tTrack)) {
        if (RestrictToFullAcc(tTrack)) {
          fHM->H2("fhTofTracksXYRICH")
            ->Fill(tTrack->GetFitX(RichZPos),
                   tTrack->GetFitY(RichZPos));   // projected in RICH Plane
          fHM->H1("fhNofTofTracks")->Fill(3.5);  // 1: All 2: left; 3: right; 4: RICH
          std::vector<int> evTofTrack;
          evTofTrack.push_back(iTofTrack);
          DrawRichTofEv(evRichHitIndx, evTofTrack);

          if (ringIndx.size() == 0 && evRichHitIndx.size() > 0)
            fHM->H1("fhTofBetaTracksWithHitsNoRing")->Fill(getBeta(tTrack));  // no Ring in CbmEvent found
          if (evRichHitIndx.size() > 0) fHM->H1("fhTofBetaTracksWithHits")->Fill(getBeta(tTrack));
          if (ringIndx.size() == 0) fHM->H1("fhTofBetaTracksNoRing")->Fill(getBeta(tTrack));

          if ((tTrack->GetNofHits() == 4 && (getBeta(tTrack) > 0.90 && getBeta(tTrack) < 1.10))) {
            //tracks after cut
            Double_t trackXpos = tTrack->GetFitX(RichZPos);
            Double_t trackYpos = tTrack->GetFitY(RichZPos);

            if (trackXpos > -8 && trackXpos < 13 && trackYpos > -21 && trackYpos < 24) {
              if (!(trackXpos > -8 && trackXpos < -3 && trackYpos > 5 && trackYpos < 7.5)
                  && !(trackXpos > 7.8 && trackXpos < 13 && trackYpos > 5 && trackYpos < 7.5)
                  && !(trackXpos > -8 && trackXpos < 2 && trackYpos > 21 && trackYpos < 24)
                  && !(trackXpos > 7.8 && trackXpos < 13 && trackYpos > 21 && trackYpos < 24)
                  && !(trackXpos > 2.2 && trackXpos < 13 && trackYpos > 21 && trackYpos < 16)) {
                fHM->H2("fhTofTracksXYRICH_Accectance")->Fill(trackXpos, trackYpos);
                fHM->H1("fhTracksWithRings")->Fill(2);
                if (ringIndx.size() > 0) fHM->H1("fhTracksWithRings")->Fill(4);
                if (FindClosestRing(tTrack, ringIndx).first > -1) fHM->H1("fhTracksWithRings")->Fill(3);
              }
            }

            // filter on Rich Acc.
            // filter on
          }

          std::pair<int, double> closeRing = FindClosestRing(tTrack, ringIndx);
          if (closeRing.first > -1) {
            fHM->H1("fhRingTrackDistance")->Fill(closeRing.second);
            fHM->H1("fhTracksWithRings")->Fill(1);
            {
              const CbmRichRing* ring = static_cast<CbmRichRing*>(fRichRings->At(ringIndx[closeRing.first]));

              const int ringX    = ring->GetCenterX();
              const int ringY    = ring->GetCenterY();
              const double xDist = (tTrack->GetFitX(RichZPos) - ringX);
              const double yDist = (tTrack->GetFitY(RichZPos) - ringY);
              fHM->H1("fhRingTrackDistance_X")->Fill(xDist);
              fHM->H1("fhRingTrackDistance_Y")->Fill(yDist);
              fHM->H2("fhRingTrackDistance_XY")->Fill(xDist, yDist);

              if (closeRing.second < ring->GetRadius() * 1.2)
                fHM->H1("fhTofBetaTrackswithClosestRingInRange")->Fill(getBeta(tTrack));
            }
          }
          //if (ringIndx.size() > 0)fHM->H1("fhTofBetaTrackswithRing")->Fill(getBeta(tTrack)); // ring is somewehere in Acc
          fHM->H1("fhTofBetaAll")->Fill(getBeta(tTrack));
        }
      }
      if (tTrack->GetFitX(RichZPos) > 30) {    // right
        fHM->H1("fhNofTofTracks")->Fill(2.5);  // 1: All 2: left; 3: right; 4: RICH
      }
      else {  //left
        fHM->H1("fhNofTofTracks")->Fill(1.5);
      }
    }


    std::vector<TVector3> RichTofEv;

    for (size_t j = 0; j < ev->GetNofData(ECbmDataType::kTofHit); j++) {
      auto iTofHit      = ev->GetIndex(ECbmDataType::kTofHit, j);
      CbmTofHit* tofHit = static_cast<CbmTofHit*>(fTofHits->At(iTofHit));
      if (tofHit->GetZ() < 2.) continue;  // Cut Bmon away!

      fHM->H1("fhTofHitsZ")->Fill(tofHit->GetZ());
      fHM->H2("fhTofHitsXZ")->Fill(tofHit->GetZ(), tofHit->GetX());
      //fHM->H2("fhTofHitsXY")->Fill(tofHit->GetX(),tofHit->GetY());
      fHM->H3("fhTofXYZ")->Fill(tofHit->GetX(), tofHit->GetZ(), tofHit->GetY());

      if (ringIndx.size() > 0) {
        //fHM->H2("fhTofRichRingXZ")->Fill(tofHit->GetZ(),tofHit->GetX());
        //RichTofEv.emplace_back(tofHit->GetX(),tofHit->GetY(),tofHit->GetZ());
        for (unsigned int rings = 0; rings < ringIndx.size(); rings++) {
          CbmRichRing* ring = static_cast<CbmRichRing*>(fRichRings->At(ringIndx[rings]));
          fHM->H2("fhTofRichRingX")->Fill(ring->GetCenterX(), tofHit->GetX());
          fHM->H2("fhTofRichRingY")->Fill(ring->GetCenterY(), tofHit->GetY());
          for (int k = 0; k < ring->GetNofHits(); k++) {
            Int_t hitInd    = ring->GetHit(k);
            CbmRichHit* hit = (CbmRichHit*) fRichHits->At(hitInd);
            if (nullptr == hit) continue;
            fHM->H2("fhTofHitRichRingHitX")->Fill(hit->GetX(), tofHit->GetX());
            fHM->H2("fhTofHitRichRingHitY")->Fill(hit->GetY(), tofHit->GetY());
          }
          //fhTofRichRingHitX here loop over hits in ring.
        }
      }
    }

    if (ringIndx.size() > 0) {  // Ring in CbmEvent
      std::vector<CbmTofTracklet*> TracksOfEvnt;
      //TRACKS
      auto nofTofTracks = ev->GetNofData(ECbmDataType::kTofTrack);
      fHM->H1("fhTofTracksPerRichEvent")->Fill(nofTofTracks);
      for (size_t j = 0; j < nofTofTracks; j++) {
        auto iTofTrack        = ev->GetIndex(ECbmDataType::kTofTrack, j);
        CbmTofTracklet* track = static_cast<CbmTofTracklet*>(fTofTracks->At(iTofTrack));
        if (track == nullptr) continue;
        if (track->GetNofHits() <= 3) continue;
        TracksOfEvnt.emplace_back(track);
        if (!isOnTarget(track)) continue;
        fHM->H2("fhTofTrackRichRingXY")->Fill(track->GetFitX(RichZPos), track->GetFitY(RichZPos));
        for (size_t k = 0; k < ev->GetNofData(ECbmDataType::kRichHit); k++) {
          auto iRichHit       = ev->GetIndex(ECbmDataType::kRichHit, k);
          CbmRichHit* richHit = static_cast<CbmRichHit*>(fRichHits->At(iRichHit));
          if (richHit == nullptr) continue;
          fHM->H2("fhTofTrackRichHitX")->Fill(richHit->GetX(), track->GetFitX(RichZPos));
          fHM->H2("fhTofTrackRichHitY")->Fill(richHit->GetY(), track->GetFitY(RichZPos));

          //cuts:
          // >3 hits /track
          //                  if (track->GetNofHits() > 3) {
          fHM->H2("fhTofTrackRichHitX_cuts")->Fill(richHit->GetX(), track->GetFitX(RichZPos));
          fHM->H2("fhTofTrackRichHitY_cuts")->Fill(richHit->GetY(), track->GetFitY(RichZPos));
          if (getBeta(track) > 0.90 && getBeta(track) < 1.10) {
            fHM->H2("fhTofTrackRichHitX_oBetacuts")->Fill(richHit->GetX(), track->GetFitX(RichZPos));
            fHM->H2("fhTofTrackRichHitY_oBetacuts")->Fill(richHit->GetY(), track->GetFitY(RichZPos));
            double deltatime = richHit->GetTime() - track->GetTime();
            fHM->H1("fhTofTrackRichHitTime_oBetacuts")->Fill(deltatime);
            if (deltatime > -10 && deltatime < +40) {
              if (track->GetNofHits() == 4) {
                fHM->H2("fhTofTrackRichHitX_oBetacuts_dtime")->Fill(richHit->GetX(), track->GetFitX(RichZPos));
                fHM->H2("fhTofTrackRichHitY_oBetacuts_dtime")->Fill(richHit->GetY(), track->GetFitY(RichZPos));
              }

              if (track->GetNofHits() > 4) {
                fHM->H2("fhTofTrackRichHitX_oBetacuts_dtime_4")->Fill(richHit->GetX(), track->GetFitX(RichZPos));
                fHM->H2("fhTofTrackRichHitY_oBetacuts_dtime_4")->Fill(richHit->GetY(), track->GetFitY(RichZPos));
              }
              else if (track->GetNofHits() > 6) {
                fHM->H2("fhTofTrackRichHitX_oBetacuts_dtime_6")->Fill(richHit->GetX(), track->GetFitX(RichZPos));
                fHM->H2("fhTofTrackRichHitY_oBetacuts_dtime_6")->Fill(richHit->GetY(), track->GetFitY(RichZPos));
              }
              else if (track->GetNofHits() > 8) {
                fHM->H2("fhTofTrackRichHitX_oBetacuts_dtime_8")->Fill(richHit->GetX(), track->GetFitX(RichZPos));
                fHM->H2("fhTofTrackRichHitY_oBetacuts_dtime_8")->Fill(richHit->GetY(), track->GetFitY(RichZPos));
              }
              else if (track->GetNofHits() > 10) {
                fHM->H2("fhTofTrackRichHitX_oBetacuts_dtime_10")->Fill(richHit->GetX(), track->GetFitX(RichZPos));
                fHM->H2("fhTofTrackRichHitY_oBetacuts_dtime_10")->Fill(richHit->GetY(), track->GetFitY(RichZPos));
              }
              // if (track->GetNofHits() == 4) {
              //   for (int l = 0; l < track->GetNofHits(); ++l) {
              //     size_t hitIndex           = track->GetHitIndex(l);
              //     size_t iTofHit            = ev->GetIndex(ECbmDataType::kTofHit, hitIndex);
              //     //if (fEventNum == 88) continue; // TODO: workaround for run 2060
              //     if (fEventNum == 4) continue;
              //     if (iTofHit > -1) continue;
              //     const CbmTofHit* tofHit = static_cast<CbmTofHit*>(fTofHits->At(iTofHit));
              //     if (tofHit->GetZ() < 2.) continue;  // Cut Bmon away!
              //     fHM->H2("fhTofTrackHitRichHitX_oBetacuts_dtime")->Fill(richHit->GetX(), tofHit->GetX());
              //     fHM->H2("fhTofTrackHitRichHitY_oBetacuts_dtime")->Fill(richHit->GetY(), tofHit->GetY());
              //   }
              // }
            }
          }
          else {
            fHM->H2("fhTofTrackRichHitX_uBetacuts")->Fill(richHit->GetX(), track->GetFitX(RichZPos));
            fHM->H2("fhTofTrackRichHitY_uBetacuts")->Fill(richHit->GetY(), track->GetFitY(RichZPos));
          }
          //}
        }
      }

      //rings
      //std::cout<< "Found Rings in CbmEvent: "<< ringIndx.size() <<std::endl;
      fRingsWithTrack[1] += ringIndx.size();
      fRingsWithTrack[2] += nofTofTracks;
      fRingsWithTrack[5] += ringIndx.size() * nofTofTracks;

      for (unsigned int rings = 0; rings < ringIndx.size(); rings++) {
        const CbmRichRing* ring = static_cast<CbmRichRing*>(fRichRings->At(ringIndx[rings]));
        if (nullptr == ring) continue;

        fHM->H1("fhRichRingChi2")->Fill(ring->GetChi2());

        if (!cutRadius(ring)) continue;
        fRingsWithTrack[3]++;  // Rings After Cut
        //DEFAULT: //DrawRing(ring,TracksOfEvnt,true); // here normally; now only Rings with high BETA

        fHM->H1("fhRichRingBeta")->Fill(getBeta(ring));

        auto clTrack = FindClosestTrack(ring, TracksOfEvnt);  // has no cut on distance
        analyseRing(ring, ev, clTrack);

        for (size_t j = 0; j < nofTofTracks; j++) {
          auto iTofTrack        = ev->GetIndex(ECbmDataType::kTofTrack, j);
          CbmTofTracklet* track = static_cast<CbmTofTracklet*>(fTofTracks->At(iTofTrack));
          if (track == nullptr) continue;
          if (!(track->GetNofHits() == 4 && (getBeta(track) > 0.90 && getBeta(track) < 1.10))) continue;
          if (rings == 0) {
            fRingsWithTrack[4]++;
            if (ring->GetChi2() < 4.)
              fSeDsply_TR->DrawEvent(ev, ringIndx, 1);  //Some will be drawn double, but for now ok
          }
          fHM->H2("fhTofTrackRichRingX")->Fill(ring->GetCenterX(), track->GetFitX(RichZPos));
          fHM->H2("fhTofTrackRichRingY")->Fill(ring->GetCenterY(), track->GetFitY(RichZPos));

          const double xDist = (track->GetFitX(RichZPos) - ring->GetCenterX());
          const double yDist = (track->GetFitY(RichZPos) - ring->GetCenterY());
          const double rDist = std::sqrt(xDist * xDist + yDist * yDist);
          fHM->H1("fhTrackRingDistance_corr")->Fill(rDist);

          fRingsWithTrack[0]++;

          for (int k = 0; k < ring->GetNofHits(); k++) {
            Int_t hitInd    = ring->GetHit(k);
            CbmRichHit* hit = (CbmRichHit*) fRichHits->At(hitInd);
            if (nullptr == hit) continue;
            fHM->H2("fhTofTrackRichRingHitX")->Fill(hit->GetX(), track->GetFitX(RichZPos));
            fHM->H2("fhTofTrackRichRingHitY")->Fill(hit->GetY(), track->GetFitY(RichZPos));
          }
        }

        if (clTrack.first > -1) {
          // first: TrackIndex;  second: Distance;
          //FIXME
          //if (getBeta(TracksOfEvnt[clTrack.first]) > 0.9) DrawRing(ring,TracksOfEvnt,true);
          fHM->H1("fhTrackRingDistance")->Fill(clTrack.second);
          CbmTofTracklet* track =
            TracksOfEvnt[clTrack.first];  //static_cast<CbmTofTracklet *>(fTofTracks->At(clTrack.first));
          if (track == nullptr) continue;
          if (isOnTarget(track)) {
            fHM->H1("fhTrackRingDistanceOnTarget")->Fill(clTrack.second);
          }
          else {
            fHM->H1("fhTrackRingDistanceOffTarget")->Fill(clTrack.second);
          }
          fHM->H2("fhTrackRingDistanceVSRingradius")->Fill(clTrack.second, ring->GetRadius());
          fHM->H2("fhTrackRingDistanceVSRingChi2")->Fill(clTrack.second, ring->GetChi2());
          //if ( (clTrack.second < 20.0 )) {
          fHM->H2("fhRichRingXY_goodTrack")->Fill(ring->GetCenterX(), ring->GetCenterY());
          fHM->H2("fhRichRing_goodTrackXY")->Fill(track->GetFitX(RichZPos), track->GetFitY(RichZPos));
          //}
          fHM->H2("fhClosTrackRingX")->Fill(ring->GetCenterX(), track->GetFitX(RichZPos));
          fHM->H2("fhClosTrackRingY")->Fill(ring->GetCenterY(), track->GetFitY(RichZPos));
          fHM->H2("fhTofClosTrackRichRingXY")->Fill(track->GetFitX(RichZPos), track->GetFitY(RichZPos));
          if ((clTrack.second < (ring->GetRadius() * 1.2))) {  //Good Ring
            fHM->H2("fhGoodRingsXY_TargetPos")->Fill(track->GetFitX(0.), track->GetFitY(0.));
            fHM->H1("fhRichRingChi2_goodRing")->Fill(ring->GetChi2());
            fHM->H2("fhTrackRingDistanceVSRingChi2_goodRing")->Fill(clTrack.second, ring->GetChi2());
            fHM->H1("fhTofBetaRing")->Fill(getBeta(track));
            fHM->H2("fhTofBetaRingDist")->Fill(getBeta(track), clTrack.second);
            fHM->H2("fhTofBetaVsRadius")->Fill(getBeta(track), ring->GetRadius());
            //Ring properties of "Good rings"
            fHM->H1("fhRichRingRadius_goodRing")->Fill(ring->GetRadius());
            fHM->H1("fhNofHitsInRing_goodRing")->Fill(ring->GetNofHits());
            fHM->H1("fhRichRingBeta_GoodRing")->Fill(getBeta(ring));
          }
        }

        fHM->H2("fhTofRichRingXZ")
          ->Fill(RichZPos,
                 ring->GetCenterX());  //Z Axis by Hand because ring has no Z component
        RichTofEv.emplace_back(ring->GetCenterX(), ring->GetCenterY(), RichZPos);
        // Draw XY position of center of rings; later add the Tracks in Tof
        fHM->H2("fhTofTrackRichRingXY")
          ->Fill(ring->GetCenterX(), ring->GetCenterY(),
                 3);  // 3 to change color for Rings
      }

      //DrawRichTofEv(RichTofEv);
      fHM->H1("fhNofCbmEventsRing")->Fill(1);
    }


    //         std::cout<< "TOF Digis:\t" << ev->GetNofData(ECbmDataType::kTofDigi) <<std::endl;
    //         std::cout<< "TOF Hits:\t" << ev->GetNofData(ECbmDataType::kTofHit) <<std::endl;
    //         std::cout<< "TOF Tracks:\t" << ev->GetNofData(ECbmDataType::kTofTrack) <<std::endl;


    //Select and Analyse Blobs
    uint32_t blob = 0;
    for (unsigned int j = 0; j < pmtHits.size(); ++j) {
      if (pmtHits[j] > 30) {
        blob++;
        // Blob found
        double xBlob = -7.95 + (int((j / 9)) * 5.3) + fXOffsetHisto;
        double yBlob = 21.2 - ((j % 9) * 5.3);
        //std::cout<<"BLOB X:" << i << "   "<< xBlob << std::endl;
        for (size_t k = 0; k < ev->GetNofData(ECbmDataType::kTofTrack); k++) {
          auto iTofTrack        = ev->GetIndex(ECbmDataType::kTofTrack, k);
          CbmTofTracklet* track = static_cast<CbmTofTracklet*>(fTofTracks->At(iTofTrack));
          if (track == nullptr) continue;
          double xBlobTrack = track->GetFitX(RichZPos) - xBlob;
          double yBlobTrack = track->GetFitY(RichZPos) - yBlob;
          fHM->H1("fhBlobTrackDistX")->Fill(std::fabs(xBlobTrack));
          fHM->H1("fhBlobTrackDistY")->Fill(std::fabs(yBlobTrack));
          fHM->H1("fhBlobTrackDist")->Fill(std::sqrt(xBlobTrack * xBlobTrack + yBlobTrack * yBlobTrack));
        }
      }
    }
    if (blob > 0) {
      fHM->H1("fhNofBlobEvents")->Fill(1);
      fHM->H1("fhNofBlobsInEvent")->Fill(blob);
    }

  }  //End CbmEvent loop


  RichRings();
}

void CbmRichMCbmQaReal::RichRings()
{
  int nofRichRings = fRichRings->GetEntriesFast();
  fHM->H1("fhNofRichRingsInTimeslice")->Fill(nofRichRings);
  for (int i = 0; i < nofRichRings; i++) {
    CbmRichRing* ring = static_cast<CbmRichRing*>(fRichRings->At(i));
    if (ring == nullptr) continue;
    //DrawRing(ring);
    fHM->H2("fhRichRingXY")->Fill(ring->GetCenterX(), ring->GetCenterY());
    fHM->H1("fhRichRingRadius")->Fill(ring->GetRadius());
    fHM->H1("fhNofHitsInRing")->Fill(ring->GetNofHits());
    fHM->H2("fhRichRingRadiusY")->Fill(ring->GetRadius(), ring->GetCenterY());
    fHM->H2("fhRichHitsRingRadius")->Fill(ring->GetNofHits(), ring->GetRadius());
  }
}

std::pair<int, double> CbmRichMCbmQaReal::FindClosestTrack(const CbmRichRing* ring,
                                                           const std::vector<CbmTofTracklet*> track)
{
  int ringX = ring->GetCenterX();
  int ringY = ring->GetCenterY();

  int closTrack   = -1;
  double closDist = -999999.99;

  for (unsigned int indx = 0; indx < track.size(); ++indx) {

    //Calc if Track is in Ring (+20% )
    if (track[indx]->GetNofHits() <= 3) continue;
    const double xDist        = (track[indx]->GetFitX(RichZPos) - ringX);
    const double yDist        = (track[indx]->GetFitY(RichZPos) - ringY);
    const double rDist        = std::sqrt(xDist * xDist + yDist * yDist);
    const double RadiusFactor = 1.2;  // Factor of how big radius of acceptance should

    if (rDist < ring->GetRadius() * RadiusFactor) {
      //std::cout<<"Track in defined Ring range ("<<ring->GetRadius()*RadiusFactor<<"cm) (RingRadius: "<<ring->GetRadius()<<"cm).  ";
    }
    //if (rDist < ring->GetRadius()*RadiusFactor && ring->GetRadius() > 2. && ring->GetRadius() < 4.2)  {
    if (indx == 0) {
      closDist  = rDist;
      closTrack = indx;
    }
    else {
      if (closDist > rDist) {
        closDist  = rDist;
        closTrack = indx;
      }
    }
    //}
  }
  //if (closTrack > -1 ) std::cout<< "closestTrack to ring "<< ring <<": "<<closTrack<<"  " << static_cast<CbmTofTracklet *>(fTofTracks->At(closTrack)) <<std::endl;
  std::pair<int, double> p;
  p.first  = closTrack;
  p.second = closDist;

  return p;
}

std::pair<int, double> CbmRichMCbmQaReal::FindClosestRing(CbmTofTracklet* track, std::vector<int>& ringIndx)
{
  // Closest Ring to Track in +20% Ring Radius!
  const double x_track = track->GetFitX(RichZPos);
  const double y_track = track->GetFitY(RichZPos);

  int closTrack   = -1;
  double closDist = -999999.99;

  for (unsigned int indx = 0; indx < ringIndx.size(); ++indx) {
    const CbmRichRing* ring = static_cast<CbmRichRing*>(fRichRings->At(ringIndx[indx]));

    int ringX = ring->GetCenterX();
    int ringY = ring->GetCenterY();

    //Calc if Track is in Ring (+20% )
    const double xDist = (x_track - ringX);
    const double yDist = (y_track - ringY);
    const double rDist = std::sqrt(xDist * xDist + yDist * yDist);
    //const double RadiusFactor = 1.2;  // Factor of how big radius of acceptance should

    if (/*rDist < ring->GetRadius() * RadiusFactor &&*/ cutRadius(ring)) {
      //std::cout<<"Track in defined Ring range ("<<ring->GetRadius()*RadiusFactor<<"cm) (RingRadius: "<<ring->GetRadius()<<"cm).  ";

      if (indx == 0) {
        closDist  = rDist;
        closTrack = indx;
      }
      else {
        if (closDist > rDist) {
          closDist  = rDist;
          closTrack = indx;
        }
      }
    }
  }

  //if (closTrack > -1 ) std::cout<< "closestRing to Track "<< track <<": "<<closTrack <<" "<< static_cast<CbmRichRing *>(fRichRings->At(ringIndx[closTrack])) <<std::endl;
  std::pair<int, double> p;
  p.first  = closTrack;
  p.second = closDist;

  return p;
}

void CbmRichMCbmQaReal::DrawHist()
{
  cout.precision(4);

  //SetDefaultDrawStyle();
  double nofEvents = fHM->H1("fhNofCbmEvents")->GetEntries();
  fHM->ScaleByPattern("fh_.*", 1. / nofEvents);

  {
    fHM->CreateCanvas("rich_mcbm_fhNofCbmEvents", "rich_mcbm_fhNofCbmEvents", 600, 600);
    DrawH1(fHM->H1("fhNofCbmEvents"));
  }

  {
    fHM->CreateCanvas("rich_mcbm_fhNofCbmEventsRing", "rich_mcbm_fhNofCbmEventsRing", 600, 600);
    DrawH1(fHM->H1("fhNofCbmEventsRing"));
  }

  {
    fHM->CreateCanvas("rich_mcbm_fhNofEvents", "rich_mcbm_fhNofEvents", 600, 600);
    DrawH1(fHM->H1("fhNofEvents"));
  }

  {
    fHM->CreateCanvas("HitsInTimeslice", "HitsInTimeslice", 600, 600);
    DrawH1(fHM->H1("fhHitsInTimeslice"));
  }

  {
    fHM->CreateCanvas("RichDigisConsecTime", "RichDigisConsecTime", 600, 600);
    DrawH1(fHM->H1("fhRichDigisConsecTime"), kLinear, kLog);
  }


  {
    fHM->CreateCanvas("RichDigisConsecTimeTOT", "RichDigisConsecTimeTOT", 600, 600);
    DrawH1(fHM->H1("fhRichDigisConsecTimeTOT"), kLinear, kLog);
  }


  /*{ // NO position info for Digis :(
        fHM->CreateCanvas("Pixelrate","Pixelrate", 600 , 600);
        double time = nofEvents * 0.0128; //seconds
        fHM->H1("fhRichDigiPixelRate")->Scale(1./time);
        DrawH1(fHM->H1("fhRichDigiPixelRate"));
    }*/

  {
    TCanvas* c = fHM->CreateCanvas("RichRingXY_goodTrack", "RichRingXY_goodTrack", 1200, 600);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhRichRingXY_goodTrack"));
    c->cd(2);
    DrawH2(fHM->H2("fhRichRing_goodTrackXY"));
  }


  {
    TCanvas* c = fHM->CreateCanvas("rich_mcbm_nofObjectsInTimeslice", "rich_mcbm_nofObjectsInTimeslice", 1500, 500);
    c->Divide(3, 1);
    c->cd(1);
    DrawH1(fHM->H1("fhNofRichDigisInTimeslice"), kLinear, kLog);
    c->cd(2);
    DrawH1(fHM->H1("fhNofRichHitsInTimeslice"), kLinear, kLog);
    c->cd(3);
    DrawH1(fHM->H1("fhNofRichRingsInTimeslice"), kLinear, kLog);
  }

  {
    TCanvas* c = fHM->CreateCanvas("rich_mcbm_XY", "rich_mcbm_XY", 1200, 600);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhRichHitXY"));
    c->cd(2);
    DrawH2(fHM->H2("fhRichRingXY"));
  }


  {
    TCanvas* c = fHM->CreateCanvas("rich_tof_XY", "rich_tof_XY", 1200, 600);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofRichX"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofRichY"));
  }


  {
    TCanvas* c = fHM->CreateCanvas("rich_tof_XY_zoomed", "rich_tof_XY_zoomed", 1200, 600);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofRichX_zoomed"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofRichY_zoomed"));
  }


  {
    TCanvas* c = fHM->CreateCanvas("rich_mcbm_richDigisTimeLog", "rich_mcbm_richDigisTimeLog", 1200, 1200);
    c->Divide(1, 2);
    c->cd(1);
    DrawH1({fHM->H1("fhRichDigisTimeLog"), fHM->H1("fhTofDigisTimeLog"), fHM->H1("fhBmonDigisTimeLog"),
            fHM->H1("fhStsDigisTimeLog"), fHM->H1("fhTrd1dDigisTimeLog"), fHM->H1("fhTrd2dDigisTimeLog")},
           {"RICH", "TOF", "Bmon", "STS", "TRD", "TRD2D"}, kLinear, kLog, true, 0.87, 0.75, 0.99, 0.99);
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.10);
    fHM->H1("fhStsDigisTimeLog")->GetYaxis()->SetTitleOffset(0.7);
    fHM->H1("fhStsDigisTimeLog")->SetMinimum(0.9);
    c->cd(2);
    DrawH1({fHM->H1("fhRichDigisTimeLogZoom"), fHM->H1("fhTofDigisTimeLogZoom"), fHM->H1("fhBmonDigisTimeLogZoom"),
            fHM->H1("fhStsDigisTimeLogZoom"), fHM->H1("fhTrd1dDigisTimeLogZoom"), fHM->H1("fhTrd2dDigisTimeLogZoom")},
           {"RICH", "TOF", "Bmon", "STS", "TRD", "TRD2D"}, kLinear, kLog, true, 0.87, 0.75, 0.99, 0.99);
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.1);
    fHM->H1("fhStsDigisTimeLogZoom")->GetYaxis()->SetTitleOffset(0.7);
    fHM->H1("fhStsDigisTimeLogZoom")->SetMinimum(0.9);
  }

  {
    fHM->CreateCanvas("rich_mcbm_richDigisTimeLog2", "rich_mcbm_richDigisTimeLog2", 1200, 600);
    DrawH1({fHM->H1("fhRichDigisTimeLogZoom2"), fHM->H1("fhTofDigisTimeLogZoom2"), fHM->H1("fhBmonDigisTimeLogZoom2"),
            fHM->H1("fhStsDigisTimeLogZoom2"), fHM->H1("fhTrd1dDigisTimeLogZoom2"),
            fHM->H1("fhTrd2dDigisTimeLogZoom2")},
           {"RICH", "TOF", "Bmon", "STS", "TRD", "TRD2D"}, kLinear, kLog, true, 0.87, 0.75, 0.99, 0.99);
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.1);
    fHM->H1("fhStsDigisTimeLogZoom2")->GetYaxis()->SetTitleOffset(0.7);
    fHM->H1("fhStsDigisTimeLogZoom2")->SetMinimum(0.9);
  }

  {
    fHM->CreateCanvas("rich_mcbm_richDigisRingTimeLog", "rich_mcbm_richDigisRingTimeLog", 1200, 600);
    TH1D* copyRichDigi = (TH1D*) fHM->H1("fhRichDigisTimeLog")->Clone();
    TH1D* copyRichRing = (TH1D*) fHM->H1("fhRichRingsTimeLog")->Clone();
    DrawH1({copyRichDigi, fHM->H1("fhTofDigisTimeLog"), fHM->H1("fhBmonDigisTimeLog"), copyRichRing,
            fHM->H1("fhStsDigisTimeLog"), fHM->H1("fhTrd1dDigisTimeLog"), fHM->H1("fhTrd2dDigisTimeLog")},
           {"RICH", "TOF", "Bmon", "RICH RING", "STS", "TRD", "TRD2D"}, kLinear, kLog, true, 0.83, 0.75, 0.99, 0.99);
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.1);
    copyRichDigi->GetYaxis()->SetTitleOffset(0.7);
    copyRichDigi->SetMinimum(0.9);
  }

  {

    TCanvas* c = fHM->CreateCanvas("rich_mcbm_richRingsTimeLog", "rich_mcbm_richRingsTimeLog", 1200, 1200);
    c->Divide(1, 2);
    c->cd(1);
    DrawH1({fHM->H1("fhRichDigisTimeLog"), fHM->H1("fhRichRingsTimeLog")}, {"Digis", "Rings"}, kLinear, kLog, true,
           0.87, 0.75, 0.99, 0.99);
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.10);
    fHM->H1("fhRichDigisTimeLog")->GetYaxis()->SetTitleOffset(0.7);
    fHM->H1("fhRichDigisTimeLog")->SetMinimum(0.9);
    c->cd(2);
    DrawH1({fHM->H1("fhRichDigisTimeLogZoom"), fHM->H1("fhRichRingsTimeLogZoom")}, {"Digis", "Rings"}, kLinear, kLog,
           true, 0.87, 0.75, 0.99, 0.99);
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.1);
    fHM->H1("fhRichDigisTimeLogZoom")->GetYaxis()->SetTitleOffset(0.7);
    fHM->H1("fhRichDigisTimeLogZoom")->SetMinimum(0.9);
  }

  {
    fHM->CreateCanvas("rich_mcbm_richRingsTimeLog2", "rich_mcbm_richRingsTimeLog2", 1200, 600);
    DrawH1({fHM->H1("fhRichDigisTimeLogZoom2"), fHM->H1("fhRichRingsTimeLogZoom2")}, {"Digis", "Rings"}, kLinear, kLog,
           true, 0.87, 0.75, 0.99, 0.99);
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.1);
    fHM->H1("fhRichDigisTimeLogZoom2")->GetYaxis()->SetTitleOffset(0.7);
    fHM->H1("fhRichDigisTimeLogZoom2")->SetMinimum(0.9);
  }


  {
    TCanvas* c = fHM->CreateCanvas("rich_ToT", "rich_ToT", 1200, 600);
    c->Divide(2, 1);
    c->cd(1);
    DrawH1(fHM->H1("fhRichDigisToT"));
    c->cd(2);
    DrawH1(fHM->H1("fhRichHitToT"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("rich_BlobTrackDist", "rich_BlobTrackDist", 1800, 600);
    c->Divide(3, 1);
    c->cd(1);
    DrawH1(fHM->H1("fhBlobTrackDistX"));
    c->cd(2);
    DrawH1(fHM->H1("fhBlobTrackDistY"));
    c->cd(3);
    DrawH1(fHM->H1("fhBlobTrackDist"));
  }

  {
    fHM->CreateCanvas("ToF_XYZ", "ToF_XYZ", 1200, 1200);
    fHM->H3("fhTofXYZ")->Draw();
  }


  {
    TCanvas* c = fHM->CreateCanvas("rich_mcbm_rings", "rich_mcbm_rings", 1200, 600);
    c->Divide(2, 1);
    c->cd(1);
    DrawH1(fHM->H1("fhRichRingRadius"));
    c->cd(2);
    DrawH1(fHM->H1("fhNofHitsInRing"));
  }

  {
    fHM->CreateCanvas("TofHitsZ", "TofHitsZ", 1200, 1200);
    fHM->H1("fhTofHitsZ")->Draw();
  }

  {
    fHM->CreateCanvas("TofTracksPerEvent", "TofTracksPerEvent", 1200, 1200);
    DrawH1(fHM->H1("fhTofTracksPerEvent"), kLinear, kLog);
  }

  {
    fHM->CreateCanvas("TofTracksPerRichEvent", "TofTracksPerRichEvent", 1200, 1200);
    DrawH1(fHM->H1("fhTofTracksPerRichEvent"), kLinear, kLog);
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofRichRingX_Y", "TofRichRingX_Y", 1200, 1200);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofRichRingX"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofRichRingY"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofRichX_Stacks", "TofRichX_Stacks", 1800, 600);
    c->Divide(3, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofRichX_stack1"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofRichX_stack2"));
    c->cd(3);
    DrawH2(fHM->H2("fhTofRichX_stack3"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofRichY_Stacks", "TofRichY_Stacks", 1800, 600);
    c->Divide(3, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofRichY_stack1"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofRichY_stack2"));
    c->cd(3);
    DrawH2(fHM->H2("fhTofRichY_stack3"));
  }

  {
    fHM->CreateCanvas("TofRichRingXZ", "TofRichRingXZ", 1200, 1200);

    DrawH2(fHM->H2("fhTofRichRingXZ"));
  }

  {
    fHM->CreateCanvas("TofHitsXZ", "TofHitsXZ", 1200, 1200);

    DrawH2(fHM->H2("fhTofHitsXZ"));
  }

  {
    fHM->CreateCanvas("TofHitsXY", "TofHitsXY", 1200, 1200);

    DrawH2(fHM->H2("fhTofHitsXY"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofHitsXY_moduleIds", "TofHitsXY_moduleIds", 2500, 500);

    c->Divide(5, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofHitsXY_0"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofHitsXY_1"));
    c->cd(3);
    DrawH2(fHM->H2("fhTofHitsXY_2"));
    c->cd(4);
    DrawH2(fHM->H2("fhTofHitsXY_3"));
    c->cd(5);
    DrawH2(fHM->H2("fhTofHitsXY_4"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofHitsZX_moduleIds", "TofHitsZX_moduleIds", 2500, 500);

    c->Divide(5, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofHitsZX_0"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofHitsZX_1"));
    c->cd(3);
    DrawH2(fHM->H2("fhTofHitsZX_2"));
    c->cd(4);
    DrawH2(fHM->H2("fhTofHitsZX_3"));
    c->cd(5);
    DrawH2(fHM->H2("fhTofHitsZX_4"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofRichHitsX_moduleIds", "TofRichHitsX_moduleIds", 2500, 500);

    c->Divide(5, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofRichX_0"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofRichX_1"));
    c->cd(3);
    DrawH2(fHM->H2("fhTofRichX_2"));
    c->cd(4);
    DrawH2(fHM->H2("fhTofRichX_3"));
    c->cd(5);
    DrawH2(fHM->H2("fhTofRichX_4"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofRichHitsY_moduleIds", "TofRichHitsY_moduleIds", 2500, 500);

    c->Divide(5, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofRichY_0"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofRichY_1"));
    c->cd(3);
    DrawH2(fHM->H2("fhTofRichY_2"));
    c->cd(4);
    DrawH2(fHM->H2("fhTofRichY_3"));
    c->cd(5);
    DrawH2(fHM->H2("fhTofRichY_4"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("fhTofRichHitsResidual_moduleIds", "fhTofRichHitsResidual_moduleIds", 2500, 500);

    c->Divide(5, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofRichHitsResidual_0"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofRichHitsResidual_1"));
    c->cd(3);
    DrawH2(fHM->H2("fhTofRichHitsResidual_2"));
    c->cd(4);
    DrawH2(fHM->H2("fhTofRichHitsResidual_3"));
    c->cd(5);
    DrawH2(fHM->H2("fhTofRichHitsResidual_4"));
  }

  {
    fHM->CreateCanvas("TofTrackRichRingXY", "TofTrackRichRingXY", 1200, 1200);

    DrawH2(fHM->H2("fhTofTrackRichRingXY"));
  }

  {
    fHM->CreateCanvas("TofClosTrackRichRingXY", "TofClosTrackRichRingXY", 1200, 1200);

    DrawH2(fHM->H2("fhTofClosTrackRichRingXY"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TrackRingDist", "TrackRingDist", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH1(fHM->H1("fhTrackRingDistance"));
    c->cd(2);
    DrawH2(fHM->H2("fhTrackRingDistanceVSRingradius"));
  }

  {
    fHM->CreateCanvas("RingTrackDist", "RingTrackDist", 1200, 800);
    DrawH1(fHM->H1("fhRingTrackDistance"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("RingTrackDist_all", "RingTrackDist_all", 1800, 800);
    c->Divide(3, 1);
    c->cd(1);
    DrawH1(fHM->H1("fhRingTrackDistance_X"));
    c->cd(2);
    DrawH1(fHM->H1("fhRingTrackDistance_Y"));
    c->cd(3);
    DrawH2(fHM->H2("fhRingTrackDistance_XY"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("ClosTrackRingXY", "ClosTrackRingXY", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhClosTrackRingX"));
    c->cd(2);
    DrawH2(fHM->H2("fhClosTrackRingY"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TrackRingXY", "TrackRingXY", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofTrackRichRingX"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofTrackRichRingY"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofTracksXY", "TofTracksXY", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofTracksXY"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofTracksXYRICH"));
  }

  {
    fHM->CreateCanvas("TofTracksXY_Target", "TofTracksXY_Target", 800, 800);
    DrawH2(fHM->H2("fhTofTracksXY_Target"));
  }

  {
    fHM->CreateCanvas("GoodRingsXY_TargetPos", "GoodRingsXY_TargetPos", 800, 800);
    DrawH2(fHM->H2("fhGoodRingsXY_TargetPos"));
  }

  {
    fHM->CreateCanvas("TofBeta", "TofBeta", 800, 800);
    gStyle->SetOptStat(0);
    fHM->H1("fhTofBetaAll")->Draw("HIST");
    fHM->H1("fhTofBetaTracksWithHitsNoRing")->SetLineColorAlpha(kGreen, 1);
    fHM->H1("fhTofBetaTracksWithHitsNoRing")->Draw("HIST SAME");

    //fHM->H1("fhTofBetaTracksNoRing")->SetLineColorAlpha(8, 1);
    //fHM->H1("fhTofBetaTracksNoRing")->Draw("HIST SAME");

    //fHM->H1("fhTofBetaTracksWithHits")->SetLineColorAlpha(44, 1);
    //fHM->H1("fhTofBetaTracksWithHits")->Draw("HIST SAME");

    fHM->H1("fhTofBetaTrackswithClosestRingInRange")->SetLineColorAlpha(44, 1);
    fHM->H1("fhTofBetaTrackswithClosestRingInRange")->Draw("HIST SAME");

    fHM->H1("fhTofBetaRing")->SetLineColorAlpha(kRed, 1);
    fHM->H1("fhTofBetaRing")->Draw("HIST SAME");

    auto legend = new TLegend(0.1, 0.75, 0.4, 0.9);
    legend->AddEntry(fHM->H1("fhTofBetaTracksWithHitsNoRing"), "Tracks with RichHits and no Ring", "l");
    //legend->AddEntry(fHM->H1("fhTofBetaTracksWithHits"),"Tracks with RichHits","l");
    //legend->AddEntry(fHM->H1("fhTofBetaTracksNoRing"),"Tracks with no Ring","l");
    legend->AddEntry(fHM->H1("fhTofBetaTrackswithClosestRingInRange"), "Tracks with clos. Ring in +20% Radius", "l");
    legend->AddEntry(fHM->H1("fhTofBetaRing"), "Tracks in good ring", "l");
    if (fRestrictToAcc) {
      legend->AddEntry(fHM->H1("fhTofBetaAll"), "All Tracks in mRICH Acc.", "l");
    }
    else if (fRestrictToFullAcc) {
      legend->AddEntry(fHM->H1("fhTofBetaAll"), "All Tracks in full mRICH Acc.", "l");
    }
    else {
      legend->AddEntry(fHM->H1("fhTofBetaAll"), "All Tracks", "l");
    }
    legend->Draw();
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofBetaLog", "TofBetaLog", 1000, 1000);
    c->SetLogy();

    Double_t max = fHM->H1("fhTofBetaAll")->GetMaximum();
    fHM->H1("fhTofBetaAll")->Draw("HIST");

    if (fHM->H1("fhTofBetaTracksWithHitsNoRing")->GetMaximum() > max)
      max = fHM->H1("fhTofBetaTracksWithHitsNoRing")->GetMaximum();
    fHM->H1("fhTofBetaTracksWithHitsNoRing")->SetLineColorAlpha(kGreen, 1);
    fHM->H1("fhTofBetaTracksWithHitsNoRing")->Draw("HIST SAME");

    //fHM->H1("fhTofBetaTracksNoRing")->SetLineColorAlpha(8, 1);
    //fHM->H1("fhTofBetaTracksNoRing")->Draw("HIST SAME");

    //fHM->H1("fhTofBetaTracksWithHits")->SetLineColorAlpha(44, 1);
    //fHM->H1("fhTofBetaTracksWithHits")->Draw("HIST SAME");

    if (fHM->H1("fhTofBetaTrackswithClosestRingInRange")->GetMaximum() > max)
      max = fHM->H1("fhTofBetaTrackswithClosestRingInRange")->GetMaximum();
    fHM->H1("fhTofBetaTrackswithClosestRingInRange")->SetLineColorAlpha(12, 1);
    fHM->H1("fhTofBetaTrackswithClosestRingInRange")->Draw("HIST SAME");

    if (fHM->H1("fhTofBetaRing")->GetMaximum() > max) max = fHM->H1("fhTofBetaRing")->GetMaximum();
    fHM->H1("fhTofBetaRing")->SetLineColorAlpha(kRed, 1);
    fHM->H1("fhTofBetaRing")->Draw("HIST SAME");

    fHM->H1("fhTofBetaAll")->SetAxisRange(1., max * 1.8, "Y");

    auto legend = new TLegend(0.75, 0.77, 0.99, 0.96);
    legend->AddEntry(fHM->H1("fhTofBetaTracksWithHitsNoRing"), "Tracks with RichHits and no Ring", "l");
    //legend->AddEntry(fHM->H1("fhTofBetaTracksWithHits"),"Tracks with RichHits","l");
    //legend->AddEntry(fHM->H1("fhTofBetaTracksNoRing"),"Tracks with no Ring","l");
    legend->AddEntry(fHM->H1("fhTofBetaTrackswithClosestRingInRange"), "Tracks with clos. Ring in +20% Radius", "l");
    legend->AddEntry(fHM->H1("fhTofBetaRing"), "Tracks in good ring", "l");
    if (fRestrictToAcc) {
      legend->AddEntry(fHM->H1("fhTofBetaAll"), "All Tracks in mRICH Acc.", "l");
    }
    else if (fRestrictToFullAcc) {
      legend->AddEntry(fHM->H1("fhTofBetaAll"), "All Tracks in full mRICH Acc.", "l");
    }
    else {
      legend->AddEntry(fHM->H1("fhTofBetaAll"), "All Tracks", "l");
    }
    legend->Draw();
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofBetaLog_pos", "TofBetaLog_pos", 1000, 1000);
    c->SetLogy();

    TH1D* copyTofBetaAll = (TH1D*) fHM->H1("fhTofBetaAll")->Clone();

    Double_t max = copyTofBetaAll->GetMaximum();

    copyTofBetaAll->GetXaxis()->SetRangeUser(0.0, 1.2);
    copyTofBetaAll->Draw("HIST");

    if (fHM->H1("fhTofBetaTracksWithHitsNoRing")->GetMaximum() > max)
      max = fHM->H1("fhTofBetaTracksWithHitsNoRing")->GetMaximum();
    fHM->H1("fhTofBetaTracksWithHitsNoRing")->SetLineColorAlpha(kGreen, 1);
    fHM->H1("fhTofBetaTracksWithHitsNoRing")->Draw("HIST SAME");

    //fHM->H1("fhTofBetaTracksNoRing")->SetLineColorAlpha(8, 1);
    //fHM->H1("fhTofBetaTracksNoRing")->Draw("HIST SAME");

    //fHM->H1("fhTofBetaTracksWithHits")->SetLineColorAlpha(44, 1);
    //fHM->H1("fhTofBetaTracksWithHits")->Draw("HIST SAME");

    if (fHM->H1("fhTofBetaTrackswithClosestRingInRange")->GetMaximum() > max)
      max = fHM->H1("fhTofBetaTrackswithClosestRingInRange")->GetMaximum();
    fHM->H1("fhTofBetaTrackswithClosestRingInRange")->SetLineColorAlpha(12, 1);
    fHM->H1("fhTofBetaTrackswithClosestRingInRange")->Draw("HIST SAME");

    if (fHM->H1("fhTofBetaRing")->GetMaximum() > max) max = fHM->H1("fhTofBetaRing")->GetMaximum();
    fHM->H1("fhTofBetaRing")->SetLineColorAlpha(kRed, 1);
    fHM->H1("fhTofBetaRing")->Draw("HIST SAME");

    copyTofBetaAll->SetAxisRange(1., max * 1.8, "Y");

    auto legend = new TLegend(0.75, 0.77, 0.99, 0.96);
    legend->AddEntry(fHM->H1("fhTofBetaTracksWithHitsNoRing"), "Tracks with RichHits and no Ring", "l");
    //legend->AddEntry(fHM->H1("fhTofBetaTracksWithHits"),"Tracks with RichHits","l");
    //legend->AddEntry(fHM->H1("fhTofBetaTracksNoRing"),"Tracks with no Ring","l");
    legend->AddEntry(fHM->H1("fhTofBetaTrackswithClosestRingInRange"), "Tracks with clos. Ring in +20% Radius", "l");
    legend->AddEntry(fHM->H1("fhTofBetaRing"), "Tracks in good ring", "l");
    if (fRestrictToAcc) {
      legend->AddEntry(fHM->H1("fhTofBetaAll"), "All Tracks in mRICH Acc.", "l");
    }
    else if (fRestrictToFullAcc) {
      legend->AddEntry(copyTofBetaAll, "All Tracks in full mRICH Acc.", "l");
    }
    else {
      legend->AddEntry(copyTofBetaAll, "All Tracks", "l");
    }
    legend->Draw();
  }

  {
    fHM->CreateCanvas("TofBetaVsRadius", "TofBetaVsRadius", 800, 800);
    DrawH2(fHM->H2("fhTofBetaVsRadius"));
  }

  {
    fHM->CreateCanvas("TofBetaRingDist", "TofBetaRingDist", 800, 800);
    DrawH2(fHM->H2("fhTofBetaRingDist"));
  }

  {
    fHM->CreateCanvas("TofBetaAllFullAcc", "TofBetaAllFullAcc", 800, 800);
    DrawH1(fHM->H1("fhTofBetaAllFullAcc"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofBetaAllFullAccLog", "TofBetaAllFullAccLog", 800, 800);
    c->SetLogy();
    DrawH1(fHM->H1("fhTofBetaAllFullAcc"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("RingAnalysis", "RingAnalysis", 1000, 1000);
    //c->SetLogy();
    c->Divide(2, 2);
    c->cd(1);
    DrawH1(fHM->H1("fhRingDeltaTime"));
    c->cd(2);
    DrawH1(fHM->H1("fhRingToTs"));
    c->cd(3);
    DrawH1(
      {fHM->H1("fhRingLE"), fHM->H1("fhRingNoClTrackLE"), fHM->H1("fhRingClTrackFarAwayLE"), fHM->H1("fhGoodRingLE")},
      {"all Rings", "Rings w/o closest Track", "Rings w/ closest Track >5cm", "Good Rings"}, kLinear, kLinear, true,
      0.70, 0.75, 0.95, 0.99);
    c->cd(4);
    DrawH2(fHM->H2("fhRingLEvsToT"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("InnerRingAnalysis", "InnerRingAnalysis", 1000, 1200);
    //c->SetLogy();
    c->Divide(2, 3);
    c->cd(1);
    DrawH1(fHM->H1("fhInnerRingDeltaTime"));
    c->cd(2);
    DrawH1(fHM->H1("fhInnerRingToTs"));
    c->cd(3);
    DrawH1({fHM->H1("fhInnerRingLE"), fHM->H1("fhInnerRingNoClTrackLE"), fHM->H1("fhInnerRingClTrackFarAwayLE"),
            fHM->H1("fhInnerGoodRingLE")},
           {"all Rings", "Rings w/o closest Track", "Rings w/ closest Track >5cm", "Good Rings"}, kLinear, kLinear,
           true, 0.70, 0.75, 0.95, 0.99);
    c->cd(4);
    DrawH1(fHM->H1("fhInnerRingFlag"));
    c->cd(5);
    DrawH1(fHM->H1("fhNofInnerHits"));
  }

  {
    fHM->CreateCanvas("DiRICHInRegion", "DiRICHInRegion", 800, 800);
    //c->SetLogy();
    DrawH1(fHM->H1("fhDiRICHsInRegion"));
  }

  {
    fHM->CreateCanvas("RichRingRadiusVsY", "RichRingRadiusVsY", 800, 800);
    //c->SetLogy();
    DrawH2(fHM->H2("fhRichRingRadiusY"));
  }

  {
    fHM->CreateCanvas("RichRingHitsVsRadius", "RichRingHitsVsRadius", 800, 800);
    //c->SetLogy();
    DrawH2(fHM->H2("fhRichHitsRingRadius"));
  }

  {
    fHM->CreateCanvas("RichHitToTEvent", "RichHitToTEvent", 800, 800);
    DrawH1(fHM->H1("fhRichHitToTEvent"));
  }

  {
    fHM->CreateCanvas("RichHitXYEvent", "RichHitXYEvent", 800, 800);
    DrawH2(fHM->H2("fhRichHitXYEvent"));
  }

  {
    fHM->CreateCanvas("rich_mcbm_fhBlobEvents", "rich_mcbm_fhBlobEvents", 600, 600);
    DrawH1(fHM->H1("fhNofBlobEvents"));
  }

  {
    fHM->CreateCanvas("rich_mcbm_fhBlobsInCbmEvent", "rich_mcbm_fhBlobsInCbmEvent", 600, 600);
    DrawH1(fHM->H1("fhNofBlobsInEvent"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofTrackRichHitXY", "TofTrackRichHitXY", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofTrackRichHitX"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofTrackRichHitY"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofTrackRichRingHitXY", "TofTrackRichRingHitXY", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofTrackRichRingHitX"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofTrackRichRingHitY"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofHitRichRingHitXY", "TofHitRichRingHitXY", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofHitRichRingHitX"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofHitRichRingHitY"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TrackRingDistance_Target", "TrackRingDistance_Target", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH1(fHM->H1("fhTrackRingDistanceOnTarget"));
    c->cd(2);
    DrawH1(fHM->H1("fhTrackRingDistanceOffTarget"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofTrackRichHitXY_cut", "TofTrackRichHitXY_cut", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofTrackRichHitX_cuts"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofTrackRichHitY_cuts"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofTrackRichHitXY_overBetaCut", "TofTrackRichHitXY_overBetaCut", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofTrackRichHitX_oBetacuts"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofTrackRichHitY_oBetacuts"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofTrackRichHitXY_underBetaCut", "TofTrackRichHitXY_underBetaCut", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofTrackRichHitX_uBetacuts"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofTrackRichHitY_uBetacuts"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TofTrackRichHitXY_oBetacuts_dtime", "TofTrackRichHitXY_oBetacuts_dtime", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofTrackRichHitX_oBetacuts_dtime"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofTrackRichHitY_oBetacuts_dtime"));
  }

  {
    TCanvas* c =
      fHM->CreateCanvas("TofTrackHitRichHitXY_oBetacuts_dtime", "TofTrackHitRichHitXY_oBetacuts_dtime", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTofTrackHitRichHitX_oBetacuts_dtime"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofTrackHitRichHitY_oBetacuts_dtime"));
  }

  {
    fHM->CreateCanvas("TofTrackRichHitTime_oBetacuts", "TofTrackRichHitTime_oBetacuts", 600, 600);
    DrawH1(fHM->H1("fhTofTrackRichHitTime_oBetacuts"));
  }


  {
    TCanvas* c = fHM->CreateCanvas("TofTrackHitRichHitXY_oBetacuts_dtime_evo",
                                   "TofTrackHitRichHitXY_oBetacuts_dtime_evo", 1200, 3200);
    c->Divide(2, 4);
    c->cd(1);
    DrawH2(fHM->H2("fhTofTrackRichHitX_oBetacuts_dtime_4"));
    c->cd(2);
    DrawH2(fHM->H2("fhTofTrackRichHitY_oBetacuts_dtime_4"));

    c->cd(3);
    DrawH2(fHM->H2("fhTofTrackRichHitX_oBetacuts_dtime_6"));
    c->cd(4);
    DrawH2(fHM->H2("fhTofTrackRichHitY_oBetacuts_dtime_6"));

    c->cd(5);
    DrawH2(fHM->H2("fhTofTrackRichHitX_oBetacuts_dtime_8"));
    c->cd(6);
    DrawH2(fHM->H2("fhTofTrackRichHitY_oBetacuts_dtime_8"));

    c->cd(7);
    DrawH2(fHM->H2("fhTofTrackRichHitX_oBetacuts_dtime_10"));
    c->cd(8);
    DrawH2(fHM->H2("fhTofTrackRichHitY_oBetacuts_dtime_10"));
  }

  {
    fHM->CreateCanvas("TrackRingDistance_AfterCorr", "TrackRingDistance_AfterCorr", 600, 600);
    DrawH1(fHM->H1("fhTrackRingDistance_corr"));
  }

  {
    fHM->CreateCanvas("TracksWithRings", "TracksWithRings", 600, 600);
    DrawH1(fHM->H1("fhTracksWithRings"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("GoodRings", "GoodRings", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH1(fHM->H1("fhRichRingRadius_goodRing"));
    c->cd(2);
    DrawH1(fHM->H1("fhNofHitsInRing_goodRing"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("richRingBeta", "richRingBeta", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH1(fHM->H1("fhRichRingBeta"));
    c->cd(2);
    DrawH1(fHM->H1("fhRichRingBeta_GoodRing"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("TrackRingDistanceVSRingChi2", "TrackRingDistanceVSRingChi2", 1200, 800);
    c->Divide(2, 1);
    c->cd(1);
    DrawH2(fHM->H2("fhTrackRingDistanceVSRingChi2"));
    c->cd(2);
    DrawH2(fHM->H2("fhTrackRingDistanceVSRingChi2_goodRing"));
  }


  {
    TCanvas* c = fHM->CreateCanvas("RichRingChi2", "RichRingChi2", 1600, 800);
    //c->SetLogy();
    c->Divide(2, 1);
    c->cd(1);
    DrawH1(fHM->H1("fhRichRingChi2"));
    c->cd(2);
    DrawH1(fHM->H1("fhRichRingChi2_goodRing"));
  }

  {
    fHM->CreateCanvas("TofTracksXYRICH_Accectance", "TofTracksXYRICH_Accectance", 1200, 1200);
    DrawH2(fHM->H2("fhTofTracksXYRICH_Accectance"));
  }

  {
    fHM->CreateCanvas("HitTimeEvent", "HitTimeEvent", 1200, 1200);
    DrawH1(fHM->H1("fhHitTimeEvent"));
  }

  {
    TCanvas* c = fHM->CreateCanvas("Bmon", "Bmon", 2400, 800);
    //c->SetLogy();
    c->Divide(3, 1);
    c->cd(1);
    DrawH1(fHM->H1("fhBmonDigiMultiplicity"));
    c->cd(2);
    DrawH1(fHM->H1("fhBmonDigiTime"));
    c->cd(3);
    DrawH1(fHM->H1("fhBmonDigiTimeEvent"));
  }
}

void CbmRichMCbmQaReal::DrawRing(CbmRichRing* ring)
{
  std::vector<CbmTofTracklet*> track;
  this->DrawRing(ring, track);
}

void CbmRichMCbmQaReal::DrawRing(CbmRichRing* ring, std::vector<CbmTofTracklet*> track, bool full)
{
  //std::cout<<"#!#DRAW!!!"<<std::endl;
  if (fNofDrawnRings > 20) return;
  fNofDrawnRings++;
  stringstream ss;
  ss << "Event" << fNofDrawnRings;
  //fNofDrawnRings++;
  TCanvas* c = nullptr;
  if (full == true) {
    c = fHM->CreateCanvas(ss.str().c_str(), ss.str().c_str(), 800, 800);
  }
  else {
    c = fHM->CreateCanvas(ss.str().c_str(), ss.str().c_str(), 500, 500);
  }
  c->SetGrid(true, true);
  TH2D* pad = nullptr;
  if (full == true) {
    pad = new TH2D(ss.str().c_str(), (ss.str() + ";X [cm];Y [cm]").c_str(), 1, -15., 10., 1, -5., 20);
  }
  else {
    pad = new TH2D(ss.str().c_str(), (ss.str() + ";X [cm];Y [cm]").c_str(), 1, -5., 5., 1, -5., 5);
  }

  pad->SetStats(false);
  pad->Draw();

  if (full == true) {
    //rough Drawing of RichDetectorAcceptance
    TLine* line0 = new TLine(-6.25, 8, -6.25, 15.9);
    line0->Draw();
    TLine* line1 = new TLine(-6.25, 15.9, -1.05, 15.9);
    line1->Draw();
    TLine* line2 = new TLine(-1.05, 15.9, -1.05, 13.2);
    line2->Draw();
    TLine* line3 = new TLine(-1.05, 13.2, +4.25, 13.2);
    line3->Draw();
    TLine* line4 = new TLine(4.25, 13.2, +4.25, 8);
    line4->Draw();
    TLine* line5 = new TLine(4.25, 8, -6.25, 8);
    line5->Draw();
  }

  // find min and max x and y positions of the hits
  // in order to shift drawing
  double xCur = 0.;
  double yCur = 0.;

  if (full == false) {
    double xmin = 99999., xmax = -99999., ymin = 99999., ymax = -99999.;
    for (int i = 0; i < ring->GetNofHits(); i++) {
      Int_t hitInd    = ring->GetHit(i);
      CbmRichHit* hit = (CbmRichHit*) fRichHits->At(hitInd);
      if (nullptr == hit) continue;
      if (xmin > hit->GetX()) xmin = hit->GetX();
      if (xmax < hit->GetX()) xmax = hit->GetX();
      if (ymin > hit->GetY()) ymin = hit->GetY();
      if (ymax < hit->GetY()) ymax = hit->GetY();
    }
    xCur = (xmin + xmax) / 2.;
    yCur = (ymin + ymax) / 2.;
  }

  //Draw circle and center
  TEllipse* circle = new TEllipse(ring->GetCenterX() - xCur, ring->GetCenterY() - yCur, ring->GetRadius());
  circle->SetFillStyle(0);
  circle->SetLineWidth(3);
  circle->Draw();
  TEllipse* center = new TEllipse(ring->GetCenterX() - xCur, ring->GetCenterY() - yCur, .1);
  center->Draw();


  double hitZ      = 0;
  uint nofDrawHits = 0;

  // Draw hits
  for (int i = 0; i < ring->GetNofHits(); i++) {
    Int_t hitInd    = ring->GetHit(i);
    CbmRichHit* hit = (CbmRichHit*) fRichHits->At(hitInd);
    if (nullptr == hit) continue;
    TEllipse* hitDr = new TEllipse(hit->GetX() - xCur, hit->GetY() - yCur, .25);
    //std::cout<<"LE of Hit: "<< hit->GetTime()- fCbmEventStartTime << "\t" << hit->GetTime() << "\t" << fCbmEventStartTime <<std::endl;
    if (doToT(hit)) {  // Good ToT selection
      hitDr->SetFillColor(kRed);
    }
    else {
      hitDr->SetFillColor(kBlue);
    }
    hitZ += hit->GetZ();
    nofDrawHits++;
    hitDr->Draw();
  }
  hitZ /= nofDrawHits;

  int Tc = 0;

  //Draw Tracks
  if (track.size() > 0) {
    stringstream ss3;
    std::string sTrackLabel;
    double dist = -99999.999;
    for (auto trackInd : track) {
      TEllipse* hitDr = new TEllipse(trackInd->GetFitX(hitZ) - xCur, trackInd->GetFitY(hitZ) - yCur, .25);
      hitDr->SetFillColor(kGreen);
      hitDr->Draw();
      //ss3 << "\\beta : "<< getBeta(trackInd); //inVel <<"x10^7 m/s";
      if (trackInd->GetFitX(hitZ) < 30.0) {  // Track on correct side of ToF
        Tc++;
        double tmp_dist = std::sqrt(trackInd->GetFitX(hitZ) * trackInd->GetFitX(hitZ)
                                    + trackInd->GetFitY(hitZ) * trackInd->GetFitY(hitZ));
        if (dist < -99999.0) {
          dist        = tmp_dist;
          sTrackLabel = "\\beta : " + std::to_string((double) getBeta(trackInd));
        }
        else if (dist > tmp_dist) {
          dist        = tmp_dist;
          sTrackLabel = "\\beta : " + std::to_string((double) getBeta(trackInd));
        }
      }
    }
    ss3 << sTrackLabel;
    TLatex* latex1 = new TLatex(-4., 0.5, ss3.str().c_str());
    latex1->Draw();
  }
  else {
    //std::cout<<"No Tracks to Draw."<<std::endl;
  }


  //Draw information
  stringstream ss2;
  if (full == false) {
    ss2 << "(x,y,r,n)=(" << setprecision(3) << ring->GetCenterX() << ", " << ring->GetCenterY() << ", "
        << ring->GetRadius() << ", " << ring->GetNofHits() << ")";
  }
  else {
    ss2 << "(x,y,r,n,T,Tc)=(" << setprecision(3) << ring->GetCenterX() << ", " << ring->GetCenterY() << ", "
        << ring->GetRadius() << ", " << ring->GetNofHits() << ", " << track.size() << ", " << Tc << ")";
  }
  TLatex* latex = nullptr;
  if (full == true) {
    latex = new TLatex(ring->GetCenterX() - 13., ring->GetCenterY() + 5., ss2.str().c_str());
  }
  else {
    latex = new TLatex(-4., 4., ss2.str().c_str());
  }
  latex->Draw();
}

void CbmRichMCbmQaReal::DrawRichTofEv(const std::vector<int> richHitIndx, const std::vector<int> tofTrackIndx)
{
  if (richHitIndx.size() < 1 || tofTrackIndx.size() < 1) return;
  if (fNofDrawnRichTofEv > 30) return;
  fNofDrawnRichTofEv++;
  std::string dir = "./" + fOutputDir + "/png/TREv/";
  gSystem->mkdir(dir.c_str(), true);  // create directory if it does not exist
  stringstream ss;
  ss << "TREv/TofRichEvent" << fNofDrawnRichTofEv;
  TCanvas* c = nullptr;
  c          = fHM->CreateCanvas(ss.str().c_str(), ss.str().c_str(), 800, 800);
  c->SetGrid(true, true);

  TH2D* pad = new TH2D(ss.str().c_str(), (ss.str() + ";X [cm];Y [cm]").c_str(), 1, -15., 10., 1, -5., 20);

  pad->SetStats(false);
  pad->Draw();

  //rough Drawing of RichDetectorAcceptance
  TLine* line0 = new TLine(-6.25, 8, -6.25, 15.9);
  line0->Draw();
  TLine* line1 = new TLine(-6.25, 15.9, -1.05, 15.9);
  line1->Draw();
  TLine* line2 = new TLine(-1.05, 15.9, -1.05, 13.2);
  line2->Draw();
  TLine* line3 = new TLine(-1.05, 13.2, +4.25, 13.2);
  line3->Draw();
  TLine* line4 = new TLine(4.25, 13.2, +4.25, 8);
  line4->Draw();
  TLine* line5 = new TLine(4.25, 8, -6.25, 8);
  line5->Draw();

  int nofDrawHits = 0;
  double hitZ     = 0.;

  for (unsigned int i = 0; i < richHitIndx.size(); i++) {
    CbmRichHit* hit = (CbmRichHit*) fRichHits->At(richHitIndx[i]);
    if (nullptr == hit) continue;
    TEllipse* hitDr = new TEllipse(hit->GetX(), hit->GetY(), .25);
    if (doToT(hit)) {  // Good ToT selection
      hitDr->SetFillColor(kRed);
    }
    else {
      hitDr->SetFillColor(kBlue);
    }
    hitZ += hit->GetZ();
    nofDrawHits++;
    hitDr->Draw();
  }
  if (nofDrawHits != 0) {
    hitZ /= nofDrawHits;
  }
  else {
    hitZ = RichZPos;
  }

  stringstream ss2;

  for (unsigned int i = 0; i < tofTrackIndx.size(); i++) {
    CbmTofTracklet* track = (CbmTofTracklet*) fTofTracks->At(tofTrackIndx[i]);
    if (track == nullptr) continue;
    TEllipse* trDr = new TEllipse(track->GetFitX(hitZ), track->GetFitY(hitZ), .25);
    trDr->SetFillColor(kGreen);
    trDr->Draw();

    ss2 << "\\beta: " << getBeta(track);
  }

  TLatex* latex = new TLatex(-4., 4., ss2.str().c_str());

  latex->Draw();
}


void CbmRichMCbmQaReal::Finish()
{
  //std::cout<<"Tracks:  "<< fTofTracks->GetEntriesFast() <<std::endl;
  std::cout << "Drawing Hists...";
  DrawHist();
  std::cout << "DONE!" << std::endl;

  if (this->fDoDrawCanvas) {
    fHM->SaveCanvasToImage(fOutputDir, "png");
    std::cout << "Canvas saved to Images!" << std::endl;
  }

  if (this->fDoWriteHistToFile) {
    /// Save old global file and folder pointer to avoid messing with FairRoot
    TFile* oldFile    = gFile;
    TDirectory* oldir = gDirectory;

    std::string s  = fOutputDir + "/RecoHists.root";
    TFile* outFile = new TFile(s.c_str(), "RECREATE");
    if (outFile->IsOpen()) {
      fHM->WriteToFile();
      std::cout << "Written to Root-file \"" << s << "\"  ...";
      outFile->Close();
      std::cout << "Done!" << std::endl;
    }
    /// Restore old global file and folder pointer to avoid messing with FairRoot
    gFile = oldFile;
    gDirectory->cd(oldir->GetPath());
  }

  std::cout << "fTracksinRich: " << fTracksinRich << std::endl;
  Double_t result = (static_cast<Double_t>(fTracksinRichWithRichHits[0]) / static_cast<Double_t>(fTracksinRich)) * 100.;
  std::cout << "fTracksinRichWithRichHits >  0: " << fTracksinRichWithRichHits[0] << "\t Percent: " << result << "%"
            << std::endl;
  result = (static_cast<Double_t>(fTracksinRichWithRichHits[1]) / static_cast<Double_t>(fTracksinRich)) * 100.;
  std::cout << "fTracksinRichWithRichHits >  5: " << fTracksinRichWithRichHits[1] << "\t Percent: " << result << "%"
            << std::endl;
  result = (static_cast<Double_t>(fTracksinRichWithRichHits[2]) / static_cast<Double_t>(fTracksinRich)) * 100.;
  std::cout << "fTracksinRichWithRichHits > 10: " << fTracksinRichWithRichHits[2] << "\t Percent: " << result << "%"
            << std::endl;
  result = (static_cast<Double_t>(fTracksinRichWithRichHits[3]) / static_cast<Double_t>(fTracksinRich)) * 100.;
  std::cout << "fTracksinRichWithRichHits > 15: " << fTracksinRichWithRichHits[3] << "\t Percent: " << result << "%"
            << std::endl;


  std::cout << "RingsWithTrack: " << fRingsWithTrack[0] << "   Rings: " << fRingsWithTrack[1]
            << "   Tracks: " << fRingsWithTrack[2] << std::endl;
  std::cout << "\t \t Rings Cutted: " << fRingsWithTrack[3] << "   Tracks Cutted: " << fRingsWithTrack[4]
            << "   Possible Combinations: " << fRingsWithTrack[5] << std::endl;
}


void CbmRichMCbmQaReal::DrawFromFile(const string& fileName, const string& outputDir)
{
  fOutputDir = outputDir;

  /// Save old global file and folder pointer to avoid messing with FairRoot
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  if (fHM != nullptr) delete fHM;

  fHM         = new CbmHistManager();
  TFile* file = new TFile(fileName.c_str());
  fHM->ReadFromFile(file);
  DrawHist();

  fHM->SaveCanvasToImage(fOutputDir);

  /// Restore old global file and folder pointer to avoid messing with FairRoot
  gFile      = oldFile;
  gDirectory = oldDir;
}

bool CbmRichMCbmQaReal::isAccmRICH(CbmTofTracklet* track)
{
  //check if Track is in mRICH acceptance

  bool inside = false;
  if (!fRestrictToAcc) return true;
  double x = track->GetFitX(RichZPos);
  double y = track->GetFitY(RichZPos);
  if (x >= -6.25 && x <= -1.05) {
    // left part of mRICH
    if (y > 8 && y < 15.9) {
      inside = true;
    }
  }
  else if (x > -1.05 && x < 4.25) {
    //right part
    if (y > 8 && y < 13.2) {
      inside = true;
    }
  }

  return inside;
}


Double_t CbmRichMCbmQaReal::getBeta(CbmTofTracklet* track)
{
  Double_t inVel = 1e7 / (track->GetTt());  // in m/s
  Double_t beta  = inVel / TMath::C();

  return beta;
}

Double_t CbmRichMCbmQaReal::getBeta(const CbmRichRing* ring)
{

  // calculate distance of ring center to penetration point in aerogel with assumption,
  // that particle is from target.
  // INFO: use center of Aerogel as reference -> 11.5cm to MAPMT (TODO: get this from GEO-File)
  const Double_t Aerogel_Dist = 11.5;
  const Double_t aeroPenetr_Y = ring->GetCenterY() * (1. - Aerogel_Dist / (RichZPos));
  const Double_t aeroPenetr_X = ring->GetCenterX() * (1. - Aerogel_Dist / (RichZPos));

  const Double_t dist = std::sqrt((ring->GetCenterX() - aeroPenetr_X) * (ring->GetCenterX() - aeroPenetr_X)
                                  + (ring->GetCenterY() - aeroPenetr_Y) * (ring->GetCenterY() - aeroPenetr_Y)
                                  + Aerogel_Dist * Aerogel_Dist);

  const Double_t ioR = 1.05;  // Index of Reflection

  Double_t beta = std::sqrt((((ring->GetRadius() * ring->GetRadius()) / dist * dist) + 1) / (ioR * ioR));

  //std::cout<<"     beta:"<<beta<<std::endl;

  return beta;
}

bool CbmRichMCbmQaReal::RestrictToFullAcc(CbmTofTracklet* track)
{
  //check if Track is in mRICH acceptance

  double x = track->GetFitX(RichZPos);
  double y = track->GetFitY(RichZPos);
  return this->RestrictToFullAcc(x, y);
}

bool CbmRichMCbmQaReal::RestrictToFullAcc(TVector3& pos)
{
  Double_t x = pos.X();
  Double_t y = pos.Y();

  return this->RestrictToFullAcc(x, y);
}

bool CbmRichMCbmQaReal::RestrictToFullAcc(Double_t x, Double_t y)
{  //check if Track is in mRICH acceptance
  if (fRestrictToFullAcc == false) return true;
  bool inside = false;
  if (x >= -16.85 && x <= 4.25) {  //TODO:check values
    // left part of mRICH
    if (y >= -23.8 && y <= 23.8) {
      inside = true;
    }
  }

  return inside;
}

void CbmRichMCbmQaReal::analyseRing(const CbmRichRing* ring, CbmEvent* ev, std::pair<int, double>& clTrack)
{

  //std::cout<<"analyse a Ring"<<std::endl;

  Double_t meanTime   = 0.;
  unsigned int hitCnt = 0;
  Double_t minRHit2   = std::numeric_limits<Double_t>::max();
  for (int i = 0; i < ring->GetNofHits(); i++) {
    Int_t hitInd    = ring->GetHit(i);
    CbmRichHit* hit = (CbmRichHit*) fRichHits->At(hitInd);
    if (nullptr == hit) continue;

    meanTime += hit->GetTime();
    hitCnt++;

    const Float_t diffX         = hit->GetX() - ring->GetCenterX();
    const Float_t diffY         = hit->GetY() - ring->GetCenterY();
    const Float_t tmpHitRadius2 = (diffX * diffX + diffY * diffY);

    if (tmpHitRadius2 < minRHit2) {
      minRHit2 = tmpHitRadius2;
    }
  }
  meanTime = meanTime / hitCnt;

  //std::cout<<"mean: "<<meanTime<<std::endl;
  for (int i = 0; i < ring->GetNofHits(); i++) {
    Int_t hitInd    = ring->GetHit(i);
    CbmRichHit* hit = (CbmRichHit*) fRichHits->At(hitInd);
    if (nullptr == hit) continue;
    //std::cout<<"DeltatTime: "<< meanTime - hit->GetTime()<<std::endl;
    //fHM->H1("fhRingDeltaTime")->Fill(static_cast<Double_t>(meanTime - hit->GetTime()));

    fHM->H1("fhRingToTs")->Fill(hit->GetToT());
    fHM->H1("fhRingLE")->Fill(static_cast<Double_t>(hit->GetTime() - ev->GetStartTime()));
    fHM->H2("fhRingLEvsToT")->Fill(static_cast<Double_t>(hit->GetTime() - ev->GetStartTime()), hit->GetToT());
    //std::vector<int> tmpRingIndx;
    //tmpRingIndx.push_back(ring->GetIndex);
    const Double_t Tdiff_ring = (hit->GetTime() - ev->GetStartTime());
    if ((Tdiff_ring > 20.) && (Tdiff_ring < 30.)) {
      // std::cout<<ev->GetNumber()<<" Address_ring: "<<std::hex<< CbmRichUtil::GetDirichId(hit->GetAddress())<<std::dec<<"  "<< CbmRichUtil::GetDirichChannel(hit->GetAddress()) <<"  "<< hit->GetToT()<<"  "<<ring->GetRadius()<<std::endl;
      //fHM->H1("fhDiRICHsInRegion")->Fill(CbmRichUtil::GetDirichId(hit->GetAddress()));
    }

    if (clTrack.first == -1)
      fHM->H1("fhRingNoClTrackLE")->Fill(static_cast<Double_t>(hit->GetTime() - ev->GetStartTime()));
    if ((clTrack.first >= 0) && !(clTrack.second < 10.))
      fHM->H1("fhRingClTrackFarAwayLE")->Fill(static_cast<Double_t>(hit->GetTime() - ev->GetStartTime()));
    if (cutDistance(clTrack) && cutRadius(ring)) {  //Good Ring
      fHM->H1("fhGoodRingLE")->Fill(static_cast<Double_t>(hit->GetTime() - ev->GetStartTime()));
      fHM->H1("fhRingDeltaTime")->Fill(static_cast<Double_t>(meanTime - hit->GetTime()));
    }
  }

  int InnerHitCnt     = 0;
  int InnerHitCnt_cut = 0;
  for (size_t j = 0; j < ev->GetNofData(ECbmDataType::kRichHit); j++) {
    auto iRichHit       = ev->GetIndex(ECbmDataType::kRichHit, j);
    CbmRichHit* richHit = static_cast<CbmRichHit*>(fRichHits->At(iRichHit));
    if (nullptr == richHit) continue;
    const Float_t diffX = richHit->GetX() - ring->GetCenterX();
    const Float_t diffY = richHit->GetY() - ring->GetCenterY();
    //select inner Part of Ring
    if (diffX * diffX + diffY * diffY < minRHit2) {
      InnerHitCnt++;
      const Double_t Tdiff_inner = (richHit->GetTime() - ev->GetStartTime());
      if ((Tdiff_inner > 20.) && (Tdiff_inner < 30.)) {
        InnerHitCnt_cut++;
        //if (InnerHitCnt_cut == 1) {DrawRing(ring);}
        //std::cout<<ev->GetNumber()<<" Address_inner: "<<std::hex<< CbmRichUtil::GetDirichId(richHit->GetAddress())<<std::dec<<"  "<< CbmRichUtil::GetDirichChannel(richHit->GetAddress()) <<"  "<< richHit->GetToT()<<"  "<<ring->GetRadius()<<std::endl;
        fHM->H1("fhDiRICHsInRegion")->Fill(CbmRichUtil::GetDirichId(richHit->GetAddress()));
      }

      fHM->H1("fhInnerRingDeltaTime")->Fill(static_cast<Double_t>(meanTime - richHit->GetTime()));
      fHM->H1("fhInnerRingToTs")->Fill(richHit->GetToT());
      fHM->H1("fhInnerRingLE")->Fill(static_cast<Double_t>(richHit->GetTime() - ev->GetStartTime()));
      if (clTrack.first == -1)
        fHM->H1("fhInnerRingNoClTrackLE")->Fill(static_cast<Double_t>(richHit->GetTime() - ev->GetStartTime()));
      if ((clTrack.first >= 0) && !(clTrack.second < 5.))
        fHM->H1("fhInnerRingClTrackFarAwayLE")->Fill(static_cast<Double_t>(richHit->GetTime() - ev->GetStartTime()));
      if (cutDistance(clTrack) && cutRadius(ring)) {  //Good Ring
        fHM->H1("fhInnerGoodRingLE")->Fill(static_cast<Double_t>(richHit->GetTime() - ev->GetStartTime()));
      }
    }
  }
  if (InnerHitCnt == 0) {
    fHM->H1("fhInnerRingFlag")->Fill(1);
  }
  else {
    fHM->H1("fhInnerRingFlag")->Fill(0);
  }
  fHM->H1("fhNofInnerHits")->Fill(InnerHitCnt);
}


Bool_t CbmRichMCbmQaReal::cutRadius(const CbmRichRing* ring)
{
  if (ring->GetRadius() > 2. && ring->GetRadius() < 4.2) return true;

  return false;
}


Bool_t CbmRichMCbmQaReal::cutDistance(std::pair<int, double>& clTrack)
{
  if ((clTrack.first >= 0) && (clTrack.second < 10.)) return true;

  return false;
}


TVector3 CbmRichMCbmQaReal::extrapolate(CbmTofHit* tofHit, Double_t Z)
{
  TVector3 extVec(0, 0, 0);
  TVector3 vertex(0, 0, 0);
  Double_t factor = (Z - vertex.Z()) / (tofHit->GetZ() - vertex.Z());
  Double_t x      = vertex.X() + factor * (tofHit->GetX() - vertex.X());
  Double_t y      = vertex.Y() + factor * (tofHit->GetY() - vertex.Y());
  extVec.SetXYZ(x, y, Z);

  return extVec;
}

ClassImp(CbmRichMCbmQaReal)
