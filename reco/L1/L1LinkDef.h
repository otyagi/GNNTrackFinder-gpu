/* Copyright (C) 2006-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Denis Bertini [committer] */

// $Id: L1LinkDef.h,v 1.10 2006/05/18 11:36:47 friese Exp $

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ enum class cbm::algo::ca::ETimer;
#pragma link C++ enum class cbm::algo::ca::ECounter;
#pragma link C++ enum class cbm::algo::ca::EDetectorID;
#pragma link C++ class cbm::algo::ca::TrackingMonitor - ;
//#pragma link C++ class cbm::algo::ca::InitManager - ;
#pragma link C++ class CbmTrackingDetectorInterfaceInit + ;
#pragma link C++ class CbmL1 + ;
#pragma link C++ class CbmL1StsTrackFinder + ;
//#pragma link C++ class  CbmL1MuchFinder+;
//#pragma link C++ class  CbmL1MuchHit+;
//#pragma link C++ class  CbmL1MuchTrack+;
//#pragma link C++ class  CbmL1MuchFinderQa+;
#pragma link C++ class CbmL1GlobalTrackFinder + ;
#pragma link C++ class CbmL1GlobalFindTracksEvents + ;
#pragma link C++ class CbmGenerateMaterialMaps + ;
//#pragma link C++ class  CbmL1SttHit+;
//#pragma link C++ class  CbmL1SttTrackFinder+;
//#pragma link C++ class  CbmL1SttTrack+;
#pragma link C++ class CbmCaInputQaMvd + ;
#pragma link C++ class CbmCaInputQaMuch + ;
#pragma link C++ class CbmCaInputQaSts + ;
#pragma link C++ class CbmCaInputQaTrd + ;
#pragma link C++ class CbmCaInputQaTof + ;
#pragma link C++ enum cbm::ca::ETrackType;
#pragma link C++ class cbm::ca::OutputQa + ;
#pragma link C++ class cbm::ca::tools::WindowFinder + ;
//#pragma link C++ class cbm::ca::IdealHitProducer < L1DetectorID::kMvd > + ;
//#pragma link C++ class cbm::ca::IdealHitProducer < L1DetectorID::kSts > + ;
//#pragma link C++ class cbm::ca::IdealHitProducer < L1DetectorID::kMuch > + ;
//#pragma link C++ class cbm::ca::IdealHitProducer < L1DetectorID::kTrd > + ;
//#pragma link C++ class cbm::ca::IdealHitProducer < L1DetectorID::kTof > + ;
#pragma link C++ class cbm::ca::IdealHitProducer + ;
#pragma link C++ class cbm::ca::tools::MaterialHelper + ;

#endif
