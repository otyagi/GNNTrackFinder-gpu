/* Copyright (C) 2020-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Norbert Herrmann, Florian Uhlig [committer] */

/** @file CbmTofCalibrator.cxx
 ** @author nh
 ** @date 28.02.2020
 **
 **/

// CBMroot classes and includes
#include "CbmTofCalibrator.h"

#include "CbmEvent.h"
#include "CbmMatch.h"
#include "CbmTofAddress.h"  // in cbmdata/tof
#include "CbmTofCell.h"     // in tof/TofData
#include "CbmTofClusterizersDef.h"
#include "CbmTofDetectorId_v21a.h"  // in cbmdata/tof
#include "CbmTofDigiBdfPar.h"       // in tof/TofParam
#include "CbmTofDigiPar.h"          // in tof/TofParam
#include "CbmTofGeoHandler.h"       // in tof/TofTools
#include "CbmTofHit.h"
#include "CbmTofTracklet.h"

// FAIR classes and includes
#include "FairRunAna.h"
#include "FairRuntimeDb.h"

#include <Logger.h>

// ROOT Classes and includes
#include "TF1.h"

#include <TClonesArray.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TFitResult.h>
#include <TGeoManager.h>
#include <TH1.h>
#include <TH2.h>
#include <TH3.h>
#include <TMinuit.h>
#include <TProfile.h>
#include <TROOT.h>

const int MaxShift            = 10;       // number of 50ps bins
const double cLight           = 29.9792;  // in cm/ns
const TString cBadChannelFile = "TofBadChannels.txt";
const double dValidEdge       = 1.0;  // FIXME: constants in code

std::vector<TH1*> hSvel;

CbmTofCalibrator::CbmTofCalibrator()
  : fDigiMan(NULL)
  , fTofClusterizer(NULL)
  , fTofFindTracks(NULL)
  , fTrackletTools(NULL)
  , fDigiPar(NULL)
  , fDigiBdfPar(NULL)
  , fTofDigiMatchColl(NULL)
  , fhCalR0(NULL)
  , fhCalDX0(NULL)
  , fhCalDY0(NULL)
  , fhCalCounterDt(NULL)
  , fhCalCounterDy(NULL)
  , fhCalChannelDt(NULL)
  , fhCalChannelDy(NULL)
  , fhCalTot()
  , fhCalPosition()
  , fhCalPos()
  , fhCalTOff()
  , fhCalTofOff()
  , fhCalDelPos()
  , fhCalDelTOff()
  , fhCalCluTrms()
  , fhCalCluSize()
  , fhCalWalk()
  , fhCalDtWalk()
  , fhCorPos()
  , fhCorTOff()
  , fhCorTot()
  , fhCorTotOff()
  , fhCorSvel()
  , fhCorWalk()
  , fDetIdIndexMap()
  , fhDoubletDt()
  , fhDoubletDd()
  , fhDoubletV()
{
}

CbmTofCalibrator::~CbmTofCalibrator() {}

InitStatus CbmTofCalibrator::Init()
{

  FairRootManager* fManager = FairRootManager::Instance();
  // Get access to TofCalDigis
  fTofCalDigiVec = fManager->InitObjectAs<std::vector<CbmTofDigi> const*>("TofCalDigi");
  //dynamic_cast<std::vector<CbmTofDigi> const*>(fManager->GetObject("TofCalDigi"));
  if (NULL == fTofCalDigiVec) LOG(warning) << "No access to TofCalDigis!";

  // check for availability of digis
  fDigiMan = CbmDigiManager::Instance();
  if (NULL == fDigiMan) {
    LOG(warning) << "No CbmDigiManager";
    return kFATAL;
  }
  fDigiMan->Init();

  if (!fDigiMan->IsPresent(ECbmModuleId::kTof)) {
    LOG(warn) << "CbmTofCalibrator: No digi input!";
  }

  // check for access to current calibration file
  fTofClusterizer = CbmTofEventClusterizer::Instance();
  if (NULL == fTofClusterizer) {
    LOG(warn) << "CbmTofCalibrator: no access to active calibration";
  }
  else {
    TString CalParFileName = fTofClusterizer->GetCalParFileName();
    LOG(info) << "Current calibration file: " << CalParFileName;
  }
  fTofFindTracks = CbmTofFindTracks::Instance();
  if (NULL == fTofFindTracks) {
    LOG(warn) << "CbmTofCalibrator: no access to tof tracker ";
  }

  fEvtHeader = (FairEventHeader*) fManager->GetObject("EventHeader.");
  if (NULL == fEvtHeader) {
    LOG(warning) << "CbmTofCalibrator::RegisterInputs => Could not get EvtHeader Object";
    return kFATAL;
  }
  // Get Access to MatchCollection
  fTofDigiMatchColl = (TClonesArray*) fManager->GetObject("TofHitCalDigiMatch");
  if (NULL == fTofDigiMatchColl) {
    LOG(warn) << "No branch TofHitCalDigiMatch found, try TofHitDigiMatch";
    fTofDigiMatchColl = (TClonesArray*) fManager->GetObject("TofHitDigiMatch");
  }

  if (NULL == fTofDigiMatchColl) {
    LOG(error) << "CbmTofCalibrator: no access to DigiMatch ";
    return kFATAL;
  }
  LOG(info) << Form("Got DigiMatchColl %s at %p", fTofDigiMatchColl->GetName(), fTofDigiMatchColl);

  // Get Parameters
  if (!InitParameters()) {
    LOG(error) << "CbmTofCalibrator: No parameters!";
  }
  // generate deviation histograms
  if (!CreateCalHist()) {
    LOG(error) << "CbmTofCalibrator: No histograms!";
    return kFATAL;
  }

  fTofId = new CbmTofDetectorId_v21a();

  fTrackletTools = new CbmTofTrackletTools();  // initialize tools

  TString shcmd = Form("rm %s  ", cBadChannelFile.Data());
  gSystem->Exec(shcmd.Data());

  shcmd = Form("touch %s  ", cBadChannelFile.Data());
  gSystem->Exec(shcmd.Data());

  LOG(info) << "TofCalibrator initialized successfully";
  return kSUCCESS;
}

Bool_t CbmTofCalibrator::InitParameters()
{
  LOG(info) << "InitParameters: get par pointers from RTDB";
  // Get Base Container
  FairRun* ana        = FairRun::Instance();
  FairRuntimeDb* rtdb = ana->GetRuntimeDb();

  fDigiPar = (CbmTofDigiPar*) (rtdb->getContainer("CbmTofDigiPar"));
  if (NULL == fDigiPar) return kFALSE;

  fDigiBdfPar = (CbmTofDigiBdfPar*) (rtdb->getContainer("CbmTofDigiBdfPar"));
  if (NULL == fDigiBdfPar) return kFALSE;

  return kTRUE;
}

Bool_t CbmTofCalibrator::CreateCalHist()
{
  TDirectory* oldir = gDirectory;  // <= To prevent histos from being sucked in by the param file of the TRootManager!
  gROOT->cd();                     // <= To prevent histos from being sucked in by the param file of the TRootManager !

  // detector related distributions
  Int_t iNbDet = fDigiBdfPar->GetNbDet();
  LOG(info) << "Define Calibrator histos for " << iNbDet << " detectors "
            << "in directory " << gDirectory->GetName() << ", old " << oldir->GetName();

  fhCalR0  = new TH1D("hCalR0", "Tracklet distance to nominal vertex; R_0 [cm]", 100, 0., 0.5);
  fhCalDX0 = new TH1D("hCalDX0", "Tracklet distance to nominal vertex; #DeltaX_0 [cm]", 100, -1.5, 1.5);
  fhCalDY0 = new TH1D("hCalDY0", "Tracklet distance to nominal vertex; #DeltaY_0 [cm]", 100, -1.5, 1.5);

  fhCalCounterDt = new TH1D("hCalCounterDt", "CalibCounterDt ; #Deltat [ns]", 100, -0.2, 0.2);
  fhCalCounterDy = new TH1D("hCalCounterDy", "CalibCounterDy ; #Deltay [cm]", 100, -0.8, 0.8);
  fhCalChannelDt = new TH1D("hCalChannelDt", "CalibChannelDt ; #Deltat [ns]", 100, -0.25, 0.25);
  fhCalChannelDy = new TH1D("hCalChannelDy", "CalibChannelDy ; #Deltat [ns]", 100, -1.8, 1.8);

  fhCalTot.resize(iNbDet);
  fhCalPosition.resize(iNbDet);
  fhCalPos.resize(iNbDet);
  fhCalTOff.resize(iNbDet);
  fhCalTofOff.resize(iNbDet);
  fhCalDelPos.resize(iNbDet);
  fhCalDelTOff.resize(iNbDet);
  fhCalCluTrms.resize(iNbDet);
  fhCalCluSize.resize(iNbDet);
  fhCalWalk.resize(iNbDet);
  fhCalDtWalk.resize(iNbDet);
  fhCalWalkAv.resize(iNbDet);

  fhCalXYTOff.resize(iNbDet);
  fhCalXYTot.resize(iNbDet);
  fhCalTotYWalk.resize(iNbDet);
  fhCalTotYTOff.resize(iNbDet);

  fDetIdIndexMap.clear();

  for (Int_t iDetIndx = 0; iDetIndx < iNbDet; iDetIndx++) {
    Int_t iUniqueId           = fDigiBdfPar->GetDetUId(iDetIndx);
    fDetIdIndexMap[iUniqueId] = iDetIndx;

    Int_t iSmType            = CbmTofAddress::GetSmType(iUniqueId);
    Int_t iSmId              = CbmTofAddress::GetSmId(iUniqueId);
    Int_t iRpcId             = CbmTofAddress::GetRpcId(iUniqueId);
    Int_t iUCellId           = CbmTofAddress::GetUniqueAddress(iSmId, iRpcId, 0, 0, iSmType);
    CbmTofCell* fChannelInfo = fDigiPar->GetCell(iUCellId);
    if (NULL == fChannelInfo) {
      LOG(warning) << "No DigiPar for Det " << Form("0x%08x", iUCellId);
      continue;
    }
    Double_t YMAX = TMath::Max(2., fChannelInfo->GetSizey()) * 0.75;

    Double_t TotMax    = 20.;
    fhCalTot[iDetIndx] = new TH2F(
      Form("cal_SmT%01d_sm%03d_rpc%03d_Tot", iSmType, iSmId, iRpcId),
      Form("Clu Tot of Rpc #%03d in Sm %03d of type %d; Strip []; Tot [a.u.]", iRpcId, iSmId, iSmType),
      2 * fDigiBdfPar->GetNbChan(iSmType, iRpcId), 0, 2 * fDigiBdfPar->GetNbChan(iSmType, iRpcId), 100, 0., TotMax);

    fhCalXYTot[iDetIndx] =
      new TH3F(Form("calXY_SmT%01d_sm%03d_rpc%03d_Tot", iSmType, iSmId, iRpcId),
               Form("Clu Tot of Rpc #%03d in Sm %03d of type %d; Strip []; y [cm]; Tot [a.u.]", iRpcId, iSmId, iSmType),
               2 * fDigiBdfPar->GetNbChan(iSmType, iRpcId), 0, 2 * fDigiBdfPar->GetNbChan(iSmType, iRpcId), 20, -YMAX,
               YMAX, 100, 0., TotMax);

    fhCalPosition[iDetIndx] =
      new TH2F(Form("cal_SmT%01d_sm%03d_rpc%03d_Position", iSmType, iSmId, iRpcId),
               Form("Clu position of Rpc #%03d in Sm %03d of type %d; Strip []; ypos [cm]", iRpcId, iSmId, iSmType),
               fDigiBdfPar->GetNbChan(iSmType, iRpcId), 0, fDigiBdfPar->GetNbChan(iSmType, iRpcId), 100, -YMAX, YMAX);

    Double_t YDMAX = 5;
    fhCalPos[iDetIndx] =
      new TH2F(Form("cal_SmT%01d_sm%03d_rpc%03d_Pos", iSmType, iSmId, iRpcId),
               Form("Clu pos deviation of Rpc #%03d in Sm %03d of type %d; "
                    "Strip []; ypos [cm]",
                    iRpcId, iSmId, iSmType),
               fDigiBdfPar->GetNbChan(iSmType, iRpcId), 0, fDigiBdfPar->GetNbChan(iSmType, iRpcId), 99, -YDMAX, YDMAX);

    Double_t TSumMax = 2.;
    //if(iSmType == 5) TSumMax *= 2.; // enlarge for diamond / beamcounter
    fhCalTOff[iDetIndx] = new TH2F(
      Form("cal_SmT%01d_sm%03d_rpc%03d_TOff", iSmType, iSmId, iRpcId),
      Form("Clu T0 deviation of Rpc #%03d in Sm %03d of type %d; Strip []; TOff [ns]", iRpcId, iSmId, iSmType),
      fDigiBdfPar->GetNbChan(iSmType, iRpcId), 0, fDigiBdfPar->GetNbChan(iSmType, iRpcId), 200, -TSumMax, TSumMax);

    fhCalXYTOff[iDetIndx] = new TH3F(
      Form("calXY_SmT%01d_sm%03d_rpc%03d_TOff", iSmType, iSmId, iRpcId),
      Form("XY T0 deviation of Rpc #%03d in Sm %03d of type %d; Strip []; y[cm]; TOff [ns]", iRpcId, iSmId, iSmType),
      fDigiBdfPar->GetNbChan(iSmType, iRpcId), 0, fDigiBdfPar->GetNbChan(iSmType, iRpcId), 100, -YMAX, YMAX, 100,
      -TSumMax * 0.5, TSumMax * 0.5);

    TSumMax               = 20.;
    fhCalTofOff[iDetIndx] = new TH2F(Form("cal_SmT%01d_sm%03d_rpc%03d_TofOff", iSmType, iSmId, iRpcId),
                                     Form("Clu T0 deviation of Rpc #%03d in Sm %03d of type %d; "
                                          "Strip []; TofOff [ns]",
                                          iRpcId, iSmId, iSmType),
                                     fDigiBdfPar->GetNbChan(iSmType, iRpcId), 0,
                                     fDigiBdfPar->GetNbChan(iSmType, iRpcId), 199, -TSumMax, TSumMax);

    fhCalDelPos[iDetIndx] =
      new TH2F(Form("cal_SmT%01d_sm%03d_rpc%03d_DelPos", iSmType, iSmId, iRpcId),
               Form("Clu position difference of Rpc #%03d in Sm %03d of type "
                    "%d; Strip []; #Deltaypos(clu) [cm]",
                    iRpcId, iSmId, iSmType),
               fDigiBdfPar->GetNbChan(iSmType, iRpcId), 0, fDigiBdfPar->GetNbChan(iSmType, iRpcId), 99, -10., 10.);

    fhCalDelTOff[iDetIndx] =
      new TH2F(Form("cal_SmT%01d_sm%03d_rpc%03d_DelTOff", iSmType, iSmId, iRpcId),
               Form("Clu TimeZero Difference of Rpc #%03d in Sm %03d of type %d; Strip "
                    "[]; #DeltaTOff(clu) [ns]",
                    iRpcId, iSmId, iSmType),
               fDigiBdfPar->GetNbChan(iSmType, iRpcId), 0, fDigiBdfPar->GetNbChan(iSmType, iRpcId), 99, -0.6, 0.6);

    fhCalCluTrms[iDetIndx] =
      new TH2D(Form("cal_SmT%01d_sm%03d_rpc%03d_Trms", iSmType, iSmId, iRpcId),
               Form("Clu Time RMS of Rpc #%03d in Sm %03d of type %d; Strip []; Trms [ns]", iRpcId, iSmId, iSmType),
               fDigiBdfPar->GetNbChan(iSmType, iRpcId), 0, fDigiBdfPar->GetNbChan(iSmType, iRpcId), 99, 0., 0.5);

    fhCalCluSize[iDetIndx] =
      new TH2D(Form("cal_SmT%01d_sm%03d_rpc%03d_Size", iSmType, iSmId, iRpcId),
               Form("Clu size of Rpc #%03d in Sm %03d of type %d; Strip []; size [strips]", iRpcId, iSmId, iSmType),
               fDigiBdfPar->GetNbChan(iSmType, iRpcId), 0, fDigiBdfPar->GetNbChan(iSmType, iRpcId), 16, 0.5, 16.5);

    TSumMax = 0.8;
    fhCalWalkAv[iDetIndx] =
      new TH2D(Form("cal_SmT%01d_sm%03d_rpc%03d_WalkAv", iSmType, iSmId, iRpcId),
               Form("Walk in SmT%01d_sm%03d_rpc%03d_WalkAv; Tot [a.u.];  #DeltaT [ns]", iSmType, iSmId, iRpcId),
               nbClWalkBinX, 0., TotMax, nbClWalkBinY, -TSumMax, TSumMax);

    fhCalWalk[iDetIndx].resize(fDigiBdfPar->GetNbChan(iSmType, iRpcId));
    fhCalDtWalk[iDetIndx].resize(fDigiBdfPar->GetNbChan(iSmType, iRpcId));
    fhCalTotYWalk[iDetIndx].resize(fDigiBdfPar->GetNbChan(iSmType, iRpcId));
    fhCalTotYTOff[iDetIndx].resize(fDigiBdfPar->GetNbChan(iSmType, iRpcId));

    for (Int_t iCh = 0; iCh < fDigiBdfPar->GetNbChan(iSmType, iRpcId); iCh++) {
      fhCalWalk[iDetIndx][iCh].resize(2);
      fhCalDtWalk[iDetIndx][iCh].resize(2);
      fhCalTotYWalk[iDetIndx][iCh].resize(2);
      fhCalTotYTOff[iDetIndx][iCh].resize(2);
      for (Int_t iSide = 0; iSide < 2; iSide++) {
        fhCalWalk[iDetIndx][iCh][iSide] =
          new TH2D(Form("cal_SmT%01d_sm%03d_rpc%03d_Ch%03d_S%01d_Walk", iSmType, iSmId, iRpcId, iCh, iSide),
                   Form("Walk in SmT%01d_sm%03d_rpc%03d_Ch%03d_S%01d_Walk; Tot [a.u.];  #DeltaT [ns]", iSmType, iSmId,
                        iRpcId, iCh, iSide),
                   nbClWalkBinX, 0., TotMax, nbClWalkBinY, -TSumMax, TSumMax);
        fhCalDtWalk[iDetIndx][iCh][iSide] =
          new TH2D(Form("cal_SmT%01d_sm%03d_rpc%03d_Ch%03d_S%01d_DtWalk", iSmType, iSmId, iRpcId, iCh, iSide),
                   Form("SmT%01d_sm%03d_rpc%03d_Ch%03d_S%01d_DtWalk; Tot [a.u.];  #DeltaT [ns]", iSmType, iSmId, iRpcId,
                        iCh, iSide),
                   nbClWalkBinX, 0., TotMax, nbClWalkBinY, -TSumMax, TSumMax);

        fhCalTotYWalk[iDetIndx][iCh][iSide] =
          new TH3D(Form("calTotY_SmT%01d_sm%03d_rpc%03d_Ch%03d_S%01d_Walk", iSmType, iSmId, iRpcId, iCh, iSide),
                   Form("YWalk in SmT%01d_sm%03d_rpc%03d_Ch%03d_S%01d_Walk; Tot [a.u.]; y [cm];  #DeltaT [ns]", iSmType,
                        iSmId, iRpcId, iCh, iSide),
                   nbClWalkBinX, 0., TotMax, 20, -YMAX, YMAX, nbClWalkBinY, -0.4, 0.4);

        fhCalTotYTOff[iDetIndx][iCh][iSide] =
          new TH3D(Form("calTotY_SmT%01d_sm%03d_rpc%03d_Ch%03d_S%01d_TOff", iSmType, iSmId, iRpcId, iCh, iSide),
                   Form("YTot in SmT%01d_sm%03d_rpc%03d_Ch%03d_S%01d_TOff; Tot [a.u.]; y [cm];  #DeltaT [ns]", iSmType,
                        iSmId, iRpcId, iCh, iSide),
                   nbClWalkBinX, 0., TotMax, 20, -YMAX, YMAX, nbClWalkBinY, -TSumMax * 0.5, TSumMax * 0.5);
      }
    }
  }

  return kTRUE;
}

