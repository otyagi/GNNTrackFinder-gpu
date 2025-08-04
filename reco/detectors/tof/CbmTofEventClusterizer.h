/* Copyright (C) 2013-2021 PI-UHd/GSI, Heidelberg/Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Norbert Herrmann [committer] */

/** @file CbmTofTestBeamClusterizer.h
 ** @author nh adopted from
 ** @author Pierre-Alain Loizeau <loizeau@physi.uni-heidelberg.de>
 ** @date 23.08.2013
 **/
/** @class CbmTofSimpClusterizer
 ** @brief Simple Cluster building and hit producing for CBM ToF using Digis as input
 ** @author Pierre-Alain Loizeau <loizeau@physi.uni-heidelberg.de>
 ** @version 1.0
 **/
#ifndef CBMTOFEVENTCLUSTERIZER_H
#define CBMTOFEVENTCLUSTERIZER_H 1

// TOF Classes and includes
// Input/Output
//class CbmTofPoint;
class CbmTofHit;
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
class CbmTofCalibrator;
class CbmTsEventHeader;
class CbmTimeSlice;

#include "CbmMatch.h"
#include "CbmTofAddress.h"  // in cbmdata/tof
#include "CbmTofDigi.h"
#include "TGeoManager.h"
class TimesliceMetaData;
class TTofCalibData;
class TTrbHeader;
class FairEventHeader;

// FAIR classes and includes
#include "FairTask.h"

// ROOT Classes and includes
class TClonesArray;
class TF1;
class TFile;
class TH1;
class TH2;
class TH3;
class TProfile;
class TString;
#include "TTimeStamp.h"

// C++ Classes and includes
#include <list>
#include <map>
#include <vector>

class CbmTofEventClusterizer : public FairTask {

  friend class CbmTofAnaTestbeam;

 public:
  inline static CbmTofEventClusterizer* Instance() { return fInstance; }

  /**
       ** @brief Constructor.
       **/
  CbmTofEventClusterizer();

  /**
       ** @brief Constructor.
       **/
  CbmTofEventClusterizer(const char* name, Int_t verbose = 1, Bool_t writeDataInOut = kTRUE);
  /**
       ** @brief Destructor.
       **/
  virtual ~CbmTofEventClusterizer();

  /**
       ** @brief Inherited from FairTask.
       **/
  virtual InitStatus Init();

  /**
       ** @brief Inherited from FairTask.
       **/
  virtual void SetParContainers();

  /**
       ** @brief Inherited from FairTask.
       **/
  virtual void Exec(Option_t* option);
  virtual void ExecEvent(Option_t* option, CbmEvent* tEvent = NULL);

  /**
       ** @brief Inherited from FairTask.
       **/
  virtual void Finish();
  virtual void Finish(Double_t calMode);

