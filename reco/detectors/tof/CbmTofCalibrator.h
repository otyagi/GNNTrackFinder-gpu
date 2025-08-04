/* Copyright (C) 2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Norbert Herrmann, Florian Uhlig [committer] */

/** @file CbmTofCalibrator.h
 ** @author nh <N.Herrmann@gsi.de>
 ** @date 28.02.2020
 **/

#ifndef CBMTOFCALIBRATOR_H
#define CBMTOFCALIBRATOR_H 1

// TOF Classes and includes
class CbmTofDigi;
class CbmTofHit;
class CbmMatch;
class CbmEvent;
class CbmVertex;
// Geometry
class CbmTofGeoHandler;
class CbmTofDetectorId;
class CbmTofDigiPar;
class CbmTofDigiBdfPar;
class CbmTofCell;
class CbmTofFindTracks;
class CbmDigiManager;

#include "CbmDigiManager.h"
#include "CbmTofDigi.h"
#include "CbmTofEventClusterizer.h"
#include "CbmTofFindTracks.h"
#include "CbmTofHit.h"
#include "CbmTofTracklet.h"
#include "CbmTofTrackletParam.h"
#include "CbmTofTrackletTools.h"
#include "FairEventHeader.h"
#include "FairTrackParam.h"
#include "TH1.h"
#include "TH2.h"
#include "TMath.h"

#include <vector>

class TClonesArray;
class TFile;

/** @class CbmTofCalibrator
 ** @brief  contains filling and updating of calibration histos
 ** @author nh
 **/
class CbmTofCalibrator : public FairTask {

 public:
  /**   Constructor   **/
  CbmTofCalibrator();

  /**   Destructor   **/
  virtual ~CbmTofCalibrator();

  InitStatus Init();
  Bool_t InitParameters();
  Bool_t CreateCalHist();
  void FillCalHist(CbmTofHit* pHit, Int_t iOpt, CbmEvent* pEvent = NULL);
  void FillHitCalHist(CbmTofHit* pHit, Int_t iOpt, CbmEvent* pEvent = NULL, TClonesArray* tHitColl = NULL);
  void FillCalHist(CbmTofTracklet* pTrk, Int_t iOpt, CbmEvent* pEvent = NULL);
  Bool_t UpdateCalHist(Int_t iOpt);
  void ReadHist(TFile* fhFile);
  void WriteHist(TFile* fhFile);
  void HstDoublets(CbmTofTracklet* pTrk);
  double* find_tofedge(const char* hname, Double_t dThr, Double_t dLen);
  double* find_tofedge(const char* hname);
  double CalcChi2(TH1* h1, TH1* h2, int iShift);
  double* fit_tofedge(const char* hname, Double_t dTmax, Double_t dThr);
  double* fit_tofedge(const char* hname);
  static double f1_tedge(double* x, double* par);
  double TruncatedMeanY(TH2* pHst, double RmsLim = 1.);

  inline void SetR0Lim(Double_t dVal) { fdR0Lim = dVal; }
  inline void SetBeam(Bool_t bVal) { fbBeam = bVal; }

 private:
  CbmDigiManager* fDigiMan;
  CbmTofEventClusterizer* fTofClusterizer;
  CbmTofFindTracks* fTofFindTracks;
  CbmTofTrackletTools* fTrackletTools;

  const std::vector<CbmTofDigi>* fTofCalDigiVec = nullptr;

  CbmTofDigiPar* fDigiPar;
  CbmTofDigiBdfPar* fDigiBdfPar;
  CbmTofGeoHandler* fGeoHandler;
  CbmTofDetectorId* fTofId;
  TClonesArray* fTofDigiMatchColl;  // TOF Digi Links

  FairEventHeader* fEvtHeader;

  TH1* fhCalR0;
  TH1* fhCalDX0;
  TH1* fhCalDY0;

  TH1* fhCalCounterDt;
  TH1* fhCalCounterDy;
  TH1* fhCalChannelDt;
  TH1* fhCalChannelDy;

  std::vector<TH2*> fhCalTot;                                 // [nbDet]
  std::vector<TH2*> fhCalPosition;                            // [nbDet]
  std::vector<TH2*> fhCalPos;                                 // [nbDet]
  std::vector<TH2*> fhCalTOff;                                // [nbDet]
  std::vector<TH2*> fhCalTofOff;                              // [nbDet]
  std::vector<TH2*> fhCalDelPos;                              // [nbDet]
  std::vector<TH2*> fhCalDelTOff;                             // [nbDet]
  std::vector<TH2*> fhCalCluTrms;                             // [nbDet]
  std::vector<TH2*> fhCalCluSize;                             // [nbDet]
  std::vector<TH2*> fhCalWalkAv;                              // [nbDet]
  std::vector<std::vector<std::vector<TH2*>>> fhCalWalk;      // [nbDet][nbCh][nSide]
  std::vector<std::vector<std::vector<TH2*>>> fhCalDtWalk;    // [nbDet][nbCh][nSide]
  std::vector<TH3*> fhCalXYTOff;                              // [nbDet]
  std::vector<TH3*> fhCalXYTot;                               // [nbDet]
  std::vector<std::vector<std::vector<TH3*>>> fhCalTotYWalk;  // [nbDet][nbCh][nSide]
  std::vector<std::vector<std::vector<TH3*>>> fhCalTotYTOff;  // [nbDet][nbCh][nSide]

  std::vector<TH1*> fhCorPos;                             // [nbDet]
  std::vector<TH1*> fhCorTOff;                            // [nbDet]
  std::vector<TH1*> fhCorTot;                             // [nbDet]
  std::vector<TH1*> fhCorTotOff;                          // [nbDet]
  std::vector<TH1*> fhCorSvel;                            // [nbDet]
  std::vector<std::vector<std::vector<TH1*>>> fhCorWalk;  // [nbDet][nbCh][nSide]

  std::map<UInt_t, UInt_t> fDetIdIndexMap;
  std::map<int, TH1*> fhDoubletDt;
  std::map<int, TH1*> fhDoubletDd;
  std::map<int, TH1*> fhDoubletV;

  Double_t fdR0Lim = 0.;
  Bool_t fbBeam    = kFALSE;

  CbmTofCalibrator(const CbmTofCalibrator&) = delete;
  CbmTofCalibrator operator=(const CbmTofCalibrator&) = delete;

  ClassDef(CbmTofCalibrator, 1);
};

#endif /* CBMTOFCALIBRATOR_H */