static Int_t NevtH = 0;
void CbmTofCalibrator::FillCalHist(CbmTofHit* pRef, Int_t iOpt, CbmEvent* tEvent)
{
  if (NULL == fTofCalDigiVec) LOG(fatal) << "No access to TofCalDigis! for Event " << tEvent;

  NevtH++;
  if (NevtH == 1) LOG(info) << "FillCalHist: look for beam counter at " << pRef;

  double dTAv = 0.;
  if (pRef != NULL)
    dTAv = pRef->GetTime();
  else {
    if (-1 < fTofClusterizer->GetBeamAddr()) {  // look for beam counter
      if (NevtH == 1) LOG(info) << Form("FillCalHist: look for beam counter 0x%08x", fTofClusterizer->GetBeamAddr());
      Int_t iHit = 0;
      for (; iHit < (int) fTofClusterizer->GetNbHits(); iHit++) {
        CbmTofHit* pHit = fTofClusterizer->GetHitPointer(iHit);
        if (NULL == pHit) continue;
        if (pHit->GetAddress() == fTofClusterizer->GetBeamAddr()) {
          pRef = pHit;
          dTAv = pRef->GetTime();
          if (NevtH == 1) LOG(info) << "FillCalHist: found beam counter";
          break;
        }
      }
      if (iHit == fTofClusterizer->GetNbHits()) return;  // beam counter not present
    }
    else {
      double dNAv = 0.;
      for (Int_t iHit = 0; iHit < (int) fTofClusterizer->GetNbHits(); iHit++) {
        LOG(debug) << "GetHitPointer for " << iHit;
        CbmTofHit* pHit = fTofClusterizer->GetHitPointer(iHit);
        if (NULL == pHit) continue;
        dTAv += pHit->GetTime();
        dNAv++;
      }
      if (dNAv == 0) return;
      dTAv /= dNAv;
    }
  }

  for (Int_t iHit = 0; iHit < (int) fTofClusterizer->GetNbHits(); iHit++) {
    CbmTofHit* pHit = fTofClusterizer->GetHitPointer(iHit);
    if (NULL == pHit) continue;

    Int_t iDetId  = (pHit->GetAddress() & DetMask);
    Int_t iSmType = CbmTofAddress::GetSmType(iDetId);
    Int_t iSm     = CbmTofAddress::GetSmId(iDetId);
    Int_t iRpc    = CbmTofAddress::GetRpcId(iDetId);

    std::map<UInt_t, UInt_t>::iterator it = fDetIdIndexMap.find(iDetId);
    if (it == fDetIdIndexMap.end()) {
      LOG(info) << "detector TSR " << iSmType << iSm << iRpc << " not found in detector map";
      continue;  // continue for invalid detector index
    }
    Int_t iDetIndx = it->second;  //fDetIdIndexMap[iDetId];

    Int_t iChId = pHit->GetAddress();
    Int_t iCh   = CbmTofAddress::GetChannelId(iChId);

    // fill CalDigi Tots
    CbmMatch* digiMatch = fTofClusterizer->GetMatchPointer(iHit);
    for (Int_t iLink = 0; iLink < digiMatch->GetNofLinks(); iLink += 2) {  // loop over digis
      CbmLink L0      = digiMatch->GetLink(iLink);                         //vDigish.at(ivDigInd);
      UInt_t iDigInd0 = L0.GetIndex();
      UInt_t iDigInd1 = (digiMatch->GetLink(iLink + 1)).GetIndex();  //vDigish.at(ivDigInd+1);
      //LOG(debug1)<<" " << iDigInd0<<", "<<iDigInd1;
      if (iDigInd0 < fTofCalDigiVec->size() && iDigInd1 < fTofCalDigiVec->size()) {
        const CbmTofDigi* pDig0 = &(fTofCalDigiVec->at(iDigInd0));
        const CbmTofDigi* pDig1 = &(fTofCalDigiVec->at(iDigInd1));
        if ((Int_t) pDig0->GetType() != iSmType) {
          LOG(error) << Form(" Wrong Digi SmType %d - %d for Tofhit %d in iDetIndx "
                             "%d, Ch %d with %d strips at Indx %d, %d",
                             iSmType, (Int_t) pDig0->GetType(), iHit, iDetIndx, iCh,
                             fDigiBdfPar->GetNbChan(iSmType, iRpc), iDigInd0, iDigInd1);

          for (Int_t iL = 0; iL < digiMatch->GetNofLinks(); iL++) {  // loop over digis
            int idx               = (digiMatch->GetLink(iL)).GetIndex();
            const CbmTofDigi* pDx = &(fTofCalDigiVec->at(idx));
            LOG(info) << Form(" Digi %d, idx %d, TSRCS %d%d%d%02d%d ", iL, idx, (int) pDx->GetType(),
                              (int) pDx->GetSm(), (int) pDx->GetRpc(), (int) pDx->GetChannel(), (int) pDx->GetSide());
          }

          for (Int_t idx = 0; idx < (int) fTofCalDigiVec->size(); idx++) {  // loop over digis
            const CbmTofDigi* pDx = &(fTofCalDigiVec->at(idx));
            LOG(info) << Form(" AllDigi %d, TSRCS %d%d%d%02d%d ", idx, (int) pDx->GetType(), (int) pDx->GetSm(),
                              (int) pDx->GetRpc(), (int) pDx->GetChannel(), (int) pDx->GetSide());
          }

          LOG(fatal) << " fhRpcCluTot:  Digi 0 " << iDigInd0 << ": Ch " << pDig0->GetChannel() << ", Side "
                     << pDig0->GetSide() << ", StripSide " << (Double_t) iCh * 2. + pDig0->GetSide() << " Digi 1 "
                     << iDigInd1 << ": Ch " << pDig1->GetChannel() << ", Side " << pDig1->GetSide() << ", StripSide "
                     << (Double_t) iCh * 2. + pDig1->GetSide() << ", Tot0 " << pDig0->GetTot() << ", Tot1 "
                     << pDig1->GetTot();
        }  // end of type mismatch condition
        fhCalTot[iDetIndx]->Fill(pDig0->GetChannel() * 2. + pDig0->GetSide(), pDig0->GetTot());
        fhCalTot[iDetIndx]->Fill(pDig1->GetChannel() * 2. + pDig1->GetSide(), pDig1->GetTot());
      }
      else {
        LOG(warn) << "Invalid CalDigi index " << iDigInd0 << ",  " << iDigInd1 << " for size "
                  << fTofCalDigiVec->size();
      }
    }

    CbmTofCell* fChannelInfo = fDigiPar->GetCell(iChId);
    if (NULL == fChannelInfo) {
      LOG(error) << "Invalid Channel Pointer for ChId " << Form(" 0x%08x ", iChId) << ", Ch " << iCh;
      continue;
    }
    //gGeoManager->FindNode(fChannelInfo->GetX(), fChannelInfo->GetY(), fChannelInfo->GetZ());
    Double_t hitpos[3];
    hitpos[0] = pHit->GetX();
    hitpos[1] = pHit->GetY();
    hitpos[2] = pHit->GetZ();
    Double_t hlocal_p[3];
    //TGeoNode* cNode=
    //gGeoManager->GetCurrentNode();
    //gGeoManager->MasterToLocal(hitpos, hlocal_p);
    fTofClusterizer->MasterToLocal(iChId, hitpos, hlocal_p);

    /*
    LOG(info)<<Form(" Ev %d: TSRC %d%d%d%2d, y %6.2f, tof %6.2f, Tref %12.1f ",
    		   NevtH,iSmType,iSm,iRpc,iCh,hlocal_p[1],pHit->GetTime() - dTAv, pRef->GetTime());
    */
    fhCalPosition[iDetIndx]->Fill((Double_t) iCh, hlocal_p[1]);  // transformed into LRF
    /*
    if(pHit != pRef )
      fhCalTofOff[iDetIndx]->Fill((Double_t) iCh,
                              pHit->GetTime() - pRef->GetTime());  // ignore any dependence
                              */
    if (iOpt < 100) {
      fhCalTOff[iDetIndx]->Fill((Double_t) iCh, pHit->GetTime() - dTAv);
      fhCalXYTOff[iDetIndx]->Fill((Double_t) iCh, hlocal_p[1], pHit->GetTime() - dTAv);
      fhCalTofOff[iDetIndx]->Fill((Double_t) iCh, pHit->GetTime() - dTAv);
    }
    else {
      double dTexp = pHit->GetR() * fTofFindTracks->GetTtTarg();
      fhCalTOff[iDetIndx]->Fill((Double_t) iCh, pHit->GetTime() - dTAv);
      fhCalXYTOff[iDetIndx]->Fill((Double_t) iCh, hlocal_p[1], pHit->GetTime() - dTAv);
      fhCalTofOff[iDetIndx]->Fill((Double_t) iCh, pHit->GetTime() - dTAv - dTexp);
    }
  }

  return;
}