  inline void SetCalMode(Int_t iMode) { fCalMode = iMode; }
  inline void SetCalSel(Int_t iSel) { fCalSel = iSel; }
  inline void SetCalSmType(Int_t iCalSmType) { fCalSmAddr = CbmTofAddress::GetUniqueAddress(0, 0, 0, 0, iCalSmType); }
  inline void SetCalSmAddr(Int_t iCalSmAddr) { fCalSmAddr = iCalSmAddr; }
  inline void SetCalRpc(Int_t iCalRpc)
  {
    if (iCalRpc != 0) {
      Int_t iSign = iCalRpc / TMath::Abs(iCalRpc);
      iCalRpc *= iSign;  // always a positive number
      Int_t iRpc = iCalRpc % 10;
      iCalRpc    = (iCalRpc - iRpc) / 10;
      Int_t iSm  = iCalRpc % 10;
      iCalRpc    = (iCalRpc - iSm) / 10;
      if (fIdMode == 1) iRpc = 0;  // ignore RPC number
      fCalSmAddr = iSign * CbmTofAddress::GetUniqueAddress(iSm, iRpc, 0, 0, iCalRpc);
    }
    else {
      fCalSmAddr = 0;
    }
  }
  inline void SetCaldXdYMax(Double_t dCaldXdYMax) { fdCaldXdYMax = dCaldXdYMax; }
  inline void SetCalCluMulMax(Int_t ival) { fiCluMulMax = ival; }
  inline void SetTRefId(Int_t Id) { fTRefMode = Id; }
  inline void SetIdMode(Int_t Id) { fIdMode = Id; }
  inline void SetDutId(Int_t Id) { fDutId = Id; }
  inline void SetDutSm(Int_t Id) { fDutSm = Id; }
  inline void SetDutRpc(Int_t Id) { fDutRpc = Id; }
  inline void SetSelId(Int_t Id) { fSelId = Id; }
  inline void SetSelSm(Int_t Id) { fSelSm = Id; }
  inline void SetSelRpc(Int_t Id) { fSelRpc = Id; }
  inline void SetBeamRefId(Int_t Id) { fiBeamRefType = Id; }
  inline void SetBeamRefSm(Int_t Id) { fiBeamRefSm = Id; }
  inline void SetBeamRefDet(Int_t Id) { fiBeamRefDet = Id; }
  inline void SetBeamRefMulMax(Int_t Id) { fiBeamRefMulMax = Id; }
  inline void SetBeamAddRefMul(Int_t ival) { fiBeamAddRefMul = ival; }
  inline void SetTRefDifMax(Double_t val) { fTRefDifMax = val; }
  inline void SetdTRefMax(Double_t val) { fdTRefMax = val; }
  inline void PosYMaxScal(Double_t val) { fPosYMaxScal = val; }
  inline void SetTotMax(Double_t val) { fTotMax = val; }
  inline void SetTotMin(Double_t val) { fTotMin = val; }
  inline double GetTotMax() { return fTotMax; }
  inline double GetTotMin() { return fTotMin; }
  inline void SetTotMean(Double_t val) { fTotMean = val; }
  inline void SetDelTofMax(Double_t val) { fdDelTofMax = val; }
  inline void SetTotPreRange(Double_t val) { fTotPreRange = val; }
  inline void SetMaxTimeDist(Double_t val) { fMaxTimeDist = val; }
  inline void SetChannelDeadtime(Double_t val) { fdChannelDeadtime = val; }
  inline void SetMemoryTime(Double_t val) { fdMemoryTime = val; }
  inline void SetYFitMin(Double_t val) { fdYFitMin = val; }
  inline void SetToDAv(Double_t val) { fdToDAv = val; }
  inline void SetSel2Id(Int_t ival) { fSel2Id = ival; }
  inline void SetSel2Sm(Int_t ival) { fSel2Sm = ival; }
  inline void SetSel2Rpc(Int_t ival) { fSel2Rpc = ival; }
  inline void SetSel2MulMax(Int_t Id) { fSel2MulMax = Id; }
  inline void SetCluSizeMin(Int_t iVal) { fiCluSizeMin = iVal; }
  inline int GetCluSizeMin() { return fiCluSizeMin; }
  inline void SetNbCalHitMin(Int_t iVal) { fNbCalHitMin = iVal; }
  inline int GetNbCalHitMin() { return fNbCalHitMin; }

  inline void SetOutHstFileName(TString OutHstFileName) { fOutHstFileName = OutHstFileName; }
  inline void SetCalParFileName(TString CalParFileName) { fCalParFileName = CalParFileName; }
  inline TString GetCalParFileName() { return fCalParFileName; }