void CbmTofCalibrator::FillHitCalHist(CbmTofHit* pRef, const Int_t iOpt, CbmEvent* tEvent, TClonesArray* tTofHitsColl)
{
  if (NULL == fTofCalDigiVec) LOG(fatal) << "No access to TofCalDigis! for Event " << tEvent;
  if (NULL == tTofHitsColl) LOG(fatal) << "No access to TofHits! for Event " << tEvent;

  NevtH++;
  if (NevtH == 1)
    LOG(warn) << "HitCalHist at " << NevtH << ", Opt = " << iOpt << ", pRef " << pRef << ", "
              << Form("0x%08x", fTofClusterizer->GetBeamAddr());

  double dTAv = 0.;
  if (pRef != NULL) {
    dTAv = pRef->GetTime();
    //LOG(debug) << "HitCalHist got Tbeam " << dTAv << ", Opt = " << iOpt;
  }
  else {
    if (-1 < fTofClusterizer->GetBeamAddr()) {  // look for beam counter
      if (NevtH == 1) LOG(warn) << Form("FillHitCalHist: look for beam counter 0x%08x", fTofClusterizer->GetBeamAddr());
      size_t iHit = 0;
      //for (; iHit < (int) fTofFindTracks->GetNbHits(); iHit++) {
      for (; iHit < tEvent->GetNofData(ECbmDataType::kTofHit); iHit++) {
        //CbmTofHit* pHit = fTofFindTracks->GetHitPointer(iHit);
        Int_t iHitIndex = static_cast<Int_t>(tEvent->GetIndex(ECbmDataType::kTofHit, iHit));
        CbmTofHit* pHit = (CbmTofHit*) tTofHitsColl->At(iHitIndex);
        if (NULL == pHit) continue;
        if ((pHit->GetAddress() & DetMask) == (fTofClusterizer->GetBeamAddr() & DetMask)) {
          if (pRef != NULL) {
            if (pHit->GetTime() < dTAv) pRef = pHit;
          }
          else {
            pRef = pHit;
          }
          dTAv = pRef->GetTime();
          if (NevtH == 1) LOG(warn) << "FillHitCalHist: found beam counter";
          break;
        }
      }
      if (iHit == tEvent->GetNofData(ECbmDataType::kTofHit)) return;  // beam counter not present
    }
    else {
      double dNAv = 0.;
      for (size_t iHit = 0; iHit < tEvent->GetNofData(ECbmDataType::kTofHit); iHit++) {
        Int_t iHitIndex = static_cast<Int_t>(tEvent->GetIndex(ECbmDataType::kTofHit, iHit));
        CbmTofHit* pHit = (CbmTofHit*) tTofHitsColl->At(iHitIndex);
        if (NULL == pHit) continue;
        dTAv += pHit->GetTime();
        dNAv++;
      }
      if (dNAv == 0) return;
      dTAv /= dNAv;
    }
  }

  for (size_t iHit = 0; iHit < tEvent->GetNofData(ECbmDataType::kTofHit); iHit++) {
    Int_t iHitIndex = static_cast<Int_t>(tEvent->GetIndex(ECbmDataType::kTofHit, iHit));
    CbmTofHit* pHit = (CbmTofHit*) tTofHitsColl->At(iHitIndex);
    if (NULL == pHit) continue;

    Int_t iDetId  = (pHit->GetAddress() & DetMask);
    Int_t iSmType = CbmTofAddress::GetSmType(iDetId);
    Int_t iSm     = CbmTofAddress::GetSmId(iDetId);
    Int_t iRpc    = CbmTofAddress::GetRpcId(iDetId);

    std::map<UInt_t, UInt_t>::iterator it = fDetIdIndexMap.find(iDetId);
    if (it == fDetIdIndexMap.end()) {
      LOG(info) << "detector TSR " << iSmType << iSm << iRpc << " not found in detector map";
      continue;  // continue for invalid detector index
    }
    Int_t iDetIndx = it->second;  //fDetIdIndexMap[iDetId];

    Int_t iChId  = pHit->GetAddress();
    Int_t iCh    = CbmTofAddress::GetChannelId(iChId);
    double dTexp = 0.;

    /*
    CbmTofCell* fChannelInfo = fDigiPar->GetCell(iChId);
    if (NULL == fChannelInfo) {
      LOG(error) << "Invalid Channel Pointer for ChId " << Form(" 0x%08x ", iChId) << ", Ch " << iCh;
      continue;
    }
    */

    Double_t hitpos[3];
    hitpos[0] = pHit->GetX();
    hitpos[1] = pHit->GetY();
    hitpos[2] = pHit->GetZ();
    Double_t hlocal_p[3];

    fTofClusterizer->MasterToLocal(iChId, hitpos, hlocal_p);

    LOG(debug) << Form(" Ev %d: TSRC %d%d%d%2d, y %6.2f, tof %6.2f, Tref %12.1f ", NevtH, iSmType, iSm, iRpc, iCh,
                       hlocal_p[1], pHit->GetTime() - dTAv, pRef->GetTime());

    fhCalPosition[iDetIndx]->Fill((Double_t) iCh, hlocal_p[1]);  // transformed into LRF

    if (iOpt < 100) {
      LOG(debug) << "Fill " << fhCalTofOff[iDetIndx]->GetName();
      if (pHit != pRef) {
        fhCalTOff[iDetIndx]->Fill((Double_t) iCh, pHit->GetTime() - dTAv);
        fhCalXYTOff[iDetIndx]->Fill((Double_t) iCh, hlocal_p[1], pHit->GetTime() - dTAv);
        fhCalTofOff[iDetIndx]->Fill((Double_t) iCh, pHit->GetTime() - dTAv);
      }
    }
    else {
      if (iSmType != 5) {
        if (NULL != pRef) {
          if ((pHit->GetAddress() & DetMask) != (pRef->GetAddress() & DetMask)) {
            dTexp = pHit->Dist3D(pRef) / cLight;  // Edge expected for speed of light,
            if (pHit->GetZ() < pRef->GetZ()) dTexp *= -1;

            LOG(debug) << Form(" Ev %d: TSRC %d%d%d%2d, y %6.2f, tof %6.2f, Tref %12.1f, Texp %12.1f ", NevtH, iSmType,
                               iSm, iRpc, iCh, hlocal_p[1], pHit->GetTime() - dTAv, pRef->GetTime(), dTexp);
          }
        }
        else {
          dTexp = pHit->GetR() / cLight;  // Edge expected for speed of light, fTofFindTracks->GetTtTarg();
        }
        /*
          LOG(debug)<<"Fill " << fhCalTofOff[iDetIndx]->GetName()<<" " << iHit <<": " << iCh << ", "
                  << pHit->GetTime() - dTAv <<" - " << dTexp;
        */
        if (dTexp != 0.) {
          fhCalTOff[iDetIndx]->Fill((Double_t) iCh, pHit->GetTime() - dTAv - dTexp);
          fhCalXYTOff[iDetIndx]->Fill((Double_t) iCh, hlocal_p[1], pHit->GetTime() - dTAv - dTexp);
          fhCalTofOff[iDetIndx]->Fill((Double_t) iCh, pHit->GetTime() - dTAv - dTexp);
        }
      }
    }

    // fill CalDigi Tots and internal walks
    if (dTexp != 0.) {
      CbmMatch* digiMatch = fTofClusterizer->GetMatchIndexPointer(iHitIndex);
      if (NULL == digiMatch) LOG(fatal) << "No digiMatch for Hit " << iHit << ", TSR " << iSmType << iSm << iRpc;
      if (digiMatch->GetNofLinks() < fTofClusterizer->GetCluSizeMin()) continue;
      double dMeanTimeSquared = 0.;
      double dTotSum          = 0.;
      double dYSum            = 0.;
      for (Int_t iLink = 0; iLink < digiMatch->GetNofLinks(); iLink += 2) {  // first loop over digis
        CbmLink L0      = digiMatch->GetLink(iLink);
        UInt_t iDigInd0 = L0.GetIndex();
        UInt_t iDigInd1 = (digiMatch->GetLink(iLink + 1)).GetIndex();
        //LOG(debug1)<<" " << iDigInd0<<", "<<iDigInd1;
        if (iDigInd0 < fTofCalDigiVec->size() && iDigInd1 < fTofCalDigiVec->size()) {
          const CbmTofDigi* pDig0 = &(fTofCalDigiVec->at(iDigInd0));
          const CbmTofDigi* pDig1 = &(fTofCalDigiVec->at(iDigInd1));
          if ((Int_t) pDig0->GetType() != iSmType) {
            LOG(error) << Form(" Wrong Digi SmType %d - %d for Tofhit %lu in iDetIndx "
                               "%d, Ch %d with %d strips at Indx %d, %d",
                               iSmType, (Int_t) pDig0->GetType(), iHit, iDetIndx, iCh,
                               fDigiBdfPar->GetNbChan(iSmType, iRpc), iDigInd0, iDigInd1);

            for (Int_t iL = 0; iL < digiMatch->GetNofLinks(); iL++) {  // loop over digis
              int idx               = (digiMatch->GetLink(iL)).GetIndex();
              const CbmTofDigi* pDx = &(fTofCalDigiVec->at(idx));
              LOG(info) << Form(" Digi %d, idx %d, TSRCS %d%d%d%02d%d ", iL, idx, (int) pDx->GetType(),
                                (int) pDx->GetSm(), (int) pDx->GetRpc(), (int) pDx->GetChannel(), (int) pDx->GetSide());
            }

            for (Int_t idx = 0; idx < (int) fTofCalDigiVec->size(); idx++) {  // loop over digis
              const CbmTofDigi* pDx = &(fTofCalDigiVec->at(idx));
              LOG(info) << Form(" AllDigi %d, TSRCS %d%d%d%02d%d ", idx, (int) pDx->GetType(), (int) pDx->GetSm(),
                                (int) pDx->GetRpc(), (int) pDx->GetChannel(), (int) pDx->GetSide());
            }

            LOG(fatal) << " fhRpcCluTot:  Digi 0 " << iDigInd0 << ": Ch " << pDig0->GetChannel() << ", Side "
                       << pDig0->GetSide() << ", StripSide " << (Double_t) iCh * 2. + pDig0->GetSide() << " Digi 1 "
                       << iDigInd1 << ": Ch " << pDig1->GetChannel() << ", Side " << pDig1->GetSide() << ", StripSide "
                       << (Double_t) iCh * 2. + pDig1->GetSide() << ", Tot0 " << pDig0->GetTot() << ", Tot1 "
                       << pDig1->GetTot();
          }  // end of type mismatch condition
          fhCalTot[iDetIndx]->Fill(pDig0->GetChannel() * 2. + pDig0->GetSide(), pDig0->GetTot());
          fhCalTot[iDetIndx]->Fill(pDig1->GetChannel() * 2. + pDig1->GetSide(), pDig1->GetTot());
          if (digiMatch->GetNofLinks() > 2) {
            double dTotStrip = pDig0->GetTot() + pDig1->GetTot();
            dTotSum += dTotStrip;
            double dYdigi = 0.5 * (pDig0->GetTime() - pDig1->GetTime()) * fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc);
            if (pDig0->GetSide() == 1) {
              dYSum += (pDig0->GetTime() - pDig1->GetTime()) * dTotStrip;  // weighted mean
            }
            else {
              dYSum += (pDig1->GetTime() - pDig0->GetTime()) * dTotStrip;
              dYdigi *= -1;
            }
            fhCalXYTot[iDetIndx]->Fill(pDig0->GetChannel() * 2. + pDig0->GetSide(), dYdigi, pDig0->GetTot());
            fhCalXYTot[iDetIndx]->Fill(pDig1->GetChannel() * 2. + pDig1->GetSide(), dYdigi, pDig1->GetTot());
          }
        }
      }  // first loop over digis
      if (digiMatch->GetNofLinks() > 2) {
        double dYAv  = dYSum / dTotSum;
        double dYLoc = 0.;
        for (Int_t iLink = 0; iLink < digiMatch->GetNofLinks(); iLink += 2) {  // second loop over digis
          CbmLink L0      = digiMatch->GetLink(iLink);
          UInt_t iDigInd0 = L0.GetIndex();
          UInt_t iDigInd1 = (digiMatch->GetLink(iLink + 1)).GetIndex();
          if (iDigInd0 < fTofCalDigiVec->size() && iDigInd1 < fTofCalDigiVec->size()) {
            const CbmTofDigi* pDig0 = &(fTofCalDigiVec->at(iDigInd0));
            const CbmTofDigi* pDig1 = &(fTofCalDigiVec->at(iDigInd1));
            double dTotStrip        = pDig0->GetTot() + pDig1->GetTot();
            double dTotWeight       = 1 - dTotStrip / dTotSum;
            //dTotWeight    *= 0.5;
            dTotWeight     = 1.;
            double dDeltaT = 0.5 * (pDig0->GetTime() + pDig1->GetTime()) - pHit->GetTime();
            //if(NULL != fTofFindTracks) dDeltaT += fTofFindTracks->GetTOff(iDetId); //obsolete ??
            fhCalDelTOff[iDetIndx]->Fill(pDig0->GetChannel(), dDeltaT * dTotWeight);
            //  TBD: position deviation in counter reference frame
            Double_t dDelPos = 0.5 * (pDig0->GetTime() - pDig1->GetTime()) * fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc);
            if (0 == pDig0->GetSide()) dDelPos *= -1.;
            fhCalDelPos[iDetIndx]->Fill(pDig0->GetChannel(), (dDelPos - fTofClusterizer->GetLocalY(pHit)));

            int iCh0   = pDig0->GetChannel();
            int iSide0 = pDig0->GetSide();
            int iCh1   = pDig1->GetChannel();
            int iSide1 = pDig1->GetSide();
            if (iCh0 != iCh1) LOG(fatal) << "Inconsistent TofHit - FixIt";
            fhCalWalk[iDetIndx][iCh0][iSide0]->Fill(pDig0->GetTot(), dDeltaT * dTotWeight);
            fhCalWalk[iDetIndx][iCh1][iSide1]->Fill(pDig1->GetTot(), dDeltaT * dTotWeight);
            dMeanTimeSquared += dDeltaT * dDeltaT;
            if (pDig0->GetSide() == 1) {
              dYLoc = pDig0->GetTime() - pDig1->GetTime();
            }
            else {
              dYLoc = pDig1->GetTime() - pDig0->GetTime();
            }
            dYLoc *= 0.5;
            fhCalTotYWalk[iDetIndx][iCh0][iSide0]->Fill(pDig0->GetTot(),
                                                        dYLoc * fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc), dDeltaT);
            fhCalTotYWalk[iDetIndx][iCh1][iSide1]->Fill(pDig1->GetTot(),
                                                        dYLoc * fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc), dDeltaT);

            fhCalTotYTOff[iDetIndx][iCh0][iSide0]->Fill(
              pDig0->GetTot(), dYLoc * fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc), pHit->GetTime() - dTAv - dTexp);
            fhCalTotYTOff[iDetIndx][iCh1][iSide1]->Fill(
              pDig1->GetTot(), dYLoc * fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc), pHit->GetTime() - dTAv - dTexp);

            Double_t dDeltaDT = dYLoc - dYAv;
            fhCalPos[iDetIndx]->Fill(iCh0, dDeltaDT * fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc));

            fhCalDtWalk[iDetIndx][iCh0][iSide0]->Fill(pDig0->GetTot(), (2. * pDig0->GetSide() - 1) * dDeltaDT);
            fhCalDtWalk[iDetIndx][iCh1][iSide1]->Fill(pDig1->GetTot(), (2. * pDig1->GetSide() - 1) * dDeltaDT);
          }
        }  // second loop over digis
      }    // if (digiMatch->GetNofLinks()>2)

      fhCalCluSize[iDetIndx]->Fill((Double_t) iCh, digiMatch->GetNofLinks() / 2);
      if (digiMatch->GetNofLinks() > 2) {
        double dVar  = dMeanTimeSquared / (digiMatch->GetNofLinks() / 2 - 1);
        double dTrms = TMath::Sqrt(dVar);
        fhCalCluTrms[iDetIndx]->Fill((Double_t) iCh, dTrms);
      }
    }  // iOpt<100
  }
  return;
}  // -- FillHitCalHist


static Int_t NevtT = 0;
void CbmTofCalibrator::FillCalHist(CbmTofTracklet* pTrk, Int_t iOpt, CbmEvent* tEvent)
{
  NevtT++;
  if (NULL == fTofCalDigiVec) LOG(fatal) << "No access to TofCalDigis! Opt " << iOpt << ", Event" << tEvent;
  // fill deviation histograms on walk level
  /*
  LOG(info)<<"Calibrator " << NevtT <<": Tt " << pTrk->GetTt() 
           <<", R0: " << pTrk->GetR0() <<", bB " 
           <<  pTrk->ContainsAddr(CbmTofAddress::GetUniqueAddress(0, 0, 0, 0, 5));
  */
  if (pTrk->GetTt() < 0) return;  // take tracks with positive velocity only
  if (pTrk->GetNofHits() < fTofFindTracks->GetNReqStations()) return;

  fhCalDX0->Fill(pTrk->GetFitX(0.));
  fhCalDY0->Fill(pTrk->GetFitY(0.));

  if (fbBeam && !pTrk->ContainsAddr(CbmTofAddress::GetUniqueAddress(0, 0, 0, 0, 5)))
    return;  // request beam counter hit for calibration

  if (fbBeam && fdR0Lim > 0.)  // consider only tracks originating from nominal interaction point
  {
    fhCalR0->Fill(pTrk->GetR0());
    if (pTrk->GetR0() > fdR0Lim) return;
  }

  //if (iOpt > 70) HstDoublets(pTrk);  // Fill Doublets histograms

  //double dEvtStart=fEvtHeader->GetEventTime();
  for (Int_t iHit = 0; iHit < pTrk->GetNofHits(); iHit++) {
    CbmTofHit* pHit = pTrk->GetTofHitPointer(iHit);
    Int_t iDetId    = (pHit->GetAddress() & DetMask);
    Int_t iSmType   = CbmTofAddress::GetSmType(iDetId);
    Int_t iSm       = CbmTofAddress::GetSmId(iDetId);
    Int_t iRpc      = CbmTofAddress::GetRpcId(iDetId);

    std::map<UInt_t, UInt_t>::iterator it = fDetIdIndexMap.find(iDetId);
    if (it == fDetIdIndexMap.end()) continue;  // continue for invalid detector index
    Int_t iDetIndx = it->second;               //fDetIdIndexMap[iDetId];

    Int_t iChId              = pHit->GetAddress();
    Int_t iCh                = CbmTofAddress::GetChannelId(iChId);
    CbmTofCell* fChannelInfo = fDigiPar->GetCell(iChId);
    if (NULL == fChannelInfo) {
      LOG(error) << "Invalid Channel Pointer for ChId " << Form(" 0x%08x ", iChId) << ", Ch " << iCh;
      continue;
    }
    //gGeoManager->FindNode(fChannelInfo->GetX(), fChannelInfo->GetY(), fChannelInfo->GetZ());
    Double_t hitpos[3];
    hitpos[0] = pHit->GetX();
    hitpos[1] = pHit->GetY();
    hitpos[2] = pHit->GetZ();
    Double_t hlocal_p[3];
    //TGeoNode* cNode=
    //gGeoManager->GetCurrentNode();
    //gGeoManager->MasterToLocal(hitpos, hlocal_p);
    fTofClusterizer->MasterToLocal(iChId, hitpos, hlocal_p);

    hitpos[0] = pTrk->GetFitX(pHit->GetZ());
    hitpos[1] = pTrk->GetFitY(pHit->GetZ());
    Double_t hlocal_f[3];
    //gGeoManager->MasterToLocal(hitpos, hlocal_f);
    fTofClusterizer->MasterToLocal(iChId, hitpos, hlocal_f);
    if (true) {  //0==fDigiBdfPar->GetChanOrient(iSmType, iRpc)) { // default
      fhCalPosition[iDetIndx]->Fill((Double_t) iCh, hlocal_p[1]);           // transformed into LRF
      fhCalPos[iDetIndx]->Fill((Double_t) iCh, hlocal_p[1] - hlocal_f[1]);  // transformed into LRF
    }
    else {  // Counters rotated with respect to module axis, strips along x-axis
      fhCalPosition[iDetIndx]->Fill((Double_t) iCh, hlocal_p[0]);           // transformed into LRF
      fhCalPos[iDetIndx]->Fill((Double_t) iCh, hlocal_p[0] - hlocal_f[0]);  // transformed into LRF
    }
    //if(fTofFindTracks->GetTtTarg()==0.033) pTrk->SetTt(fTofFindTracks->GetTtTarg()); //for calibration mode 1,2,31
    //double dTexp = pTrk->GetFitT(pHit->GetZ());  // residuals transformed into LRF
    //double dTexp = fTrackletTools->GetTexpected(pTrk, pHit->GetAddress()&DetMask, pHit, fTofFindTracks->GetTtLight());
    double dTexp = fTrackletTools->GetTexpected(pTrk, -1, pHit, fTofFindTracks->GetTtLight());
    fhCalTOff[iDetIndx]->Fill((Double_t) iCh, pHit->GetTime() - dTexp);
    fhCalXYTOff[iDetIndx]->Fill((Double_t) iCh, hlocal_p[1],
                                pHit->GetTime() - dTexp);  // residuals transformed into LRF
    //fhCalTOff[iDetIndx]->Fill((Double_t)iCh,fTrackletTools->GetTdif(pTrk, iDetId, pHit));

    Int_t iEA  = pTrk->GetTofHitIndex(iHit);
    Int_t iTSA = fTofFindTracks->GetTofHitIndex(iEA);

    if (iTSA > fTofDigiMatchColl->GetEntriesFast()) {
      LOG(error) << " Inconsistent DigiMatches for Hitind " << iTSA
                 << ", TClonesArraySize: " << fTofDigiMatchColl->GetEntriesFast();
    }
    CbmMatch* digiMatch = (CbmMatch*) fTofDigiMatchColl->At(iTSA);

    Double_t hlocal_d[3];
    for (Int_t iLink = 0; iLink < digiMatch->GetNofLinks(); iLink += 2) {  // loop over digis
      CbmLink L0     = digiMatch->GetLink(iLink);
      Int_t iDigInd0 = L0.GetIndex();
      Int_t iDigInd1 = (digiMatch->GetLink(iLink + 1)).GetIndex();

      const CbmTofDigi* tDigi0 = NULL;
      const CbmTofDigi* tDigi1 = NULL;
      if (tEvent != NULL) {  //disable
        LOG(debug) << "Locate MatchDigiInd " << iDigInd0 << " and " << iDigInd1 << " in CalDigiVec of size "
                   << fTofCalDigiVec->size();
        //		  <<" in current event not implemented";
        //continue;
        tDigi0 = &(fTofCalDigiVec->at(iDigInd0));
        tDigi1 = &(fTofCalDigiVec->at(iDigInd1));
      }
      else {  // event wise entries (TS mode)
        LOG(debug) << "TS mode: locate MatchDigiInd " << iDigInd0 << " and " << iDigInd1 << " in CalDigiVec of size "
                   << fTofCalDigiVec->size();
        if (fair::Logger::Logging(fair::Severity::debug2)) {
          for (UInt_t iDigi = 0; iDigi < fTofCalDigiVec->size(); iDigi++) {
            tDigi0 = &(fTofCalDigiVec->at(iDigi));
            LOG(debug) << Form("#%u: TSRCS %d%d%d%2d%d", iDigi, (Int_t) tDigi0->GetType(), (Int_t) tDigi0->GetSm(),
                               (Int_t) tDigi0->GetRpc(), (Int_t) tDigi0->GetChannel(), (Int_t) tDigi0->GetSide());
          }
        }
        tDigi0 = &(fTofCalDigiVec->at(iDigInd0));
        tDigi1 = &(fTofCalDigiVec->at(iDigInd1));
        //tDigi0 = fDigiMan->Get<CbmTofDigi>(iDigInd0);
        //tDigi1 = fDigiMan->Get<CbmTofDigi>(iDigInd1);
      }

      Int_t iCh0   = tDigi0->GetChannel();
      Int_t iSide0 = tDigi0->GetSide();

      LOG(debug) << "Fill Walk for Hit Ind " << iEA << ", " << iTSA
                 << Form(", TSRC %d%d%d%2d, DigiInd %2d, %2d", iSmType, iSm, iRpc, iCh, iDigInd0, iDigInd1)
                 << Form(", TSRCS %d%d%d%2d%d %d%d%d%2d%d", (Int_t) tDigi0->GetType(), (Int_t) tDigi0->GetSm(),
                         (Int_t) tDigi0->GetRpc(), (Int_t) tDigi0->GetChannel(), (Int_t) tDigi0->GetSide(),
                         (Int_t) tDigi1->GetType(), (Int_t) tDigi1->GetSm(), (Int_t) tDigi1->GetRpc(),
                         (Int_t) tDigi1->GetChannel(), (Int_t) tDigi1->GetSide());

      if (iDetIndx > (Int_t) fhCalWalk.size()) {
        LOG(error) << "Invalid DetIndx " << iDetIndx;
        continue;
      }
      if (iCh0 > (Int_t) fhCalWalk[iDetIndx].size()) {
        LOG(error) << "Invalid Ch0 " << iCh0 << " for detIndx" << iDetIndx << ", size " << fhCalWalk[iDetIndx].size();
        continue;
      }
      if (iSide0 > (Int_t) fhCalWalk[iDetIndx][iCh0].size()) {
        LOG(error) << "Invalid Side0 " << iSide0 << " for "
                   << " for detIndx" << iDetIndx << ", size " << fhCalWalk[iDetIndx][iCh0].size();
        continue;
      }

      Int_t iCh1   = tDigi1->GetChannel();
      Int_t iSide1 = tDigi1->GetSide();
      if (iCh1 > (Int_t) fhCalWalk[iDetIndx].size()) {
        LOG(error) << "Invalid Ch1 " << iCh1 << " for "
                   << " for detIndx" << iDetIndx;
        continue;
      }
      if (iSide1 > (Int_t) fhCalWalk[iDetIndx][iCh1].size()) {
        LOG(error) << "Invalid Side1 " << iSide1 << " for "
                   << " for detIndx" << iDetIndx;
        continue;
      }

      if (iCh0 != iCh1 || iSide0 == iSide1) {
        LOG(fatal) << "Invalid digi pair for TSR " << iSmType << iSm << iRpc
                   << Form(" Ch  %2d %2d side %d %d", iCh0, iCh1, iSide0, iSide1) << " in event " << NevtT;
        continue;
      }

      hlocal_d[1] =
        -0.5 * ((1. - 2. * tDigi0->GetSide()) * tDigi0->GetTime() + (1. - 2. * tDigi1->GetSide()) * tDigi1->GetTime())
        * fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc);

      if (TMath::Abs(hlocal_d[1] - hlocal_p[1]) > 10.) {
        LOG(warn) << "CMPY for TSRC " << iSmType << iSm << iRpc << iCh0 << ": " << hlocal_f[1] << ", " << hlocal_p[1]
                  << ", " << hlocal_d[1] << ", TOT: " << tDigi0->GetTot() << " " << tDigi1->GetTot();
      }
      Int_t iWalkMode = 0;  //(iOpt - iOpt % 100) / 100;
      switch (iWalkMode) {
        case 1:
          fhCalWalk[iDetIndx][iCh0][iSide0]->Fill(
            tDigi0->GetTot(),
            tDigi0->GetTime() + (1. - 2. * tDigi0->GetSide()) * hlocal_d[1] / fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc)
              - pTrk->GetFitT(pHit->GetZ())  //-fTrackletTools->GetTexpected(pTrk, iDetId, pHit)
              + fTofFindTracks->GetTOff(iDetId)
              + 2. * (1. - 2. * tDigi0->GetSide()) * (hlocal_d[1] - hlocal_f[1])
                  / fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc));
          /*
        LOG(info)<<"TSRCS "<<iSmType<<iSm<<iRpc<<iCh<<iSide0<<Form(": digi0 %f, ex %f, prop %f, Off %f, res %f",
                            tDigi0->GetTime(),
                            fTrackletTools->GetTexpected(pTrk, iDetId, pHit) ,
                            fTofFindTracks->GetTOff(iDetId),
                            (1.-2.*tDigi0->GetSide())*hlocal_f[1]/fDigiBdfPar->GetSigVel(iSmType,iSm,iRpc),
                            tDigi0->GetTime()-fTrackletTools->GetTexpected(pTrk, iDetId, pHit)
                            -(1.-2.*tDigi0->GetSide())*hlocal_f[1]/fDigiBdfPar->GetSigVel(iSmType,iSm,iRpc));
        */

          fhCalWalk[iDetIndx][iCh1][iSide1]->Fill(
            tDigi1->GetTot(),
            tDigi1->GetTime() + (1. - 2. * tDigi1->GetSide()) * hlocal_d[1] / fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc)
              - pTrk->GetFitT(pHit->GetZ())  //-fTrackletTools->GetTexpected(pTrk, iDetId, pHit)
              + fTofFindTracks->GetTOff(iDetId)
              + 2. * (1. - 2. * tDigi1->GetSide()) * (hlocal_d[1] - hlocal_f[1])
                  / fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc));
          break;

        case 0: {
          Double_t dDeltaT =
            0.5 * (tDigi0->GetTime() + tDigi1->GetTime()) + fTofFindTracks->GetTOff(iDetId) - pHit->GetTime();

          fhCalWalkAv[iDetIndx]->Fill(tDigi0->GetTot(), dDeltaT);
          fhCalWalkAv[iDetIndx]->Fill(tDigi1->GetTot(), dDeltaT);
          fhCalWalk[iDetIndx][iCh0][iSide0]->Fill(tDigi0->GetTot(), dDeltaT);
          fhCalWalk[iDetIndx][iCh1][iSide1]->Fill(tDigi1->GetTot(), dDeltaT);
          if (iSmType == 5 || iSmType == 8) {  // symmetrize for Pad counters
            fhCalWalk[iDetIndx][iCh0][iSide0]->Fill(tDigi1->GetTot(), dDeltaT);
            fhCalWalk[iDetIndx][iCh1][iSide1]->Fill(tDigi0->GetTot(), dDeltaT);
          }
          Double_t dDeltaDT = (hlocal_d[1] - hlocal_p[1]) / fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc);
          fhCalDtWalk[iDetIndx][iCh0][iSide0]->Fill(tDigi0->GetTot(), (2. * tDigi0->GetSide() - 1) * dDeltaDT);
          fhCalDtWalk[iDetIndx][iCh1][iSide1]->Fill(tDigi1->GetTot(), (2. * tDigi1->GetSide() - 1) * dDeltaDT);
        } break;
      }
    }
  }
}

Bool_t CbmTofCalibrator::UpdateCalHist(Int_t iOpt)
{
  // get current calibration histos
  TString CalFileName = fTofClusterizer->GetCalParFileName();
  LOG(info) << "CbmTofCalibrator:: update histos from "
            << "file " << CalFileName << " with option " << iOpt;
  int iOpt0 = iOpt % 10;
  int iOpt1 = (iOpt - iOpt0) / 10;

  /// Save old global file and folder pointer to avoid messing with FairRoot
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  TFile* fCalParFile = new TFile(CalFileName, "update");
  if (NULL == fCalParFile) {
    LOG(warn) << "Could not open TofClusterizer calibration file " << CalFileName;
    if (iOpt0 == 9) {  //modify reference file name
      int iCalMode          = CalFileName.Index("tofClust") - 3;
      CalFileName(iCalMode) = '3';
      LOG(info) << "Modified CalFileName = " << CalFileName;
      fCalParFile = new TFile(CalFileName, "update");
      if (NULL == fCalParFile) LOG(fatal) << "Could not open TofClusterizer calibration file " << CalFileName;
    }
  }
  assert(fCalParFile);
  ReadHist(fCalParFile);
  if (kTRUE) {                     // all calibration modes
    const Double_t MINCTS = 100.;  //FIXME, numerical constant in code
    // modify calibration histograms
    // check for beam counter
    Double_t dBeamTOff = 0.;
    for (Int_t iDetIndx = 0; iDetIndx < fDigiBdfPar->GetNbDet(); iDetIndx++) {
      Int_t iUniqueId = fDigiBdfPar->GetDetUId(iDetIndx);
      Int_t iSmType   = CbmTofAddress::GetSmType(iUniqueId);
      if (5 == iSmType) {
        if (fhCalTOff[iDetIndx]->GetEntries() > MINCTS) {
          TH1* hBy = (TH1*) fhCalTOff[iDetIndx]->ProjectionY();
          // Fit gaussian around peak value
          Double_t dFMean    = hBy->GetBinCenter(hBy->GetMaximumBin());
          Double_t dFLim     = 0.5;  // CAUTION, fixed numeric value
          Double_t dBinSize  = hBy->GetBinWidth(1);
          dFLim              = TMath::Max(dFLim, 5. * dBinSize);
          TFitResultPtr fRes = hBy->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
          dBeamTOff          = fRes->Parameter(1);  //overwrite mean
          LOG(info) << "Found beam counter with average TOff = " << dBeamTOff;
        }
        else {
          LOG(info) << "Beam counter has too few entries: " << fhCalTOff[iDetIndx]->GetEntries();
        }
        break;
      }
    }
    if (dBeamTOff == 0.) LOG(warn) << "No beam counter found";

    for (Int_t iDetIndx = 0; iDetIndx < fDigiBdfPar->GetNbDet(); iDetIndx++) {
      Int_t iUniqueId = fDigiBdfPar->GetDetUId(iDetIndx);
      // Int_t iSmAddr   = iUniqueId & DetMask;
      Int_t iSmType = CbmTofAddress::GetSmType(iUniqueId);
      Int_t iSm     = CbmTofAddress::GetSmId(iUniqueId);
      Int_t iRpc    = CbmTofAddress::GetRpcId(iUniqueId);
      if (NULL == fhCorTOff[iDetIndx]) {
        LOG(warn) << "hCorTOff for TSR " << iSmType << iSm << iRpc << " not available";
        continue;
      }

      switch (iOpt0) {
        case 0:  // none
        {
          double* dRes;
          switch (iOpt1) {
            case 10: {
              LOG(info) << "Equalize velocities with const threshold counterwise for " << fhCorTOff[iDetIndx]->GetName()
                        << ", MeanTOff " << fhCorTOff[iDetIndx]->GetMean();
              TString hname2 = Form("cal_SmT%d_sm%03d_rpc%03d_TOff", iSmType, iSm, iRpc);
              //TH2* h2 = (TH2*) gROOT->FindObjectAny(hname2);
              TH2* h2  = fhCalTOff[iDetIndx];
              TH2* h2W = fhCalTofOff[iDetIndx];
              if (NULL == h2) {
                LOG(warn) << "Calibration data not available from " << hname2;
                continue;
              }

              double dResIni[1] = {0.};
              dRes              = &dResIni[0];
              Int_t NStrips     = h2->GetNbinsX();
              TH1* h2Wy         = h2W->ProjectionY(Form("%s_py", h2W->GetName()), 1, NStrips);
              TH1* h2y          = h2->ProjectionY(Form("%s_py", h2->GetName()), 1, NStrips);
              double dEdge      = 0.;
              if (iSmType != 5) {
                dRes  = find_tofedge((const char*) (h2Wy->GetName()), fTofClusterizer->GetEdgeThr(),
                                    fTofClusterizer->GetEdgeLen());
                dEdge = dRes[0];
              }
              else {
                dEdge = h2Wy->GetBinCenter(h2Wy->GetMaximumBin());
              }

              /*
              LOG(info) << h2Wy->GetName() << ": Coarse Max at " << h2Wy->GetMaximumBin()
                        << ", " << dEdge << ", " << h2y->GetBinCenter(1)
                        << ", " << h2y->GetBinCenter(h2y->GetNbinsX());
              */
              if (dEdge < h2y->GetBinCenter(1) || dEdge > h2y->GetBinCenter(h2y->GetNbinsX())) {
                dRes[0] = dEdge;  // - fTofClusterizer->GetEdgeTbias();
                LOG(info) << h2Wy->GetName() << ": Coarse TOff shift by " << dRes[0];
              }
              else {
                if (iSmType != 5) {
                  dRes = find_tofedge((const char*) (h2y->GetName()), fTofClusterizer->GetEdgeThr(),
                                      fTofClusterizer->GetEdgeLen());
                  if (TMath::Abs(dRes[0]) < 0.5) {
                    //LOG(info) << "Now fit edge above noise, dT " << dRes[0] <<", T "<< iSmType;
                    double dTfind = dRes[0];
                    double dTmax  = dTfind + fTofClusterizer->GetEdgeFrange();
                    dRes          = fit_tofedge((const char*) (h2y->GetName()), dTmax, fTofClusterizer->GetEdgeThr());
                    if (std::isnan(dRes[0])) dRes[0] = dTfind;
                  }
                }
              }
              //LOG(info) << h2y->GetName() << ": shift TOff by " << dRes[0];
              fhCalChannelDt->Fill(dRes[0]);
              for (Int_t i = 0; i < NStrips; i++) {
                fhCorTOff[iDetIndx]->SetBinContent(i + 1, fhCorTOff[iDetIndx]->GetBinContent(i + 1) + dRes[0]);
              }
            } break;

            case 15:
            case 12:
            case 11: {
              LOG(info) << "Equalize velocities with chi2 deviations for " << fhCorTOff[iDetIndx]->GetName();
              TString hname2 = Form("cal_SmT%d_sm%03d_rpc%03d_TOff", iSmType, iSm, iRpc);

              double dTRefSum    = 0.;
              double dRefNormSum = 0.;
              double dNorm       = 0.;

              TH2* h2W = fhCalTofOff[iDetIndx];

              if (h2W->GetEntries() < MINCTS) continue;

              TH2* h2       = fhCalTOff[iDetIndx];
              Int_t NStrips = h2->GetNbinsX();
              TH1* h2yAv;
              double dResIni[1] = {0.};
              dRes              = &dResIni[0];

              TH1* h2Wy    = h2W->ProjectionY(Form("%s_py", h2W->GetName()), 1, NStrips);
              h2yAv        = h2->ProjectionY(Form("%s_py", h2->GetName()), 1, NStrips);
              double dEdge = 0.;

              double dMean = TruncatedMeanY(h2W);  //h2Wy->GetMean();
              if (iSmType == 5) {
                dEdge = h2Wy->GetBinCenter(h2Wy->GetMaximumBin());
                dNorm = h2Wy->GetBinContent(h2Wy->GetMaximumBin());
              }
              else {
                dRes  = find_tofedge((const char*) (h2Wy->GetName()), fTofClusterizer->GetEdgeThr(),
                                    fTofClusterizer->GetEdgeLen());
                dEdge = dRes[0];
                if ((dMean - dEdge) > 10.) {  //FIXME: constant in code
                  LOG(warn) << h2Wy->GetName() << ": wrong edge detected " << dEdge << ", Mean " << dMean;
                  dEdge = dMean;
                }
              }
              /*
              LOG(info) << h2Wy->GetName() << ": Coarse shift at " << h2Wy->GetMaximumBin() 
                        << ", Edge " << dEdge   
                        << ", " << h2yAv->GetBinCenter(1)
                        << ", " << h2yAv->GetBinCenter(h2yAv->GetNbinsX());
              */
              if (dEdge < h2yAv->GetBinCenter(1) || dEdge > h2yAv->GetBinCenter(h2yAv->GetNbinsX())) {
                dRes[0] = dEdge;  // - fTofClusterizer->GetEdgeTbias();
                //LOG(info) << h2Wy->GetName() << ": Coarse TOff shift by " << dRes[0];
              }
              else {
                if (iSmType != 5) {
                  dRes = find_tofedge((const char*) (h2yAv->GetName()), fTofClusterizer->GetEdgeThr(),
                                      fTofClusterizer->GetEdgeLen());
                  if (TMath::Abs(dRes[0]) < dValidEdge) {
                    double dTfind = dRes[0];
                    double dTmax  = dTfind + fTofClusterizer->GetEdgeFrange();
                    dRes          = fit_tofedge((const char*) (h2yAv->GetName()), dTmax, fTofClusterizer->GetEdgeThr());
                    if (std::isnan(dRes[0])) dRes[0] = dTfind;
                    LOG(info) << h2yAv->GetName() << ": fitted AvEdge up to " << dTmax << ", Res " << dRes[0]
                              << ", FindE " << dTfind;
                  }
                }
              }

              if (iSmType != 5) {
                fhCalCounterDt->Fill(dRes[0] - fTofClusterizer->GetEdgeTbias());
                if (TMath::Abs(dRes[0] - fTofClusterizer->GetEdgeTbias()) > -fhCalCounterDt->GetBinCenter(1)) {
                  LOG(warn) << "Channel ooCR: " << fhCorTOff[iDetIndx]->GetName() << ", " << dRes[0] << ", "
                            << fTofClusterizer->GetEdgeTbias() << ", " << -fhCalCounterDt->GetBinCenter(1);
                }
              }
              else {
                fhCalCounterDt->Fill(dEdge);
              }

              const double dResAv    = dRes[0];
              const int nValues      = MaxShift * 2 + 1;
              const double dRangeFac = 1.;
              double chi2Shift[NStrips][nValues];
              double chi2Minimum[NStrips];
              double chi2MinShift[NStrips];
              double chi2LowMinNeighbor[NStrips];
              double chi2LowMinNeighborShift[NStrips];
              for (Int_t i = 0; i < NStrips; i++) {
                double dTminShift = 0.;
                double dEdgei     = 0.;
                double dMeani     = 0.;
                TH1* h2y          = h2->ProjectionY(Form("%s_py%d", h2->GetName(), i), i + 1, i + 1);
                if (h2y->GetEntries() < MINCTS) continue;
                LOG(info) << h2y->GetName() << ": inspect with iOpt1 = " << iOpt1;
                if (kTRUE) {
                  if (iOpt1 == 11 || iOpt1 == 15) {
                    //check peak position
                    TH1* h2Wyi = h2W->ProjectionY(Form("%s_py%d", h2W->GetName(), i), i + 1, i + 1);
                    if (iSmType == 5) {
                      dEdgei = h2Wy->GetBinCenter(h2Wy->GetMaximumBin());
                      dNorm  = h2Wy->GetBinContent(h2Wy->GetMaximumBin());
                    }
                    else {
                      dRes   = find_tofedge((const char*) (h2Wyi->GetName()), fTofClusterizer->GetEdgeThr(),
                                          fTofClusterizer->GetEdgeLen());
                      dEdgei = dRes[0];
                    }

                    if (dEdgei < dRangeFac * h2y->GetBinCenter(1)
                        || dEdgei > dRangeFac * h2y->GetBinCenter(h2y->GetNbinsX())) {
                      dMeani = h2Wyi->GetMean();
                      LOG(info) << h2Wyi->GetName() << " ooR: " << dRes[0] << ", E " << dEdgei << ", Av " << dResAv
                                << ", Meani " << dMeani << ", MeanY " << dMean;
                      if ((dMeani - dEdgei) > 10.) {  //FIXME: constant in code
                        LOG(warn) << h2Wyi->GetName() << ": invalid edge detected " << dEdgei << ", Mean " << dMeani;
                        dRes[0] = dMeani - dMean;
                      }
                      //dRes[0]=dEdgei
                    }
                    else {
                      if (iSmType != 5) {
                        dRes = find_tofedge((const char*) (h2y->GetName()), fTofClusterizer->GetEdgeThr(),
                                            fTofClusterizer->GetEdgeLen());
                        LOG(info) << h2y->GetName() << " CheckValidEdge: " << dRes[0] << ", " << dValidEdge;
                        if (TMath::Abs(dRes[0]) < dValidEdge) {
                          switch (iOpt1) {
                            case 11: {
                              double dTfind = dRes[0];
                              double dTmax  = dTfind + fTofClusterizer->GetEdgeFrange();
                              dRes = fit_tofedge((const char*) (h2y->GetName()), dTmax, fTofClusterizer->GetEdgeThr());
                              if (std::isnan(dRes[0])) dRes[0] = dTfind;
                              LOG(info) << h2y->GetName() << ": fit edge up to " << dTmax << ", Res " << dRes[0]
                                        << ", FindE " << dTfind;
                            } break;
                            case 15:
                              chi2Minimum[i]        = 1.E6;
                              chi2LowMinNeighbor[i] = 1.E6;
                              int idx               = 0;
                              for (int j = -MaxShift; j < MaxShift + 1; j++) {
                                chi2Shift[i][idx] = CalcChi2(h2yAv, h2y, j);
                                if (chi2Shift[i][idx] < chi2Minimum[i]) {
                                  chi2LowMinNeighbor[i]      = chi2Minimum[i];
                                  chi2LowMinNeighborShift[i] = chi2MinShift[i];
                                  chi2Minimum[i]             = chi2Shift[i][idx];
                                  chi2MinShift[i]            = j * h2y->GetBinWidth(1);
                                }
                                else {
                                  if (chi2Shift[i][idx] < chi2LowMinNeighbor[i]) {
                                    chi2LowMinNeighbor[i]      = chi2Shift[i][idx];
                                    chi2LowMinNeighborShift[i] = j * h2y->GetBinWidth(1);
                                  }
                                }
                                idx++;
                              }
                              double chi2sum = chi2Minimum[i] + chi2LowMinNeighbor[i];
                              dTminShift     = (chi2MinShift[i] * (chi2sum - chi2Minimum[i])
                                            + chi2LowMinNeighborShift[i] * (chi2sum - chi2LowMinNeighbor[i]))
                                           / chi2sum;
                              //dTminShift=chi2MinShift[i];   // take Min only, for debugging
                              LOG(info) << h2y->GetName() << ": Chi2 shift " << dTminShift << ", Res " << dRes[0]
                                        << ", ResAv " << dResAv;
                              dRes[0] = dResAv;
                              break;
                          }
                        }
                        else {
                          LOG(info) << h2y->GetName() << ": stick to coarse offset " << dEdgei << ", Res " << dRes[0];
                          dRes[0] = dEdgei;  // use offset from wide histo
                        }
                      }
                      else {  // beam counter
                        dEdgei = h2y->GetBinCenter(h2y->GetMaximumBin());
                        dNorm  = h2y->GetBinContent(h2y->GetMaximumBin());
                      }

                      //LOG(info) << h2Wyi->GetName() << " TminShift: " << dTminShift;
                    }
                  }
                }
                if (iSmType == 5) {
                  LOG(info) << "Update StartCounterCalib " << i << ": " << dRes[0] << ", " << dBeamTOff << ", "
                            << dTminShift << ", " << dEdgei << ", N " << dNorm;
                  dRes[0] = -dEdgei;
                }
                else {
                  dTminShift -= fTofClusterizer->GetEdgeTbias();  // shift to physical scale
                }
                fhCalChannelDt->Fill(dRes[0] + dTminShift);
                if (TMath::Abs(dRes[0] + dTminShift) > -fhCalChannelDt->GetBinCenter(1)) {
                  LOG(warn) << "Channel ooHR: " << h2Wy->GetName() << i << ", " << dRes[0] << ", " << dTminShift;
                }
                double dVal = fhCorTOff[iDetIndx]->GetBinContent(i + 1);
                if (std::isnan(dVal)) fhCorTOff[iDetIndx]->SetBinContent(i + 1, 0.);  // fix NANs

                LOG(info) << h2Wy->GetName() << i << " CorTOff: " << dRes[0] << ", " << dResAv << ", " << dTminShift
                          << ", " << fhCorTOff[iDetIndx]->GetBinContent(i + 1) << " -> "
                          << fhCorTOff[iDetIndx]->GetBinContent(i + 1) + dRes[0] + dTminShift;

                fhCorTOff[iDetIndx]->SetBinContent(i + 1,
                                                   (fhCorTOff[iDetIndx]->GetBinContent(i + 1) + dRes[0] + dTminShift));
                if (iSmType == 5) {
                  dTRefSum += fhCorTOff[iDetIndx]->GetBinContent(i + 1) * dNorm;
                  dRefNormSum += dNorm;
                }
                //LOG(info)<<fhCorTOff[iDetIndx]->GetName() << ": new TOff " << fhCorTOff[iDetIndx]->GetBinContent(i + 1);
              }                    //for (Int_t i = 0; i < NStrips; i++)
              if (iSmType == 5) {  // avoid runaway
                double dTRefAv = dTRefSum / dRefNormSum;
                LOG(info) << "Shift average ref time to 0: " << dTRefAv;
                for (Int_t i = 0; i < NStrips; i++) {
                  fhCorTOff[iDetIndx]->SetBinContent(i + 1, (fhCorTOff[iDetIndx]->GetBinContent(i + 1) - dTRefAv));
                }
              }
            } break;

            case 14:
            case 13: {
              LOG(info) << "TofCalibrator: skip TOff, calibrate hit positions, check YBox, Opt = " << iOpt;
            } break;
            case 16: {
              LOG(info) << "TofCalibrator: determine counter time offsets with cosmics, Opt = " << iOpt;
              TH2* h2  = fhCalTOff[iDetIndx];
              TH2* h2W = fhCalTofOff[iDetIndx];
              if (NULL == h2) {
                LOG(warn) << "Calibration data not available for detx " << iDetIndx;
                continue;
              }
              Int_t NStrips = h2->GetNbinsX();
              TH1* h2Wy     = h2W->ProjectionY(Form("%s_py", h2W->GetName()), 1, NStrips);
              TH1* h2y      = h2->ProjectionY(Form("%s_py", h2->GetName()), 1, NStrips);

              Double_t dCtsWy = h2Wy->GetEntries();
              Double_t dDt    = 0;
              if (dCtsWy > MINCTS) {
                double dFMean      = h2Wy->GetBinCenter(h2Wy->GetMaximumBin());
                double dFLim       = 0.5;  // CAUTION, fixed numeric value
                double dBinSize    = h2Wy->GetBinWidth(1);
                dFLim              = TMath::Max(dFLim, 5. * dBinSize);
                TFitResultPtr fRes = h2Wy->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
                if (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED")) {
                  dDt = fRes->Parameter(1);
                  if (TMath::Abs(dDt) < 0.9 * h2y->GetXaxis()->GetXmax()) {
                    dFMean   = h2y->GetBinCenter(h2y->GetMaximumBin());
                    dFLim    = 0.5;  // CAUTION, fixed numeric value
                    dBinSize = h2y->GetBinWidth(1);
                    dFLim    = TMath::Max(dFLim, 5. * dBinSize);
                    fRes     = h2y->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
                    if (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED")) {
                      dDt = fRes->Parameter(1);
                    }
                  }
                  fhCalCounterDt->Fill(dDt);
                }
                LOG(info) << "Update cor hist " << fhCorTOff[iDetIndx]->GetName() << " by " << dDt << ", " << dBeamTOff;
                for (int iCh = 0; iCh < fhCorTOff[iDetIndx]->GetNbinsX(); iCh++) {
                  fhCalChannelDt->Fill(dDt);
                  fhCorTOff[iDetIndx]->SetBinContent(iCh + 1, fhCorTOff[iDetIndx]->GetBinContent(iCh + 1) + dDt);
                }
              }
            } break;

            case 17: {
              LOG(info) << "TofCalibrator: determine channel time offsets with cosmics, Opt = " << iOpt;
              TH2* h2  = fhCalTOff[iDetIndx];
              TH2* h2W = fhCalTofOff[iDetIndx];
              if (NULL == h2) {
                LOG(warn) << "Calibration data not available for detx " << iDetIndx;
                continue;
              }
              Int_t NStrips = h2->GetNbinsX();
              for (int i = 0; i < NStrips; i++) {
                TH1* h2Wy = h2W->ProjectionY(Form("%s_py_%02d", h2W->GetName(), i), i + 1, i + 1);
                TH1* h2y  = h2->ProjectionY(Form("%s_py_%02d", h2->GetName(), i), i + 1, i + 1);

                Double_t dCtsWy = h2Wy->GetEntries();
                Double_t dDt    = 0;
                if (dCtsWy > MINCTS) {
                  double dFMean      = h2Wy->GetBinCenter(h2Wy->GetMaximumBin());
                  double dFLim       = 0.5;  // CAUTION, fixed numeric value
                  double dBinSize    = h2Wy->GetBinWidth(1);
                  dFLim              = TMath::Max(dFLim, 5. * dBinSize);
                  TFitResultPtr fRes = h2Wy->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
                  Int_t iFitStatus   = fRes;
                  //LOG(warn)<<h2Wy->GetName() << ":  "<< iFitStatus << " " <<fRes;
                  if (iFitStatus != -1)
                    if (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED")) {
                      dDt = fRes->Parameter(1);
                      //LOG(warn)<<h2Wy->GetName() << " Dt "<< dDt;
                      if (h2y->GetEntries() > MINCTS)
                        if (TMath::Abs(dDt) < 0.9 * h2y->GetXaxis()->GetXmax()) {
                          dFMean   = h2y->GetBinCenter(h2y->GetMaximumBin());
                          dFLim    = 0.5;  // CAUTION, fixed numeric value
                          dBinSize = h2y->GetBinWidth(1);
                          dFLim    = TMath::Max(dFLim, 5. * dBinSize);
                          fRes     = h2y->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
                          if (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED")) {
                            dDt = fRes->Parameter(1);
                          }
                        }
                      fhCalChannelDt->Fill(dDt);
                      fhCorTOff[iDetIndx]->SetBinContent(i + 1, fhCorTOff[iDetIndx]->GetBinContent(i + 1) + dDt);
                    }
                }
                else {
                  LOG(warn) << "Too few counts in " << h2Wy->GetName() << ": " << dCtsWy;
                }
              }
            } break;

            case 18: {
              LOG(info) << "TofCalibrator: determine channel walks with cosmics, Opt = " << iOpt << " for TSR "
                        << iSmType << iSm << iRpc;
              //if (iSmType == 5) continue;  // no walk correction for beam counter up to now
              const Double_t MinCounts = 10.;
              Int_t iNbCh              = fDigiBdfPar->GetNbChan(iSmType, iRpc);
              for (Int_t iCh = 0; iCh < iNbCh; iCh++) {
                TH1* hCY = fhCalPos[iDetIndx]->ProjectionY(Form("%s_py_%d", fhCalPos[iDetIndx]->GetName(), iCh),
                                                           iCh + 1, iCh + 1);
                if (hCY->GetEntries() > 1)
                  fhCalChannelDy->Fill(hCY->GetRMS() / fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc));
                //LOG(info) << "Get walk histo pointer for TSRCS " << iSmType<<iSm<<iRpc<<iCh<<iSide;
                TProfile* hpW0 = fhCalWalk[iDetIndx][iCh][0]->ProfileX();     // mean deviation
                TH1* hCW0      = fhCalWalk[iDetIndx][iCh][0]->ProjectionX();  // contributing counts
                TProfile* hpW1 = fhCalWalk[iDetIndx][iCh][1]->ProfileX();     // mean deviation
                TH1* hCW1      = fhCalWalk[iDetIndx][iCh][1]->ProjectionX();  // contributing counts
                if (hCW0->GetEntries() == 0 || hCW1->GetEntries() == 0) {
                  //LOG(info) << "No entries in " << hCW0->GetName();
                  continue;
                }
                Double_t dCorT = 0;
                for (Int_t iBin = 0; iBin < fhCorWalk[iDetIndx][iCh][0]->GetNbinsX(); iBin++) {
                  Double_t dCts0  = hCW0->GetBinContent(iBin + 1);
                  Double_t dCts1  = hCW1->GetBinContent(iBin + 1);
                  Double_t dWOff0 = fhCorWalk[iDetIndx][iCh][0]->GetBinContent(iBin + 1);  // current value
                  Double_t dWOff1 = fhCorWalk[iDetIndx][iCh][1]->GetBinContent(iBin + 1);  // current value
                  if (iBin > 0 && dCts0 == 0 && dCts1 == 0) {
                    fhCorWalk[iDetIndx][iCh][0]->SetBinContent(iBin + 1,
                                                               fhCorWalk[iDetIndx][iCh][0]->GetBinContent(iBin));
                    fhCorWalk[iDetIndx][iCh][1]->SetBinContent(iBin + 1,
                                                               fhCorWalk[iDetIndx][iCh][1]->GetBinContent(iBin));
                  }
                  dCorT = 0.;
                  if (dCts0 > MinCounts && dCts1 > MinCounts) {
                    dCorT = 0.5 * (hpW0->GetBinContent(iBin + 1) + hpW1->GetBinContent(iBin + 1));
                    fhCalChannelDt->Fill(dCorT);
                  }
                  fhCorWalk[iDetIndx][iCh][0]->SetBinContent(iBin + 1, dWOff0 + dCorT);  //set new value
                  fhCorWalk[iDetIndx][iCh][1]->SetBinContent(iBin + 1, dWOff1 + dCorT);  //set new value
                }
              }
            } break;

            default:
              LOG(info) << "TofCalibrator: unknown option " << iOpt;
              return kTRUE;
              break;
          }  // switch iOpt1 end

          // re-adjust positions
          if (fhCalPosition[iDetIndx]->GetEntries() == 0) continue;
          double dYShift           = 0;
          TH1* hCalP               = fhCalPosition[iDetIndx]->ProjectionX();
          TH1* hCalPos_py          = fhCalPosition[iDetIndx]->ProjectionY();  // full counter
          dYShift                  = hCalPos_py->GetMean();
          int iChId                = CbmTofAddress::GetUniqueAddress(iSm, iRpc, 0, 0, iSmType);
          CbmTofCell* fChannelInfo = fDigiPar->GetCell(iChId);
          if (NULL == fChannelInfo) LOG(fatal) << Form("invalid ChannelInfo for 0x%08x", iChId);
          double dLen = fChannelInfo->GetSizey() * fTofClusterizer->GetModifySigvel();
          fTofClusterizer->fit_ybox(hCalPos_py, dLen);
          TF1* ff = hCalPos_py->GetFunction("YBox");  // box fit for monitoring signal velocity
          if (NULL != ff) {
            LOG(info) << "FRes YBox " << hCalPos_py->GetEntries() << " entries in TSR " << iSmType << iSm << iRpc
                      << ", chi2 " << ff->GetChisquare() / ff->GetNDF()
                      << Form(", striplen (%5.2f): %7.2f +/- %5.2f, pos "
                              "res %5.2f +/- %5.2f at y_cen = %5.2f +/- %5.2f",
                              dLen * fTofClusterizer->GetModifySigvel(), 2. * ff->GetParameter(1),
                              2. * ff->GetParError(1), ff->GetParameter(2), ff->GetParError(2), ff->GetParameter(3),
                              ff->GetParError(3));
            //dYShift = ff->GetParameter(3); // update common shift by box fit
            if (iOpt1 == 14) {  // Update signal velocity
              if (hSvel[iSmType] != NULL) {
                int iNbRpc    = fDigiBdfPar->GetNbRpc(iSmType);
                double dSscal = hSvel[iSmType]->GetBinContent(iSm * iNbRpc + iRpc + 1);
                double dRatio = dLen * fTofClusterizer->GetModifySigvel() / 2. / ff->GetParameter(1);
                LOG(info) << "Modify SigVel modifier for TSR " << iSmType << iSm << iRpc << ": " << dSscal << ", "
                          << dRatio << " -> " << dSscal * dRatio;
                dSscal = dSscal * dRatio;
                if (dSscal < 0.8) dSscal = 0.8;
                if (dSscal > 1.2) dSscal = 1.2;
                hSvel[iSmType]->SetBinContent(iSm * iNbRpc + iRpc + 1,
                                              dSscal);  // does not work in input file directory
              }
            }
          }

          double dThr = hCalPos_py->GetBinContent(hCalPos_py->GetMaximumBin()) / 10;
          if (dThr < 3.) dThr = 3.;
          dRes = fTofClusterizer->find_yedges((const char*) (hCalPos_py->GetName()), dThr, fChannelInfo->GetSizey());
          if (TMath::Abs(dRes[0] - fChannelInfo->GetSizey()) / fChannelInfo->GetSizey() < 0.1) {
            dYShift = dRes[1];
          }

          if (iOpt1 != 16 && iOpt1 != 12) {
            for (Int_t iBin = 0; iBin < fhCorPos[iDetIndx]->GetNbinsX(); iBin++) {
              Double_t dCorP = fhCorPos[iDetIndx]->GetBinContent(iBin + 1);
              Double_t dCtsP = hCalP->GetBinContent(iBin + 1);
              if (dCtsP > MINCTS) {
                TH1* hPos_py = fhCalPosition[iDetIndx]->ProjectionY(
                  Form("%s_py_%d", fhCalPosition[iDetIndx]->GetName(), iBin), iBin + 1, iBin + 1);
                dYShift = hPos_py->GetMean();
                dThr    = hPos_py->GetBinContent(hPos_py->GetMaximumBin()) / 10;
                if (dThr < 3.) dThr = 3.;
                dRes = fTofClusterizer->find_yedges((const char*) (hPos_py->GetName()), dThr, fChannelInfo->GetSizey());

                LOG(info) << Form("EdgeY for %s, TSR %d%d%d: DY %5.2f, Len %5.2f, Size %5.2f ", hPos_py->GetName(),
                                  iSmType, iSm, iRpc, dRes[1], dRes[0],
                                  fChannelInfo->GetSizey() * fTofClusterizer->GetModifySigvel());

                if (TMath::Abs(dRes[0] - fChannelInfo->GetSizey()) / fChannelInfo->GetSizey() < 0.1) {
                  dYShift = dRes[1];
                }
                else {
                  dYShift = hPos_py->GetMean();
                }
                // apply correction
                //LOG(info)<<Form("UpdateCalPos TSRC %d%d%d%2d: %6.2f -> %6.2f ",iSmType,iSm,iRpc,iBin,dCorP,dCorP+dYShift);
                fhCorPos[iDetIndx]->SetBinContent(iBin + 1, dCorP + dYShift);
                fhCalChannelDy->Fill(dYShift);
              }
            }
          }
          else {  // no individuak y-corrections
            for (Int_t iBin = 0; iBin < fhCorPos[iDetIndx]->GetNbinsX(); iBin++) {
              Double_t dCorP = fhCorPos[iDetIndx]->GetBinContent(iBin + 1);
              fhCorPos[iDetIndx]->SetBinContent(iBin + 1, dCorP + dYShift);
              fhCalChannelDy->Fill(dYShift);
            }
          }
        } break;  // case iOpt0=0

        case 1:  // update channel mean
        {
          LOG(info) << "Update time offsets for TSR " << iSmType << iSm << iRpc << " with opt1 " << iOpt1;
          TProfile* hpP = fhCalPos[iDetIndx]->ProfileX();
          TProfile* hpT = fhCalTOff[iDetIndx]->ProfileX();
          TH1* hCalP    = fhCalPos[iDetIndx]->ProjectionX();
          TH1* hCalT    = fhCalTOff[iDetIndx]->ProjectionX();
          //fhCorPos[iDetIndx]->Add((TH1 *)hpP,-1.);
          //fhCorTOff[iDetIndx]->Add((TH1*)hpT,-1.);
          if (iOpt1 == 3) {  // update counter times from track pulls
            TH1* hCalTpy     = fhCalTOff[iDetIndx]->ProjectionY();
            Double_t dCtsTpy = hCalTpy->GetEntries();
            Double_t dDt     = 0;
            if (dCtsTpy > MINCTS) {
              double dFMean      = hCalTpy->GetBinCenter(hCalTpy->GetMaximumBin());
              double dFLim       = 0.5;  // CAUTION, fixed numeric value
              double dBinSize    = hCalTpy->GetBinWidth(1);
              dFLim              = TMath::Max(dFLim, 5. * dBinSize);
              TFitResultPtr fRes = hCalTpy->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
              if (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED")) {
                dDt = fRes->Parameter(1);
                LOG(info) << "Update cor hist " << fhCorTOff[iDetIndx]->GetName() << " by " << dDt << ", " << dBeamTOff;
                if (iSmType == 5) {  // do not shift beam counter in time
                  dDt -= dBeamTOff;
                }
                else {
                  dDt += dBeamTOff;
                }
                fhCalCounterDt->Fill(dDt);
                for (int iCh = 0; iCh < fhCorTOff[iDetIndx]->GetNbinsX(); iCh++) {
                  fhCalChannelDt->Fill(dDt);
                  fhCorTOff[iDetIndx]->SetBinContent(iCh + 1, fhCorTOff[iDetIndx]->GetBinContent(iCh + 1) + dDt);
                }
              }
            }
          }
          else {
            double dTOffMeanShift = 0.;
            double dNCh           = 0.;
            for (Int_t iBin = 0; iBin < fhCorTOff[iDetIndx]->GetNbinsX(); iBin++) {
              Double_t dDt   = hpT->GetBinContent(iBin + 1);
              Double_t dCorT = fhCorTOff[iDetIndx]->GetBinContent(iBin + 1);
              Double_t dCtsT = hCalT->GetBinContent(iBin + 1);
              Double_t dCtsP = hCalP->GetBinContent(iBin + 1);
              Double_t dDp   = hpP->GetBinContent(iBin + 1);
              Double_t dCorP = fhCorPos[iDetIndx]->GetBinContent(iBin + 1);
              LOG(debug) << "Cts check for " << fhCalTOff[iDetIndx]->GetName() << ", bin " << iBin << ": " << dCtsT
                         << ", " << dCtsP << ", " << MINCTS;
              if (dCtsT > MINCTS && dCtsP > MINCTS) {
                // Fit Gaussian around peak
                TH1* hpPy =
                  (TH1*) fhCalPos[iDetIndx]->ProjectionY(Form("PosPy_%d_%d", iDetIndx, iBin), iBin + 1, iBin + 1);
                if (hpPy->Integral() > MINCTS) {
                  Double_t dFMean    = hpPy->GetBinCenter(hpPy->GetMaximumBin());
                  Double_t dFLim     = 0.5;  // CAUTION, fixed numeric value
                  Double_t dBinSize  = hpPy->GetBinWidth(1);
                  dFLim              = TMath::Max(dFLim, 5. * dBinSize);
                  TFitResultPtr fRes = hpPy->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
                  if (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED")) {
                    dDp = fRes->Parameter(1);  //overwrite mean
                    fhCalChannelDy->Fill(dDt);
                  }
                }
                TH1* hpTy =
                  (TH1*) fhCalTOff[iDetIndx]->ProjectionY(Form("TOffPy_%d_%d", iDetIndx, iBin), iBin + 1, iBin + 1);
                if (hpTy->Integral() > MINCTS) {
                  Double_t dFMean    = hpTy->GetBinCenter(hpTy->GetMaximumBin());
                  Double_t dFLim     = 0.5;  // CAUTION, fixed numeric value
                  Double_t dBinSize  = hpTy->GetBinWidth(1);
                  dFLim              = TMath::Max(dFLim, 5. * dBinSize);
                  TFitResultPtr fRes = hpTy->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
                  if (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED")) {
                    dDt = fRes->Parameter(1);  //overwrite mean
                    fhCalChannelDt->Fill(dDt);
                  }
                }
                // Double_t dDpRes = fRes->Parameter(2);
                if (iSmType == 5) {  // do not shift beam counter in time
                  fhCorTOff[iDetIndx]->SetBinContent(iBin + 1, dCorT + dDt - dBeamTOff);
                  dTOffMeanShift += dDt - dBeamTOff;
                }
                else {
                  fhCorTOff[iDetIndx]->SetBinContent(iBin + 1, dCorT + dDt + dBeamTOff);
                  dTOffMeanShift += dDt + dBeamTOff;
                }
                dNCh += 1.;
                if (iOpt1 > 0) {  // active  y-position correction
                  fhCorPos[iDetIndx]->SetBinContent(iBin + 1, dCorP + dDp);
                }
                if (iDetIndx > -1) {
                  LOG(debug) << Form("Update %s: bin %02d, Cts: %d, Old %6.3f, dev %6.3f, beam %6.3f, new %6.3f",
                                     fhCorTOff[iDetIndx]->GetName(), iBin, (Int_t) dCtsT, dCorT, dDt, dBeamTOff,
                                     dCorT + dDt + dBeamTOff);
                }
              }
            }
            if (dNCh > 0) dTOffMeanShift /= dNCh;
            //LOG(info) << "Apply dTOffMeanShift " << dTOffMeanShift << " to " << fhCorTOff[iDetIndx]->GetName();
            for (Int_t iBin = 0; iBin < fhCorTOff[iDetIndx]->GetNbinsX(); iBin++) {  //preserve mean offset
              fhCorTOff[iDetIndx]->SetBinContent(iBin + 1,
                                                 fhCorTOff[iDetIndx]->GetBinContent(iBin + 1) - dTOffMeanShift);
            }
          }
        } break;
        case 2: {  // iopt1==11: update individual channel walks from inside Cluster deviations
          switch (iOpt1) {
            case 10: {
              LOG(debug) << "Update Cluster Offsets for TSR " << iSmType << iSm << iRpc;
              if (iSmType == 5) continue;  // do not shift beam counter in time
              //TProfile* hpP = fhCalDelPos[iDetIndx]->ProfileX();
              TProfile* hpT = fhCalDelTOff[iDetIndx]->ProfileX();
              TH1* hCalP    = fhCalDelPos[iDetIndx]->ProjectionX();
              TH1* hCalT    = fhCalDelTOff[iDetIndx]->ProjectionX();
              //double dTOffMeanShift = 0.;
              //double dNCh           = 0.;
              for (Int_t iBin = 0; iBin < fhCorTOff[iDetIndx]->GetNbinsX(); iBin++) {
                Double_t dDt   = hpT->GetBinContent(iBin + 1);
                Double_t dCorT = fhCorTOff[iDetIndx]->GetBinContent(iBin + 1);
                Double_t dCtsT = hCalT->GetBinContent(iBin + 1);
                Double_t dCtsP = hCalP->GetBinContent(iBin + 1);
                //Double_t dDp   = hpP->GetBinContent(iBin + 1);
                //Double_t dCorP = fhCorPos[iDetIndx]->GetBinContent(iBin + 1);
                LOG(debug) << "Cts check for " << fhCalDelTOff[iDetIndx]->GetName() << ", bin " << iBin << ": " << dCtsT
                           << ", " << dCtsP << ", " << MINCTS;
                if (dCtsT > MINCTS) {  //&& dCtsP > MINCTS) {
                                       // Fit Gaussian around peak
                  TH1* hpTy       = (TH1*) fhCalDelTOff[iDetIndx]->ProjectionY(Form("DelTOffPy_%d_%d", iDetIndx, iBin),
                                                                         iBin + 1, iBin + 1);
                  double dFMean   = hpTy->GetBinCenter(hpTy->GetMaximumBin());
                  double dFLim    = 0.5;  // CAUTION, fixed numeric value
                  double dBinSize = hpTy->GetBinWidth(1);
                  dFLim           = TMath::Max(dFLim, 5. * dBinSize);
                  TFitResultPtr fRes = hpTy->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
                  if (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED")) {
                    dDt = fRes->Parameter(1);  //overwrite mean
                  }
                  fhCorTOff[iDetIndx]->SetBinContent(iBin + 1, dCorT + dDt);
                  fhCalChannelDt->Fill(dDt);
                }
                if (iDetIndx > -1) {
                  LOG(debug) << Form("Update %s: bin %02d, Cts: %d, Old %6.3f, dev %6.3f, new %6.3f",
                                     fhCorTOff[iDetIndx]->GetName(), iBin, (Int_t) dCtsT, dCorT, dDt, dCorT + dDt);
                }
              }
            } break;
            case 12: {
              LOG(debug) << "Update Cluster Position for TSR " << iSmType << iSm << iRpc;
              if (iSmType == 5) continue;  // do not shift beam counter in time
              TProfile* hpP = fhCalDelPos[iDetIndx]->ProfileX();
              TProfile* hpT = fhCalDelTOff[iDetIndx]->ProfileX();
              TH1* hCalP    = fhCalDelPos[iDetIndx]->ProjectionX();
              TH1* hCalT    = fhCalDelTOff[iDetIndx]->ProjectionX();
              //double dTOffMeanShift = 0.;
              //double dNCh           = 0.;
              for (Int_t iBin = 0; iBin < fhCorTOff[iDetIndx]->GetNbinsX(); iBin++) {
                Double_t dDt   = hpT->GetBinContent(iBin + 1);
                Double_t dCorT = fhCorTOff[iDetIndx]->GetBinContent(iBin + 1);
                Double_t dCtsT = hCalT->GetBinContent(iBin + 1);
                Double_t dCtsP = hCalP->GetBinContent(iBin + 1);
                Double_t dDp   = hpP->GetBinContent(iBin + 1);
                Double_t dCorP = fhCorPos[iDetIndx]->GetBinContent(iBin + 1);
                LOG(debug) << "Cts check for " << fhCalDelTOff[iDetIndx]->GetName() << ", bin " << iBin << ": " << dCtsT
                           << ", " << dCtsP << ", " << MINCTS;
                if (dCtsT > MINCTS && dCtsP > MINCTS) {
                  // Fit Gaussian around peak
                  TH1* hpTy       = (TH1*) fhCalDelTOff[iDetIndx]->ProjectionY(Form("DelTOffPy_%d_%d", iDetIndx, iBin),
                                                                         iBin + 1, iBin + 1);
                  double dFMean   = hpTy->GetBinCenter(hpTy->GetMaximumBin());
                  double dFLim    = 0.5;  // CAUTION, fixed numeric value
                  double dBinSize = hpTy->GetBinWidth(1);
                  dFLim           = TMath::Max(dFLim, 5. * dBinSize);
                  TFitResultPtr fRes = hpTy->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
                  if (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED")) {
                    dDt = fRes->Parameter(1);  //overwrite mean
                  }
                  fhCorTOff[iDetIndx]->SetBinContent(iBin + 1, dCorT + dDt);
                  fhCalChannelDt->Fill(dDt);

                  TH1* hpPy = (TH1*) fhCalDelPos[iDetIndx]->ProjectionY(Form("DelPosPy_%d_%d", iDetIndx, iBin),
                                                                        iBin + 1, iBin + 1);
                  dFMean    = hpPy->GetBinCenter(hpTy->GetMaximumBin());
                  dFLim     = 0.5;  // CAUTION, fixed numeric value
                  dBinSize  = hpPy->GetBinWidth(1);
                  dFLim     = TMath::Max(dFLim, 5. * dBinSize);
                  fRes      = hpPy->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
                  if (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED")) {
                    dDp = fRes->Parameter(1);  //overwrite mean
                  }
                  fhCorPos[iDetIndx]->SetBinContent(iBin + 1, dCorP + dDp);
                  fhCalChannelDy->Fill(dDp);
                }
                if (iDetIndx > -1) {
                  LOG(debug) << Form("Update %s: bin %02d, Cts: %d, Old %6.3f, dev %6.3f, new %6.3f",
                                     fhCorTOff[iDetIndx]->GetName(), iBin, (Int_t) dCtsT, dCorT, dDt, dCorT + dDt);
                }
              }
            } break;
            case 0:
            case 11: {
              LOG(debug) << "Update Walks for TSR " << iSmType << iSm << iRpc;
              //if (iSmType == 5) continue;  // no walk correction for beam counter up to now
              const Double_t MinCounts = 10.;
              Int_t iNbCh              = fDigiBdfPar->GetNbChan(iSmType, iRpc);
              for (Int_t iCh = 0; iCh < iNbCh; iCh++) {
                TH1* hCY = fhCalPos[iDetIndx]->ProjectionY(Form("%s_py_%d", fhCalPos[iDetIndx]->GetName(), iCh),
                                                           iCh + 1, iCh + 1);
                if (hCY->GetEntries() > 1) fhCalChannelDy->Fill(hCY->GetRMS());
                //LOG(info) << "Get walk histo pointer for TSRCS " << iSmType<<iSm<<iRpc<<iCh<<iSide;
                TProfile* hpW0 = fhCalWalk[iDetIndx][iCh][0]->ProfileX();     // mean deviation
                TH1* hCW0      = fhCalWalk[iDetIndx][iCh][0]->ProjectionX();  // contributing counts
                TProfile* hpW1 = fhCalWalk[iDetIndx][iCh][1]->ProfileX();     // mean deviation
                TH1* hCW1      = fhCalWalk[iDetIndx][iCh][1]->ProjectionX();  // contributing counts
                if (iOpt0 == 3) {                                             // use average distributions
                  hpW0 = fhCalWalkAv[iDetIndx]->ProfileX();                   // mean deviation
                  hCW0 = fhCalWalkAv[iDetIndx]->ProjectionX();                // contributing counts
                  hpW1 = fhCalWalkAv[iDetIndx]->ProfileX();                   // mean deviation
                  hCW1 = fhCalWalkAv[iDetIndx]->ProjectionX();                // contributing counts
                }
                if (hCW0->GetEntries() == 0 || hCW1->GetEntries() == 0) {
                  //LOG(info) << "No entries in " << hCW0->GetName();
                  continue;
                }
                Double_t dCorT = 0;
                for (Int_t iBin = 0; iBin < fhCorWalk[iDetIndx][iCh][0]->GetNbinsX(); iBin++) {
                  Double_t dCts0  = hCW0->GetBinContent(iBin + 1);
                  Double_t dCts1  = hCW1->GetBinContent(iBin + 1);
                  Double_t dWOff0 = fhCorWalk[iDetIndx][iCh][0]->GetBinContent(iBin + 1);  // current value
                  Double_t dWOff1 = fhCorWalk[iDetIndx][iCh][1]->GetBinContent(iBin + 1);  // current value
                  if (iBin > 0 && dCts0 == 0 && dCts1 == 0) {
                    fhCorWalk[iDetIndx][iCh][0]->SetBinContent(iBin + 1,
                                                               fhCorWalk[iDetIndx][iCh][0]->GetBinContent(iBin));
                    fhCorWalk[iDetIndx][iCh][1]->SetBinContent(iBin + 1,
                                                               fhCorWalk[iDetIndx][iCh][1]->GetBinContent(iBin));
                  }
                  dCorT = 0.;
                  if (dCts0 > MinCounts && dCts1 > MinCounts) {
                    dCorT = 0.5 * (hpW0->GetBinContent(iBin + 1) + hpW1->GetBinContent(iBin + 1));
                    fhCalChannelDt->Fill(dCorT);
                  }
                  fhCorWalk[iDetIndx][iCh][0]->SetBinContent(iBin + 1, dWOff0 + dCorT);  //set new value
                  fhCorWalk[iDetIndx][iCh][1]->SetBinContent(iBin + 1, dWOff1 + dCorT);  //set new value
                  if (iSmType == 0 && iSm == 0 && iRpc == 2)                             // debugging
                    LOG(info) << "UpdWalk " << fhCorWalk[iDetIndx][iCh][0]->GetName() << " bin " << iBin << " Tot "
                              << hCW0->GetBinCenter(iBin + 1) << ", " << hCW1->GetBinCenter(iBin + 1) << ": curVal "
                              << dWOff0 << ", " << dWOff1 << ", Cts " << dCts0 << ", " << dCts1 << ", dCorT " << dCorT
                              << ", newVal " << dWOff0 + dCorT << ", " << dWOff1 + dCorT;
                }
                // determine effective/count rate weighted mean
                /*
                  Double_t dMean   = 0;
                  Double_t dCtsAll = 0.;
                  for (Int_t iBin = 0; iBin < fhCorWalk[iDetIndx][iCh][iSide]->GetNbinsX(); iBin++) {
                    Double_t dCts  = hCW->GetBinContent(iBin + 1);
                    Double_t dWOff = fhCorWalk[iDetIndx][iCh][iSide]->GetBinContent(iBin + 1);
                    if (dCts > MinCounts) {
                      dCtsAll += dCts;
                      dMean += dCts * dWOff;
                    }
                  }
                  if (dCtsAll > 0.) dMean /= dCtsAll;

                  //LOG(info) << "Mean shift for TSRCS " << iSmType << iSm << iRpc << iCh << iSide << ": " << dMean;
                  // keep mean value at 0
                  for (Int_t iBin = 0; iBin < fhCorWalk[iDetIndx][iCh][iSide]->GetNbinsX(); iBin++) {
                    Double_t dWOff = fhCorWalk[iDetIndx][iCh][iSide]->GetBinContent(iBin + 1);  // current value
                    if (iSmType == 0 && iSm == 0 && iRpc == 2)                                  // debugging
                    LOG(info) << "UpdWalk " << fhCorWalk[iDetIndx][iCh][iSide]->GetName() 
                              << " bin " << iBin << " Tot " << hCW->GetBinCenter(iBin + 1)
                              << ": curDev " << hpW->GetBinContent(iBin + 1)
                              << ", Cts "    << hCW->GetBinContent(iBin + 1)
                              << ", all "    << dCtsAll
                              << ", oldWOff " << dWOff - hpW->GetBinContent(iBin + 1) 
                              << ", Mean " << dMean
                              << ", newWOff " << dWOff - dMean;
                    fhCorWalk[iDetIndx][iCh][iSide]->SetBinContent(iBin + 1, dWOff + dMean);    //set new value                            
                  }
                */
              }
            } break;  //iopt1=11 end
            default:; LOG(info) << "Calibrator option " << iOpt << " not implemented ";
          }  // switch iOpt1 end
        } break;

        case 9:  // update channel means on cluster level
        {
          // position, do Edge fit by default
          //LOG(info) << "Update Offsets for TSR " << iSmType << iSm << iRpc;

          //TProfile* hpP = fhCalPosition[iDetIndx]->ProfileX();
          TH1* hCalP = fhCalPosition[iDetIndx]->ProjectionX();

          for (Int_t iBin = 0; iBin < fhCorPos[iDetIndx]->GetNbinsX(); iBin++) {
            Double_t dCorP = fhCorPos[iDetIndx]->GetBinContent(iBin + 1);
            //Double_t dDp   = hpP->GetBinContent(iBin + 1);
            Double_t dCtsP = hCalP->GetBinContent(iBin + 1);
            if (dCtsP > MINCTS) {
              TH1* hPos_py = fhCalPosition[iDetIndx]->ProjectionY(
                Form("%s_py_%d", fhCalPosition[iDetIndx]->GetName(), iBin), iBin + 1, iBin + 1);
              double dYShift           = hPos_py->GetMean();
              int iChId                = CbmTofAddress::GetUniqueAddress(iSm, iRpc, 0, 0, iSmType);
              CbmTofCell* fChannelInfo = fDigiPar->GetCell(iChId);
              if (NULL == fChannelInfo) LOG(fatal) << Form("invalid ChannelInfo for 0x%08x", iChId);

              double dThr = dCtsP / 100;
              if (dThr < 3.) dThr = 3.;
              double* dRes =
                fTofClusterizer->find_yedges((const char*) (hPos_py->GetName()), dThr, fChannelInfo->GetSizey());
              /*
          	  LOG(info) << Form("EdgeY for %s, TSR %d%d%d: DY %5.2f, Len %5.2f, Size %5.2f ",
          	                     hPos_py->GetName(), iSmType, iSm, iRpc, dRes[1], dRes[0], fChannelInfo->GetSizey());
              */
              if (TMath::Abs(dRes[0] - fChannelInfo->GetSizey()) / fChannelInfo->GetSizey() < 0.1) {
                dYShift = dRes[1];
              }
              else {
                dYShift = hPos_py->GetMean();
              }
              // apply correction
              //LOG(info)<<Form("UpdateCalPos TSRC %d%d%d%2d: %6.2f -> %6.2f ",iSmType,iSm,iRpc,iBin,dCorP,dCorP+dYShift);
              fhCorPos[iDetIndx]->SetBinContent(iBin + 1, dCorP + dYShift);
            }
          }

          //TofOff or TOff as input ?
          //TProfile* hpT = fhCalTofOff[iDetIndx]->ProfileX();
          //TH1* hCalT    = fhCalTofOff[iDetIndx]->ProjectionX();
          TProfile* hpT = fhCalTOff[iDetIndx]->ProfileX();
          TH1* hCalT    = fhCalTOff[iDetIndx]->ProjectionX();
          for (Int_t iBin = 0; iBin < fhCorTOff[iDetIndx]->GetNbinsX(); iBin++) {
            //Double_t dDt   = hpT->GetBinContent(iBin + 1);
            Double_t dCorT = fhCorTOff[iDetIndx]->GetBinContent(iBin + 1);
            Double_t dCtsT = hCalT->GetBinContent(iBin + 1);
            if (dCtsT > MINCTS) {
              double dTmean = hpT->GetBinContent(iBin + 1);
              TH1* hTy      = (TH1*) fhCalTofOff[iDetIndx]->ProjectionY(
                Form("%s_py%d", fhCalTofOff[iDetIndx]->GetName(), iBin), iBin + 1, iBin + 1);
              Double_t dNPeak = hTy->GetBinContent(hTy->GetMaximumBin());
              if (dNPeak > MINCTS * 0.1) {
                Double_t dFMean    = hTy->GetBinCenter(hTy->GetMaximumBin());
                Double_t dFLim     = 2.0;  // CAUTION, fixed numeric value
                Double_t dBinSize  = hTy->GetBinWidth(1);
                dFLim              = TMath::Max(dFLim, 10. * dBinSize);
                TFitResultPtr fRes = hTy->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
                //if (fRes == 0 )
                if (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED")) {
                  //if (TMath::Abs(TMean - fRes->Parameter(1)) > 1.)
                  /*
                  LOG(info) << "CalTOff  "
                            << Form("TSRC %d%d%d%d, %s gaus %8.2f %8.2f %8.2f for "
                                    "TM %8.2f, TBeam %6.2f",
                                    iSmType, iSm, iRpc, iBin, hTy->GetName(), fRes->Parameter(0), fRes->Parameter(1),
                                    fRes->Parameter(2), dTmean, dBeamTOff);
                  */
                  dTmean = fRes->Parameter(1);  //overwrite mean
                }
              }
              /*
              LOG(info) << Form("UpdateCalTOff TSRC %d%d%d%2d, cts %d: %6.2f -> %6.2f, %6.2f ", iSmType, iSm, iRpc,
                                iBin, (int) dCtsT, dCorT, dCorT + dTmean, dCorT + hpT->GetBinContent(iBin + 1));
              */
              fhCorTOff[iDetIndx]->SetBinContent(iBin + 1, dCorT + dTmean);
            }
          }

          //Tot
          double dCalTotMean = fTofClusterizer->GetTotMean();  // Target value
          for (Int_t iBin = 0; iBin < fhCorTot[iDetIndx]->GetNbinsX(); iBin++) {
            int ib          = iBin + 1;
            TH1* hbin       = fhCalTot[iDetIndx]->ProjectionY(Form("bin%d", ib), ib, ib);
            double dTotMean = hbin->GetMean();
            // Do gaus fit around maximum
            Double_t dCtsTot = hbin->GetEntries();
            if (dCtsTot > MINCTS) {
              double dFMean = hbin->GetBinCenter(hbin->GetMaximumBin());
              double dFLim  = dFMean * 0.5;  // CAUTION,
              //LOG(info)<<Form("FitTot TSRC %d%d%d%2d:  Mean %6.2f, Width %6.2f ",iSmType,iSm,iRpc,iBin,dFMean,dFLim);
              if (dFMean > 2.) {
                TFitResultPtr fRes = hbin->Fit("gaus", "SQM", "", dFMean - dFLim, dFMean + dFLim);
                if (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED")) {
                  dTotMean = fRes->Parameter(1);  //overwrite mean
                }
                else {
                  LOG(warn) << "FitFail " << hbin->GetName();
                }
              }
            }
            else {  //append to broken channel list
              int iCh   = iBin % 2;
              int iSide = iBin - iCh * 2;
              CbmTofDetectorInfo xDetInfo(ECbmModuleId::kTof, iSmType, iSm, iRpc, iCh, iSide);
              Int_t iChId   = fTofId->SetDetectorInfo(xDetInfo);
              TString shcmd = Form("echo %d >> %s  ", iChId, cBadChannelFile.Data());
              gSystem->Exec(shcmd.Data());
            }
            if (dTotMean > 0.) {  // prevent zero-divide
              double dCorTot = fhCorTot[iDetIndx]->GetBinContent(ib);
              /*
          	  LOG(info)<<Form("UpdateCalTot TSRC %d%d%d%2d: %6.2f, %6.2f, %6.2f -> %6.2f ",iSmType,iSm,iRpc,iBin,
          			  dCorTot,dCalTotMean,dTotMean,dCorTot*dTotMean/dCalTotMean);
          		*/
              fhCorTot[iDetIndx]->SetBinContent(ib, dCorTot * dTotMean / dCalTotMean);
            }
          }

        } break;
        default: LOG(fatal) << "No valid calibration mode " << iOpt0;
      }  //switch( iOpt0) end
    }
  }
  else {
    // currently not needed
  }
  TString fFile = fCalParFile->GetName();
  if (!fFile.Contains("/")) {
    TFile* fCalParFileNew = new TFile(Form("New_%s", fCalParFile->GetName()), "RECREATE");
    WriteHist(fCalParFileNew);
    fCalParFileNew->Close();
  }
  else {
    WriteHist(fCalParFile);
  }
  fCalParFile->Close();

  /// Restore old global file and folder pointer to avoid messing with FairRoot
  gFile = oldFile;
  //gDirectory = oldDir;
  gDirectory->cd(oldDir->GetPath());

  return kTRUE;
}