  inline void SetEnableMatchPosScaling(Bool_t bval) { fEnableMatchPosScaling = bval; }
  inline void SetEnableAvWalk(Bool_t bval) { fEnableAvWalk = bval; }
  inline void SetPs2Ns(Bool_t bval) { fbPs2Ns = bval; }
  inline Double_t GetStartAnalysisTime() { return fdStartAnalysisTime; }
  inline int GetNbHits() { return fiNbHits; }
  CbmTofHit* GetHitPointer(int iHit);
  CbmMatch* GetMatchPointer(int iHit);
  CbmMatch* GetMatchIndexPointer(int idx);
  inline double GetTotMean() { return fTotMean; }
  inline int GetBeamAddr() { return fiBeamRefAddr; }
  double GetLocalX(CbmTofHit* pHit);
  double GetLocalY(CbmTofHit* pHit);
  void MasterToLocal(const Int_t CellId, const Double_t* master, Double_t* local);

  //static Double_t  f1_xboxe(double *x, double *par); // Fit function
  virtual void fit_ybox(const char* hname);                    // Fit
  virtual void fit_ybox(TH1* h, Double_t dy);                  // Fit
  virtual void fit_ybox(TH1* h, Double_t dy, Double_t* fpar);  // Fit
  virtual double* find_yedges(const char* hname, Double_t dThr, Double_t dLen);
  virtual void CheckLHMemory();  // Check consistency of stored last hits
  virtual void CleanLHMemory();  // Cleanup
  virtual Bool_t AddNextChan(Int_t iSmType, Int_t iSm, Int_t iRpc, Int_t iLastChan, Double_t dLastPosX,
                             Double_t dLastPosY, Double_t dLastTime,
                             Double_t dLastTot);  // needed for time based data
  virtual void LH_store(Int_t iSmType, Int_t iSm, Int_t iRpc, Int_t iChm, CbmTofHit* pHit);

  void SwapChannelSides(Bool_t bSwap) { fbSwapChannelSides = bSwap; }
  void SetFileIndex(Int_t iIndex) { fiFileIndex = iIndex; }
  void SetWriteDigisInOut(Bool_t bDigis) { fbWriteDigisInOut = bDigis; }
  void SetWriteHitsInOut(Bool_t bHits) { fbWriteHitsInOut = bHits; }
  void SetAlternativeBranchNames(Bool_t bNames) { fbAlternativeBranchNames = bNames; }
  void SetDeadStrips(Int_t iDet, UInt_t ival);

  inline void SetEdgeThr(double val) { fdEdgeThr = val; }
  inline void SetEdgeLen(double val) { fdEdgeLen = val; }
  inline void SetEdgeTbias(double val) { fdEdgeTbias = val; }
  inline void SetEdgeFrange(double val) { fdEdgeFrange = val; }
  inline double GetEdgeThr() { return fdEdgeThr; }
  inline double GetEdgeLen() { return fdEdgeLen; }
  inline double GetEdgeTbias() { return fdEdgeTbias; }
  inline double GetEdgeFrange() { return fdEdgeFrange; }
  inline void SetModifySigvel(Double_t val) { fdModifySigvel = val; }
  inline double GetModifySigvel() { return fdModifySigvel; }

 protected:
 private:
  static CbmTofEventClusterizer* fInstance;
  /**
       ** @brief Copy constructor.
       **/
  CbmTofEventClusterizer(const CbmTofEventClusterizer&);
  /**
       ** @brief Copy operator.
       **/
  CbmTofEventClusterizer& operator=(const CbmTofEventClusterizer&);

  // Functions common for all clusters approximations
  /**
       ** @brief Recover pointer on input TClonesArray: TofPoints, TofDigis...
       **/
  Bool_t RegisterInputs();
  /**
       ** @brief Create and register output TClonesArray of Tof Hits.
       **/
  Bool_t RegisterOutputs();
  /**
       ** @brief Initialize other parameters not included in parameter classes.
       **/
  Bool_t InitParameters();
  /**
       ** @brief Initialize other parameters not included in parameter classes.
       **/
  Bool_t InitCalibParameter();
  /**
       ** @brief Load the geometry: for now just resizing the Digis temporary vectors
       **/
  Bool_t LoadGeometry();
  /**
       ** @brief Delete the geometry related arrays: for now just clearing the Digis temporary vectors
       **/
  Bool_t DeleteGeometry();