void CbmTofCalibrator::ReadHist(TFile* fHist)
{
  LOG(info) << "Read Cor histos from file " << fHist->GetName();
  if (0 == fhCorPos.size()) {
    Int_t iNbDet = fDigiBdfPar->GetNbDet();
    //LOG(info) << "resize histo vector for " << iNbDet << " detectors ";
    fhCorPos.resize(iNbDet);
    fhCorTOff.resize(iNbDet);
    fhCorTot.resize(iNbDet);
    fhCorTotOff.resize(iNbDet);
    fhCorSvel.resize(iNbDet);
    fhCorWalk.resize(iNbDet);
  }

  for (Int_t iDetIndx = 0; iDetIndx < fDigiBdfPar->GetNbDet(); iDetIndx++) {
    Int_t iUniqueId = fDigiBdfPar->GetDetUId(iDetIndx);
    //Int_t iSmAddr   = iUniqueId & DetMask;
    Int_t iSmType = CbmTofAddress::GetSmType(iUniqueId);
    Int_t iSm     = CbmTofAddress::GetSmId(iUniqueId);
    Int_t iRpc    = CbmTofAddress::GetRpcId(iUniqueId);
    //LOG(info) << "Get histo pointer for TSR " << iSmType<<iSm<<iRpc;

    fhCorPos[iDetIndx] =
      (TH1*) gDirectory->FindObjectAny(Form("cl_CorSmT%01d_sm%03d_rpc%03d_Pos_pfx", iSmType, iSm, iRpc));
    if (NULL == fhCorPos[iDetIndx]) {
      LOG(error) << "hCorPos not found for TSR " << iSmType << iSm << iRpc;
      continue;
    }
    fhCorTOff[iDetIndx] =
      (TH1*) gDirectory->FindObjectAny(Form("cl_CorSmT%01d_sm%03d_rpc%03d_TOff_pfx", iSmType, iSm, iRpc));
    fhCorTot[iDetIndx] =
      (TH1*) gDirectory->FindObjectAny(Form("cl_CorSmT%01d_sm%03d_rpc%03d_Tot_Mean", iSmType, iSm, iRpc));
    fhCorTotOff[iDetIndx] =
      (TH1*) gDirectory->FindObjectAny(Form("cl_CorSmT%01d_sm%03d_rpc%03d_Tot_Off", iSmType, iSm, iRpc));

    Int_t iNbCh = fDigiBdfPar->GetNbChan(iSmType, iRpc);
    fhCorWalk[iDetIndx].resize(iNbCh);
    for (Int_t iCh = 0; iCh < iNbCh; iCh++) {
      fhCorWalk[iDetIndx][iCh].resize(2);
      for (Int_t iSide = 0; iSide < 2; iSide++) {
        //LOG(info) << "Get walk histo pointer for TSRCS " << iSmType<<iSm<<iRpc<<iCh<<iSide;
        TString hname = Form("Cor_SmT%01d_sm%03d_rpc%03d_Ch%03d_S%d_Walk_px", iSmType, iSm, iRpc, iCh, iSide);
        fhCorWalk[iDetIndx][iCh][iSide] = (TH1*) gDirectory->FindObjectAny(hname);
        if (NULL == fhCorWalk[iDetIndx][iCh][iSide]) {
          LOG(warn) << "No Walk histo for TSRCS " << iSmType << iSm << iRpc << iCh << iSide;
          if (NULL != fhCalWalk[iDetIndx][iCh][iSide]) {
            LOG(info) << "Create walk correction histo " << hname;
            fhCorWalk[iDetIndx][iCh][iSide] =
              new TH1D(hname,
                       Form("Walk in SmT%01d_sm%03d_rpc%03d_Ch%03d_S%01d_Walk; Tot [a.u.];  #DeltaT [ns]", iSmType, iSm,
                            iRpc, iCh, iSide),
                       nbClWalkBinX, fTofClusterizer->GetTotMin(), fTofClusterizer->GetTotMax());
          }
          //continue;
        }

        if (iSmType == 8 && iSide == 1) {  //pad special treatment
          LOG(warn) << "Overwrite pad counter walk for TSRCS " << iSmType << iSm << iRpc << iCh << iSide;
          TH1* hTmp = (TH1*) gDirectory->FindObjectAny(
            Form("Cor_SmT%01d_sm%03d_rpc%03d_Ch%03d_S%d_Walk_px", iSmType, iSm, iRpc, iCh, 1 - iSide));
          for (Int_t iBin = 0; iBin < fhCorWalk[iDetIndx][iCh][iSide]->GetNbinsX(); iBin++)
            fhCorWalk[iDetIndx][iCh][iSide]->SetBinContent(iBin + 1, hTmp->GetBinContent(iBin + 1));
        }
      }
    }
  }

  int iNbTypes = fDigiBdfPar->GetNbSmTypes();
  hSvel.resize(iNbTypes);
  for (int iSmType = 0; iSmType < iNbTypes; iSmType++) {
    int iNbRpc = fDigiBdfPar->GetNbRpc(iSmType);
    if (iNbRpc > 0 && fDigiBdfPar->GetNbSm(iSmType) > 0) {
      TH1* hTmp = (TH1*) gDirectory->FindObjectAny(Form("cl_SmT%01d_Svel", iSmType));
      if (hTmp == NULL) {
        TDirectory* oldir = gDirectory;
        gROOT->cd();
        LOG(warn) << Form("cl_SmT%01d_Svel not found, creating ... in %s ", iSmType, gDirectory->GetName());
        hSvel[iSmType] =
          new TH1F(Form("cl_SmT%01d_Svel", iSmType), Form("Clu Svel in SmType %d; Sm+Rpc# []; v/v_{nominal} ", iSmType),
                   fDigiBdfPar->GetNbSm(iSmType) * iNbRpc, 0, fDigiBdfPar->GetNbSm(iSmType) * iNbRpc);

        for (int i = 0; i < hSvel[iSmType]->GetNbinsX(); i++)
          hSvel[iSmType]->SetBinContent(i + 1, 1.);  // set default
        oldir->cd();
      }
      else {
        //LOG(info) << "hSvel "<< hTmp->GetName() << " located in " <<  gDirectory->GetName();
        TDirectory* oldir = gDirectory;
        gROOT->cd();
        hSvel[iSmType] = (TProfile*) hTmp->Clone();
        //LOG(info) << "hSvel "<< hSvel[iSmType]->GetName() << " cloned in " <<  gDirectory->GetName();
        oldir->cd();
      }
    }
  }
}

void CbmTofCalibrator::WriteHist(TFile* fHist)
{
  LOG(info) << "Write Cor histos to file " << fHist->GetName();
  TDirectory* oldir = gDirectory;
  fHist->cd();
  for (Int_t iDetIndx = 0; iDetIndx < fDigiBdfPar->GetNbDet(); iDetIndx++) {
    if (NULL == fhCorPos[iDetIndx]) continue;
    fhCorPos[iDetIndx]->Write();
    fhCorTOff[iDetIndx]->Write();
    fhCorTot[iDetIndx]->Write();
    fhCorTotOff[iDetIndx]->Write();

    Int_t iNbCh = (Int_t) fhCorWalk[iDetIndx].size();
    for (Int_t iCh = 0; iCh < iNbCh; iCh++) {
      for (Int_t iSide = 0; iSide < 2; iSide++) {
        //LOG(info)<<"Write " << fhCorWalk[iDetIndx][iCh][iSide]->GetName();
        if (NULL != fhCorWalk[iDetIndx][iCh][iSide])
          fhCorWalk[iDetIndx][iCh][iSide]->Write();
        else
          LOG(warn) << "Walk correction histo for DetIndx " << iDetIndx << ", Ch " << iCh << ", S " << iSide
                    << " not found";
      }
    }
  }
  for (Int_t iS = 0; iS < fDigiBdfPar->GetNbSmTypes(); iS++) {
    if (NULL != hSvel[iS]) {
      hSvel[iS]->Write();
      LOG(warn) << "Wrote " << hSvel[iS]->GetName() << " to " << gDirectory->GetName();
    }
    else {
      LOG(warn) << Form("cl_SmT%01d_Svel not found ", iS);
    }
  }
  oldir->cd();
}