  // Histogramming functions
  Bool_t CreateHistos();
  Bool_t FillHistos(CbmEvent* tEvent);
  Bool_t WriteHistos();
  Bool_t DeleteHistos();

  /**
       ** @brief Build clusters out of ToF Digis and store the resulting info in a TofHit.
       **/
  Bool_t BuildClusters();
  Bool_t MergeClusters();
  Bool_t BuildHits();
  Bool_t CalibRawDigis();
  Bool_t InspectRawDigis();
  Bool_t CalibHits();

  // ToF geometry variables
  CbmTofGeoHandler* fGeoHandler;
  CbmTofDetectorId* fTofId;
  CbmTofDigiPar* fDigiPar;
  CbmTofCell* fChannelInfo;
  CbmTofDigiBdfPar* fDigiBdfPar;

  TTrbHeader* fTrbHeader;
  FairEventHeader* fEvtHeader;
  const CbmTsEventHeader* fTsHeader;
  const CbmTimeSlice* fTimeSlice;

  // Input variables
  const std::vector<CbmMatch>* fTofDigiPointMatches = nullptr;  // TOF MC point matches
  //TClonesArray          * fTofDigisColl;     // TOF Digis
  std::vector<CbmTofDigi> fTofDigiVec{};  //! TOF Digis
  CbmDigiManager* fDigiMan;               // TOF Input Digis
  TClonesArray* fEventsColl;              // CBMEvents (time based)

  // Output variables
  Bool_t fbWriteHitsInOut;
  Bool_t fbWriteDigisInOut;
  std::vector<CbmTofDigi>* fTofCalDigiVec = nullptr;  //! // Calibrated TOF Digis
  TClonesArray* fTofHitsColl;                         // TOF hits
  TClonesArray* fTofDigiMatchColl;                    // TOF Digi Links
  //TClonesArray          * fTofCalDigisCollOut;   // Calibrated TOF Digis
  std::vector<CbmTofDigi>* fTofCalDigiVecOut = nullptr;      //! // Calibrated TOF Digis
  TClonesArray* fTofHitsCollOut;                             // TOF hits
  TClonesArray* fTofDigiMatchCollOut;                        // TOF Digi Links
  std::vector<CbmMatch>* fTofDigiPointMatchesOut = nullptr;  // TOF Digi MC Point Matches
  Int_t fiNbHits;                                            // Index of the CbmTofHit TClonesArray

  // Generic
  Int_t fVerbose;
  Int_t fiNbSkip1 = 0;
  Int_t fiNbSkip2 = 0;


  // Intermediate storage variables
  std::vector<std::vector<std::vector<std::vector<CbmTofDigi*>>>> fStorDigi;  //[nbType][nbSm*nbRpc][nbCh][nDigis]
  std::vector<std::vector<std::vector<std::vector<Int_t>>>> fStorDigiInd;     //[nbType][nbSm*nbRpc][nbCh][nDigis]
  std::vector<Int_t> vDigiIndRef;

  std::vector<std::vector<std::vector<Int_t>>> fviClusterMul;   //[nbType][nbSm][nbRpc]
  std::vector<std::vector<std::vector<Int_t>>> fviClusterSize;  //[nbType][nbRpc][nClusters]
  std::vector<std::vector<std::vector<Int_t>>> fviTrkMul;       //[nbType][nbRpc][nClusters]
  std::vector<std::vector<std::vector<Double_t>>> fvdX;         //[nbType][nbRpc][nClusters]
  std::vector<std::vector<std::vector<Double_t>>> fvdY;         //[nbType][nbRpc][nClusters]
  std::vector<std::vector<std::vector<Double_t>>> fvdDifX;      //[nbType][nbRpc][nClusters]
  std::vector<std::vector<std::vector<Double_t>>> fvdDifY;      //[nbType][nbRpc][nClusters]
  std::vector<std::vector<std::vector<Double_t>>> fvdDifCh;     //[nbType][nbRpc][nClusters]

  CbmTofCalibrator* fTofCalibrator;

  // Histograms
  TH1* fhClustBuildTime;
  TH2* fhClustHitsDigi;
  TH1* fhHitsPerTracks;
  TH1* fhPtsPerHit;
  TH1* fhTimeResSingHits;
  TH2* fhTimeResSingHitsB;
  TH2* fhTimePtVsHits;
  TH1* fhClusterSize;
  TH2* fhClusterSizeType;
  TH1* fhTrackMul;
  TH2* fhClusterSizeMulti;
  TH2* fhTrk1MulPos;
  TH2* fhHiTrkMulPos;
  TH2* fhAllTrkMulPos;
  TH2* fhMultiTrkProbPos;
  TH1* fhDigSpacDifClust;
  TH1* fhDigTimeDifClust;
  TH2* fhDigDistClust;
  TH2* fhClustSizeDifX;
  TH2* fhClustSizeDifY;
  TH2* fhChDifDifX;
  TH2* fhChDifDifY;
  TH2* fhCluMulCorDutSel;
  TH2* fhEvCluMul;

  std::vector<TH2*> fhRpcDigiCor;                            //[nbDet]
  std::vector<TH2*> fhRpcDigiMul;                            //[nbDet]
  std::vector<TH2*> fhRpcDigiStatus;                         //[nbDet]
  std::vector<TH2*> fhRpcDigiDTLD;                           //[nbDet]
  std::vector<TH2*> fhRpcDigiDTFD;                           //[nbDet]
  std::vector<TH2*> fhRpcDigiDTMul;                          //[nbDet]
  std::vector<TH1*> fhRpcDigiRate;                           //[nbDet]
  std::vector<TH2*> fhRpcDigiTotLeft;                        //[nbDet]
  std::vector<TH2*> fhRpcDigiTotRight;                       //[nbDet]
  std::vector<TH2*> fhRpcDigiTotDiff;                        //[nbDet]
  std::vector<TH2*> fhRpcDigiTotMap;                         //[nbDet]
  std::vector<TH1*> fhRpcCluMul;                             //[nbDet]
  std::vector<TH1*> fhRpcCluRate;                            //[nbDet]
  std::vector<TH1*> fhRpcCluRate10s;                         //[nbDet]
  std::vector<TH2*> fhRpcCluPosition;                        //[nbDet]
  std::vector<TH2*> fhRpcCluPos;                             //[nbDet]
  std::vector<TProfile*> fhRpcCluPositionEvol;               //[nbDet]
  std::vector<TProfile*> fhRpcCluTimeEvol;                   //[nbDet]
  std::vector<TH2*> fhRpcCluDelPos;                          //[nbDet]
  std::vector<TH2*> fhRpcCluDelMatPos;                       //[nbDet]
  std::vector<TH2*> fhRpcCluTOff;                            //[nbDet]
  std::vector<TH2*> fhRpcCluDelTOff;                         //[nbDet]
  std::vector<TH2*> fhRpcCluDelMatTOff;                      //[nbDet]
  std::vector<TH2*> fhRpcCluTrms;                            //[nbDet]
  std::vector<TH2*> fhRpcCluTot;                             //[nbDet]
  std::vector<TH2*> fhRpcCluSize;                            //[nbDet]
  std::vector<TH2*> fhRpcCluAvWalk;                          //[nbDet]
  std::vector<TH2*> fhRpcCluAvLnWalk;                        //[nbDet]
  std::vector<std::vector<std::vector<TH2*>>> fhRpcCluWalk;  // [nbDet][nbCh][nSide]
  std::vector<TH2*> fhSmCluPosition;                         //[nbSmTypes]
  std::vector<TH2*> fhSmCluTOff;
  std::vector<TProfile*> fhSmCluSvel;
  std::vector<std::vector<TProfile*>> fhSmCluFpar;
  std::vector<TH1*> fhRpcDTLastHits;          //[nbDet]
  std::vector<TH1*> fhRpcDTLastHits_Tot;      //[nbDet]
  std::vector<TH1*> fhRpcDTLastHits_CluSize;  //[nbDet]