void CbmTofCalibrator::HstDoublets(CbmTofTracklet* pTrk)
{
  for (Int_t iHit = 0; iHit < pTrk->GetNofHits() - 1; iHit++) {
    for (Int_t iHit1 = 0; iHit1 < pTrk->GetNofHits(); iHit1++) {
      if (iHit == iHit1) continue;
      CbmTofHit* pHit0 = pTrk->GetTofHitPointer(iHit);
      CbmTofHit* pHit1 = pTrk->GetTofHitPointer(iHit1);
      //auto iHind0=pTrk->GetTofHitIndex(iHit);
      //auto iHind1=pTrk->GetTofHitIndex(iHit+1);
      int iDind0 = fDetIdIndexMap[pTrk->GetTofDetIndex(iHit)];
      int iDind1 = fDetIdIndexMap[pTrk->GetTofDetIndex(iHit1)];
      if (pHit1->GetZ() < pHit0->GetZ()) {
        continue;
        //iHind0=pTrk->GetTofHitIndex(iHit+1);
        //iHind1=pTrk->GetTofHitIndex(iHit);
        /*
	    iDind0=fDetIdIndexMap[ pTrk->GetTofDetIndex(iHit1) ]; // numbering according to BDF
        iDind1=fDetIdIndexMap[ pTrk->GetTofDetIndex(iHit) ];
	    pHit0=pTrk->GetTofHitPointer(iHit1);
	    pHit1=pTrk->GetTofHitPointer(iHit);
	    */
      }
      int iHst = iDind0 * 100 + iDind1;
      if (NULL == fhDoubletDt[iHst]) {
        // create histograms
        TDirectory* oldir =
          gDirectory;  // <= To prevent histos from being sucked in by the param file of the TRootManager!
        gROOT->cd();   // <= To prevent histos from being sucked in by the param file of the TRootManager !
        TString hNameDt = Form("hDoubletDt_%02d%02d", iDind0, iDind1);
        LOG(info) << Form("Book histo %lu %s, Addrs 0x%08x, 0x%08x ", fhDoubletDt.size(), hNameDt.Data(),
                          pTrk->GetTofDetIndex(iHit), pTrk->GetTofDetIndex(iHit + 1));
        TH1D* pHstDt      = new TH1D(hNameDt, Form("%s; #Delta t (ns); cts", hNameDt.Data()), 100, -5., 5.);
        fhDoubletDt[iHst] = pHstDt;
        TString hNameDd   = Form("hDoubletDd_%02d%02d", iDind0, iDind1);
        TH1D* pHstDd      = new TH1D(hNameDd, Form("%s; #Delta D (cm); cts", hNameDd.Data()), 200, 0., 200.);
        fhDoubletDd[iHst] = pHstDd;
        TString hNameV    = Form("hDoubletV_%02d%02d", iDind0, iDind1);
        TH1D* pHstV       = new TH1D(hNameV, Form("%s; v (cm/ns); cts", hNameV.Data()), 100, 0., 100.);
        fhDoubletV[iHst]  = pHstV;
        oldir->cd();
      }
      // Fill Histograms
      double dDt = pHit1->GetTime() - pHit0->GetTime();
      double dDd = pTrk->Dist3D(pHit1, pHit0);
      fhDoubletDt[iHst]->Fill(dDt);
      fhDoubletDd[iHst]->Fill(dDd);
      fhDoubletV[iHst]->Fill(dDd / dDt);
    }
  }
}