  std::vector<std::vector<TH1*>> fhTRpcCluMul;                             //[nbDet][nbSel]
  std::vector<std::vector<TH2*>> fhTRpcCluPosition;                        //[nbDet][nbSel]
  std::vector<std::vector<TH2*>> fhTRpcCluTOff;                            //[nbDet] [nbSel]
  std::vector<std::vector<TH2*>> fhTRpcCluTofOff;                          //[nbDet] [nbSel]
  std::vector<std::vector<TH2*>> fhTRpcCluTot;                             // [nbDet][nbSel]
  std::vector<std::vector<TH2*>> fhTRpcCluSize;                            // [nbDet][nbSel]
  std::vector<std::vector<TH2*>> fhTRpcCluAvWalk;                          // [nbDet][nbSel]
  std::vector<std::vector<TH2*>> fhTRpcCluDelTof;                          // [nbDet][nbSel]
  std::vector<std::vector<TH2*>> fhTRpcCludXdY;                            // [nbDet][nbSel]
  std::vector<std::vector<TH2*>> fhTRpcCluQASY;                            // [nbDet][nbSel]
  std::vector<std::vector<std::vector<std::vector<TH2*>>>> fhTRpcCluWalk;  // [nbDet][nbSel][nbCh][nSide]
  std::vector<std::vector<TH3*>> fhTRpcCluWalk2;                           // [nbDet][nbSel]
  std::vector<std::vector<TH3*>> fhTRpcCluQ2DT;                            // [nbDet][nbSel]

  std::vector<std::vector<TH2*>> fhTSmCluPosition;  //[nbSmTypes][nbSel]
  std::vector<std::vector<TH2*>> fhTSmCluTOff;      //[nbSmTypes][nbSel]
  std::vector<std::vector<TH2*>> fhTSmCluTRun;      //[nbSmTypes][nbSel]
  std::vector<std::vector<TH2*>> fhTRpcCluTOffDTLastHits;
  std::vector<std::vector<TH2*>> fhTRpcCluTotDTLastHits;
  std::vector<std::vector<TH2*>> fhTRpcCluSizeDTLastHits;
  std::vector<std::vector<TH2*>> fhTRpcCluMemMulDTLastHits;

  std::vector<TH1*> fhSeldT;  //[nbSel]

  std::vector<std::vector<std::vector<std::vector<Double_t>>>> fvCPDelTof;   //[nSMT][nRpc][nbClDelTofBinX][nbSel]
  std::vector<std::vector<std::vector<std::vector<Double_t>>>> fvCPTOff;     //[nSMT][nRpc][nCh][nbSide]
  std::vector<std::vector<std::vector<std::vector<Double_t>>>> fvCPTotGain;  //[nSMT][nRpc][nCh][nbSide]
  std::vector<std::vector<std::vector<std::vector<Double_t>>>> fvCPTotOff;   //[nSMT][nRpc][nCh][nbSide]
  std::vector<std::vector<std::vector<std::vector<std::vector<Double_t>>>>>
    fvCPWalk;                                                 //[nSMT][nRpc][nCh][nbSide][nbWalkBins]
  std::vector<std::vector<std::vector<Double_t>>> fvCPTOffY;  //[nSMT][nRpc][nBin]
  std::vector<std::vector<Double_t>> fvCPTOffYBinWidth;       //[nSMT][nRpc]
  std::vector<std::vector<Double_t>> fvCPTOffYRange;          //[nSMT][nRpc]