double* CbmTofCalibrator::find_tofedge(const char* hname, double dThr, double dLen)
{
  TH1* h1               = (TH1*) gROOT->FindObjectAny(hname);
  static double dRes[2] = {2 * 0};
  dRes[0]               = 0.;
  if (NULL != h1) {
    /*
	LOG(debug) << "Inspect " << h1->GetName() << ", entries " << h1->GetNbinsX() 
	     << ", Thr " << dThr << ", Len " << dLen << endl;
	     */

    const int iMaxInt = 3;
    int iMax          = h1->GetMaximumBin();
    int iLow          = TMath::Max(iMax - iMaxInt, 1);
    int iHigh         = TMath::Min(iMax + iMaxInt, h1->GetNbinsX());
    double dMax       = 0.;
    int nMax          = 0;
    for (int i = iLow; i < iHigh; i++) {
      dMax += h1->GetBinContent(i);
      nMax++;
    }
    dMax /= nMax;

    // determine average noise and RMS per bin
    double dNoise  = 0;
    double dNoise2 = 0;
    double dRMS    = 0;
    int iNbins     = 0;
    for (int iBin = 0; iBin < h1->GetNbinsX() / 4; iBin++) {
      if (h1->GetBinContent(iBin + 1) > 0 || iNbins > 0) {  // start counting from first bin with entries
        dNoise += h1->GetBinContent(iBin + 1);
        dNoise2 += h1->GetBinContent(iBin + 1) * h1->GetBinContent(iBin + 1);
        iNbins++;
      }
    }
    if (iNbins > 0) {
      dNoise /= iNbins;
      dNoise2 /= iNbins;
      dRMS = TMath::Sqrt(dNoise2 - dNoise * dNoise);
    }
    else {
      dNoise = h1->GetMaximum() * 0.1;
      LOG(warn) << h1->GetName() << ": Noise level could not be determined, init to " << dNoise;
    }
    double dLev = dNoise + (dMax - dNoise) * dThr;
    dLev        = TMath::Max(dNoise + 3. * dRMS, dLev);
    if (dLev == 0) {
      LOG(warn) << "Invalid threshold level " << dLev;
      return dRes;
    }
    int iBl     = -1;
    double xLow = h1->GetBinCenter(1);

    for (int iBin = 1; iBin < h1->GetNbinsX() - dLen; iBin++) {
      int iLen = 0;
      for (; iLen < dLen; iLen++) {
        LOG(debug) << h1->GetName() << ": Lev " << dLev << ",  Bin " << iBin << ", Len " << iLen << ", cont "
                   << h1->GetBinContent(iBin + iLen);
        if (h1->GetBinContent(iBin + iLen) < dLev) break;
      }
      if (iLen == dLen) {
        if (iBin == 1) {
          LOG(warn) << "find_tofedge: " << h1->GetName() << ",  Lvl1 " << dLev;
          iBin--;
          dLev *= 2;
          continue;
        }
        iBl  = iBin;
        xLow = h1->GetBinCenter(iBin);
        if (iBin == 1) {  // not active since did not work, case already caught above
          LOG(warn) << GetName() << ": " << h1->GetName() << " Lvl1 " << dLev;
          xLow *= 2;
        }
        break;
      }
    }
    dRes[0] = xLow;
    LOG(info) << h1->GetName() << " with Thr " << dLev << ", Noise " << dNoise << ", RMS " << dRMS << " at iBl " << iBl
              << ", TOff " << dRes[0];
  }
  return dRes;
}

double* CbmTofCalibrator::find_tofedge(const char* hname)
{
  double dThr = 0.1;
  double dLen = 3.;
  return find_tofedge(hname, dThr, dLen);
}

double CbmTofCalibrator::CalcChi2(TH1* h1, TH1* h2, int i)
{
  double chi2         = 0.;
  double nBin         = 0.;
  double nEntries1    = h1->GetEntries();
  double nEntries2    = h2->GetEntries();
  const int CompRange = (int) h1->GetNbinsX() / 4;
  for (int iBin1 = CompRange; iBin1 < h1->GetNbinsX() - CompRange; iBin1++) {
    double diff = h1->GetBinContent(iBin1) / nEntries1 - h2->GetBinContent(iBin1 + i) / nEntries2;
    chi2 += diff * diff;
    nBin++;
  }
  chi2 /= nBin;
  return chi2;
}


Double_t CbmTofCalibrator::f1_tedge(double* x, double* par)
{
  double xx = x[0];
  //double wx    = par[0] + par[1] * TMath::Exp(xx*par[2]);
  double wx = par[0] + par[1] * (1. + TMath::Erf((xx - par[2]) / par[3]));

  return wx;
}

double* CbmTofCalibrator::fit_tofedge(const char* hname, Double_t TMax, Double_t dThr)
{
  TH1* h1;
  static double res[10] = {10 * 0.};
  double err[10];
  h1 = (TH1*) gROOT->FindObjectAny(hname);
  if (NULL != h1) {
    const double MinCts = 1000.;
    res[2]              = 0.;
    if (h1->GetEntries() < MinCts) {
      LOG(warn) << h1->GetName() << ": too few entries for edgefit " << h1->GetEntries();
      return &res[2];
    }

    TAxis* xaxis   = h1->GetXaxis();
    Double_t Tmin  = xaxis->GetXmin();
    Double_t Tmax  = xaxis->GetXmax();
    Double_t dXmax = Tmax;
    if (TMax == 0.)
      TMax = Tmax;
    else
      Tmax = TMax - 1.;
    Tmax               = TMath::Max(Tmax, Tmin + 0.3);
    TFitResultPtr fRes = h1->Fit("pol0", "SQM", "", Tmin, Tmax);
    TF1* f0            = h1->GetFunction("pol0");
    if (fRes != 0 && f0 != NULL && (gMinuit->fCstatu.Contains("OK") || gMinuit->fCstatu.Contains("CONVERGED"))) {
      //double dBackground=fRes->Parameter(0) ;
      double dBackground = f0->GetParameter(0);
      //cout << h1->GetName() << ":  av background rate \t" <<  dBackground<< endl;
      TF1* f1 = new TF1("TEdge", f1_tedge, Tmin, dXmax, 4);
      if (TMax != 0.) {
        f1->SetParameters(dBackground, 1000., 0., 0.1);
        f1->SetParLimits(0, dBackground, dBackground);  //fix background
      }
      f1->SetParLimits(2, -0.5, 0.5);
      f1->SetParLimits(3, 0.05, 0.5);

      h1->Fit("TEdge", "SQM", "", Tmin, TMax);
      res[9] = f1->GetChisquare();

      for (int i = 0; i < 4; i++) {
        res[i] = f1->GetParameter(i);
        err[i] = f1->GetParError(i);
      }

      if (res[1] < 0.) {
        LOG(warn) << h1->GetName() << "fitted wrong sign of step function ";
        res[0] = h1->GetBinCenter(h1->GetNbinsX());
        return &res[0];
      }
      // determine toff at 10*background
      /*
      double dToff=0.;
      if (res[1] !=0. && res[2] != 0.) dToff=TMath::Log(10*dBackground/res[1])/res[2];
      LOG(info) << "TEdge Fit of " << hname 
                << " in [" << Tmin <<","<< TMax <<"]"
		            << " ended with chi2 = " << res[9]
		            << ", Toff " << dToff
                << Form(", noise level %7.2f +/- %5.2f, e0 %7.2f "
                   "+/- %5.2f, exp = %7.2f +/- %5.2f",
                   res[0], err[0], res[1], err[1], res[2], err[2]);
      res[0]=dToff;
      */
      // FIXME: code duplication from find_tofedge
      const int iMaxInt = 3;
      int iMax          = h1->GetMaximumBin();
      int iLow          = TMath::Max(iMax - iMaxInt, 1);
      int iHigh         = TMath::Min(iMax + iMaxInt, h1->GetNbinsX());
      double dMax       = 0.;
      int nMax          = 0;
      for (int i = iLow; i < iHigh; i++) {
        dMax += h1->GetBinContent(i);
        nMax++;
      }
      dMax /= nMax;
      double dLev = dThr * dMax;
      dLev        = dBackground + (dMax - dBackground) * 0.1;

      double dToff = f1->GetX(dLev, Tmin, dXmax);

      LOG(info) << "TEdge Fit of " << hname << " in [" << Tmin << "," << TMax << "/" << dXmax << "]"
                << ": chi2 " << res[9]
                << Form(
                     ", noise %6.2f +/-%4.2f, norm %6.2f +/-%4.2f, mean %6.2f +/-%5.3f, wid %6.2f +/-%5.3f -> %6.3f ",
                     res[0], err[0], res[1], err[1], res[2], err[2], res[3], err[3], dToff);
      res[2] = dToff;
    }
    else {
      LOG(warn) << "pol0 fit failed, Minuit " << gMinuit->fCstatu << ", fRes " << fRes << ", "
                << ", f0 " << f0;
    }
  }
  return &res[2];
}

double* CbmTofCalibrator::fit_tofedge(const char* hname)
{
  Double_t Tmax = 0.3;
  return fit_tofedge(hname, Tmax, fTofClusterizer->GetEdgeThr());
}

double CbmTofCalibrator::TruncatedMeanY(TH2* h2, double dRmsLim)
{
  double dMeanIn = h2->ProjectionY()->GetMean();
  double dRmsIn  = h2->ProjectionY()->GetRMS();
  const int Nx   = h2->GetNbinsX();

  int Ntru = 0;
  TH1* h1[Nx];
  double dMean  = 0.;
  double dMean2 = 0.;
  double dNorm  = 0.;
  double dRms   = 0.;
  for (int i = 0; i < Nx; i++) {
    h1[i] = h2->ProjectionY(Form("%s_py%d", h2->GetName(), i), i + 1, i + 1);
    if (TMath::Abs(dMeanIn - h1[i]->GetMean()) < dRmsLim * dRmsIn) {
      dMean += h1[i]->GetMean() * h1[i]->GetEntries();
      dMean2 += h1[i]->GetMean() * h1[i]->GetMean() * h1[i]->GetEntries();
      dNorm += h1[i]->GetEntries();
      Ntru++;
    }
  }
  if (Ntru > 0) {
    dMean /= dNorm;
    dMean2 /= dNorm;
    if (dMean2 > dMean * dMean) {
      dRms = TMath::Sqrt(dMean2 - dMean * dMean);
    }
    else {
      LOG(error) << h2->GetName() << " Invalid Rms calculation: " << dMean2 << ", " << dMean * dMean;
      return dMean;
    }
    double dMean1 = dMean;
    dMean         = 0.;
    dMean2        = 0.;
    dNorm         = 0.;
    Ntru          = 0;
    for (int j = 0; j < Nx; j++) {
      if (TMath::Abs(dMean1 - h1[j]->GetMean()) < dRmsLim * dRms) {
        dMean += h1[j]->GetMean() * h1[j]->GetEntries();
        dMean2 += h1[j]->GetMean() * h1[j]->GetMean() * h1[j]->GetEntries();
        dNorm += h1[j]->GetEntries();
        Ntru++;
      }
    }
    if (Ntru > 0) {
      dMean /= dNorm;
    }
  }
  LOG(info) << h2->GetName() << ": TruncY " << dMean << "( " << Ntru << ")"
            << ", full " << dMeanIn << ", RMS " << dRms;
  return dMean;
}

ClassImp(CbmTofCalibrator)