  std::vector<std::vector<std::vector<std::vector<std::list<CbmTofHit*>>>>> fvLastHits;  //[nSMT[nSm][nRpc][nCh][NHits]
  std::vector<UInt_t> fvDeadStrips;                                                      //[nbDet]
  std::vector<std::vector<Double_t>> fvTimeLastDigi;                                     //[nbDet][nChannel*2]
  std::vector<std::vector<Double_t>> fvTimeFirstDigi;                                    //[nbDet][nChannel*2]
  std::vector<std::vector<Double_t>> fvMulDigi;                                          //[nbDet][nChannel*2]

  // Digis quality
  Int_t fiNbSameSide;
  TH1* fhNbSameSide;
  TH1* fhNbDigiPerChan;

  // Control
  TTimeStamp fStart;
  TTimeStamp fStop;

  // Calib
  Double_t dTRef;
  Double_t fdTRefMax;
  Int_t fCalMode;
  Int_t fCalSel;
  Int_t fCalSmAddr;
  Double_t fdCaldXdYMax;
  Int_t fiCluMulMax;
  Int_t fTRefMode;
  Int_t fTRefHits;
  Int_t fIdMode;
  Int_t fDutId;
  Int_t fDutSm;
  Int_t fDutRpc;
  Int_t fDutAddr;
  Int_t fSelId;
  Int_t fSelSm;
  Int_t fSelRpc;
  Int_t fSelAddr;
  Int_t fiBeamRefType;
  Int_t fiBeamRefSm;
  Int_t fiBeamRefDet;
  Int_t fiBeamRefAddr;
  Int_t fiBeamRefMulMax;
  Int_t fiBeamAddRefMul;
  Int_t fSel2Id;
  Int_t fSel2Sm;
  Int_t fSel2Rpc;
  Int_t fSel2Addr;
  Int_t fSel2MulMax;
  Int_t fiCluSizeMin;
  Int_t fNbCalHitMin;
  std::map<UInt_t, UInt_t> fDetIdIndexMap;
  std::map<UInt_t, TGeoPhysicalNode*> fCellIdGeoMap;
  std::vector<Int_t> fviDetId;

  Double_t fPosYMaxScal;
  Double_t fTRefDifMax;
  Double_t fTotMax;
  Double_t fTotMin;
  Double_t fTotOff;
  Double_t fTotMean;
  Double_t fdDelTofMax;
  Double_t fTotPreRange;
  Double_t fMaxTimeDist;
  Double_t fdChannelDeadtime;
  Double_t fdMemoryTime;
  Double_t fdYFitMin;
  Double_t fdToDAv;

  Bool_t fEnableMatchPosScaling;
  Bool_t fEnableAvWalk;
  Bool_t fbPs2Ns;  // convert input raw digis from ps to ns

  TString fCalParFileName;  // name of the file name with Calibration Parameters
  TString fOutHstFileName;  // name of the histogram output file name with Calibration Parameters
  TFile* fCalParFile;       // pointer to Calibration Parameter file

  // Constants or setting parameters
  Int_t fiNevtBuild;
  Int_t fiMsgCnt;

  Double_t fdTOTMax;
  Double_t fdTOTMin;
  Double_t fdTTotMean;

  Double_t fdMaxTimeDist;   // Isn't this just a local variable? Why make it global and preset?!?
  Double_t fdMaxSpaceDist;  // Isn't this just a local variable? Why make it global and preset?!?
  Double_t fdEdgeThr;       // for Calibrator
  Double_t fdEdgeLen;       // for Calibrator
  Double_t fdEdgeTbias;
  Double_t fdEdgeFrange;
  Double_t fdModifySigvel;

  Double_t fdEvent;
  Double_t fdStartAnalysisTime;

  Double_t fProcessTime = 0.0;
  uint64_t fuNbDigis    = 0;
  uint64_t fuNbHits     = 0;

  Bool_t fbSwapChannelSides;
  Int_t fiOutputTreeEntry;
  Int_t fiFileIndex;
  Bool_t fbAlternativeBranchNames;

  ClassDef(CbmTofEventClusterizer, 1);
};

#endif  // CBMTOFEVENTCLUSTERIZER_H
