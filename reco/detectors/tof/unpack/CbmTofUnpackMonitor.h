/* Copyright (C) 2022 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer]  */

#ifndef CbmTofUnpackMonitor_H
#define CbmTofUnpackMonitor_H

#include "CbmBmonDigi.h"
#include "CbmMcbm2018TofPar.h"

#include <Rtypes.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TProfile2D.h>

#include <array>
#include <cstdint>
#include <vector>

class TCanvas;

class CbmTofUnpackMonitor {
 public:
  CbmTofUnpackMonitor();

  virtual ~CbmTofUnpackMonitor();

  /** @brief Init all required parameter informations and histograms */
  Bool_t Init(CbmMcbm2018TofPar* digiParSet);

  Bool_t CreateHistograms();
  Bool_t ResetHistograms();
  void DrawCanvases();

  Bool_t CreateBmonHistograms();
  Bool_t ResetBmonHistograms(Bool_t bResetTime);
  void DrawBmonCanvases();

  Bool_t CreateHistogramsMicroSpills();
  Bool_t ResetHistogramsMicroSpills(Bool_t bResetTime);
  void DrawCanvasesMicroSpills();

  Bool_t CreateHistogramsQFactors(Bool_t bBmon = kTRUE);
  Bool_t ResetHistogramsQFactors(Bool_t bResetTime);
  void DrawCanvasesQFactors(Bool_t bBmon = kTRUE);

  Bool_t CreateMsComponentSizeHistos(UInt_t component);
  Bool_t ResetMsComponentSizeHistos(UInt_t component);

  /** @brief Write all histograms and canvases to file */
  void Finish();

  void SetHistoFileName(TString nameIn) { fHistoFileName = nameIn; }

  inline void SetPulserTotLimits(UInt_t uMin, UInt_t uMax)
  {
    fuMinTotPulser = uMin;
    fuMaxTotPulser = uMax;
  }
  inline void SetSpillThreshold(UInt_t uCntLimit) { fuOffSpillCountLimit = uCntLimit; }
  inline void SetSpillThresholdNonPulser(UInt_t uCntLimit) { fuOffSpillCountLimitNonPulser = uCntLimit; }
  inline void SetSpillCheckInterval(Double_t dIntervalSec) { fdSpillCheckInterval = dIntervalSec; }
  void SetBmonChannelMap(std::vector<uint32_t> vChanMapIn);

  void SetLongDurationLimits(UInt_t uDurationSeconds, UInt_t uBinSize)
  {
    //fbLongHistoEnable     = kTRUE;
    fuLongHistoNbSeconds  = uDurationSeconds;
    fuLongHistoBinSizeSec = uBinSize;
  }

  void AddHistoToVector(TNamed* pointer, std::string sFolder = "")
  {
    fvpAllHistoPointers.push_back(std::pair<TNamed*, std::string>(pointer, sFolder));
  }
  std::vector<std::pair<TNamed*, std::string>> GetHistoVector() { return fvpAllHistoPointers; }

  void AddCanvasToVector(TCanvas* pointer, std::string sFolder = "")
  {
    fvpAllCanvasPointers.push_back(std::pair<TCanvas*, std::string>(pointer, sFolder));
  }
  std::vector<std::pair<TCanvas*, std::string>> GetCanvasVector() { return fvpAllCanvasPointers; }

  UInt_t GetMaxNbFlibLinks() { return kuMaxNbFlibLinks; }

  /// Fill Ms Component Size Histos
  void FillMsSize(UInt_t uMsComp, UInt_t uSize) { fvhMsSize[uMsComp]->Fill(uSize); }
  void FillMsSizeTime(UInt_t uMsComp, Double_t dTime, UInt_t uSize) { fvhMsSizeTime[uMsComp]->Fill(dTime, uSize); }


  /// Fill general histograms
  //  void FillMsCntEvo(ULong64_t MsIdx) { fhMsCntEvo->Fill(MsIdx * 1e-9); }
  void FillHitMonitoringHistos(const double_t& dMsTime, const uint32_t& uCurrCompIdx, const uint32_t& uGet4Id,
                               const uint32_t& uRawCh, const uint32_t& uRemapCh, const uint32_t& uTot);
  void FillEpochMonitoringHistos(const uint32_t& uCurrCompIdx, const uint32_t& uGet4Id, const bool& bSyncFlag,
                                 const bool& bDataLoss, const bool& bEpochLoss, const bool& bMissmMatch);
  void FillScmMonitoringHistos(const uint32_t& uCurrCompIdx, const uint32_t& uGet4Id, const uint32_t& uCh,
                               const uint32_t& uEdge, const uint32_t& uType);
  void FillSysMonitoringHistos(const uint32_t& uCurrCompIdx, const uint32_t& uGet4Id, const uint32_t& uType);
  void FillErrMonitoringHistos(const uint32_t& uCurrCompIdx, const uint32_t& uGet4Id, const uint32_t& uCh,
                               const uint32_t& uType);

  /// Fill BMon histograms
  void CheckBmonSpillLimits(const double_t& dMsTime);
  void FillHitBmonMonitoringHistos(const double_t& dMsTime, const uint32_t& uCurrCompIdx, const uint32_t& uGet4Id,
                                   const uint32_t& uTot);

  void FillErrBmonMonitoringHistos(const double_t& dMsTime, const uint32_t& uCurrCompIdx, const uint32_t& uGet4Id,
                                   const bool& bErrEvtLost);

  /** @brief Activate the BMon mode */
  void SetBmonMode(bool value) { fBmonMode = value; }

  /** @brief Read the BMon mode */
  bool GetBmonMode() { return fBmonMode; }

  /// Fill BMon Microspill histograms
  void FillHitBmonMicroSpillHistos(const double_t& dMsTime, const double_t& dTime);
  /// Finalize BMon Microspill histograms
  void FinalizeTsBmonMicroSpillHistos();

  /// Fill Microspill histograms
  void FillHitBmonQfactorHistos(const double_t& dMsTime, const double_t& dTime);
  /// Finalize Microspill histograms
  void FinalizeTsBmonQfactorHistos(uint64_t uTsTimeNs, std::vector<CbmBmonDigi>* vDigis);

  /** @brief Activate the BMon mode */
  void SetBmonMicroSpillMode(bool value) { fBmonMicroSpillMode = value; }

  /** @brief Read the Bmon mode */
  bool GetBmonMicroSpillMode() { return fBmonMicroSpillMode; }

  /** @brief Activate the BMon sCVD mode */
  void SetBmonScvdMode(bool value) { fBmonScvdMode = value; }

  /** @brief Read the BMon mode */
  bool GetBmonScvdMode() { return fBmonScvdMode; }

  /** @brief Activate the Bmon QFactor mode */
  void SetBmonQFactorMode(bool value)
  {
    fBmonQfactorsMode = value;
    if (fBmonQfactorsMode) {
      fBmonMicroSpillMode = false;
      fBmonScvdMode       = false;
    }
  }

  /** @brief Read the Bmon QFactor mode */
  bool GetBmonQFactorMode() { return fBmonQfactorsMode; }

  /** @brief Activate the Tof QFactor mode */
  void SetTofQFactorMode(bool value) { fTofQfactorsMode = value; }

  /** @brief Read the Tof QFactor mode */
  bool GetTofQFactorMode() { return fTofQfactorsMode; }

  /** @brief Activate/de-activate the internal histo serve mode */
  void SetInternalHttpMode(bool value) { fbInternalHttp = value; }

  /** @brief Read the Bmon mode */
  bool GetInternalHttpMode() { return fbInternalHttp; }

  /** @brief Set start time for evolution histos */
  void SetHistosStartTime(double_t value) { fdStartTime = value; }

  /** @brief Set start time for evolution histos */
  double_t GetHistosStartTime() { return fdStartTime; }

 private:
  TString fHistoFileName = "data/mon.tof.root";

  /// Settings from parameter file
  CbmMcbm2018TofPar* fUnpackPar = nullptr;
  /// Readout chain dimensions and mapping
  UInt_t fuNbOfComps           = 0;  //! Total number of Components in the system
  UInt_t fuNbOfGet4PerComp     = 0;  //! Max number of Get4 per component
  UInt_t fuNbOfChannelsPerGet4 = 0;  //! Number of channels per Get4, constant
  UInt_t fuNbOfChannelsPerComp = 0;  //! Number of channels per Component, recalculated
  UInt_t fuNbOfGet4InSyst      = 0;  //! Total/maximum number of Get4 in system

  /// Rate evolution histos
  Double_t fdStartTime = -1.0; /** Time of first valid hit (epoch available), used as reference for evolution plots**/
  UInt_t fuHistoryHistoSize  = 3600; /** Size in seconds of the evolution histograms **/
  double_t dFirstTsStartTime = 0;
  //Bool_t fbLongHistoEnable;
  UInt_t fuLongHistoNbSeconds  = 3600;
  UInt_t fuLongHistoBinSizeSec = 10;
  UInt_t fuLongHistoBinNb      = 1;

  /** @brief Flag if debug mode is active or not */
  bool fBmonMode           = false;
  bool fBmonScvdMode       = false;
  bool fBmonMicroSpillMode = false;
  bool fBmonQfactorsMode   = false;
  bool fTofQfactorsMode    = false;
  bool fbInternalHttp      = true;

  /// ---> Constants
  static const UInt_t kuMaxNbFlibLinks  = 32;
  static const UInt_t kuBytesPerMessage = 8;
  static const UInt_t kuNbChanBmon      = 16;
  static const UInt_t kuNbChanBmonScvd  = 20;
  static const UInt_t kuNbSpillPlots    = 5;

  /// ---> User settings: Data correction parameters
  UInt_t fuMinTotPulser                = 185;
  UInt_t fuMaxTotPulser                = 189;
  UInt_t fuOffSpillCountLimit          = 200;
  UInt_t fuOffSpillCountLimitNonPulser = 80;
  Double_t fdSpillCheckInterval        = 1.0;
  /// Map from electronics channel to BMon strip
  /// 2022 mapping: Y[0-3] on c0, Y[4-7] on c1, X[4-7] on c2, X[0-3] on c3, re-ordered in par file and par class
  /// Y not cabled for diamond but pulser there
  static constexpr UInt_t kuBmonChanMap[kuNbChanBmon] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  /// 2024 mapping: +4 channels scvd added
  static constexpr UInt_t kuBmonChanMapScvd[kuNbChanBmonScvd] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
                                                                 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
  /// Runtime values
  UInt_t fuNbChanBmon               = 0;
  std::vector<UInt_t> fuBmonChanMap = {};

  /// MS size histograms
  TH1* fvhMsSize[kuMaxNbFlibLinks];
  TProfile* fvhMsSizeTime[kuMaxNbFlibLinks];

  /// TOF system general histograms
  /// ---> Per GET4 in system
  TH2* fhGet4MessType    = nullptr;
  TH2* fhGet4EpochFlags  = nullptr;
  TH2* fhGet4ScmType     = nullptr;
  TH2* fhGet4SysMessType = nullptr;
  TH2* fhGet4ErrorsType  = nullptr;
  /// ---> Per GET4 in Component
  std::vector<TH2*> fvhCompGet4MessType = {};
  std::vector<TH2*> fvhCompGet4ChScm    = {};
  std::vector<TH2*> fvhCompGet4ChErrors = {};
  /// ---> Per raw channel in Component
  std::vector<TH1*> fvhCompRawChCount = {};
  std::vector<TH2*> fvhCompRawChRate  = {};
  std::vector<TH2*> fvhCompRawChTot   = {};
  /// ---> Per remapped (PADI) channel in Component
  std::vector<TH1*> fvhCompRemapChCount = {};
  std::vector<TH2*> fvhCompRemapChRate  = {};
  std::vector<TH2*> fvhCompRemapChTot   = {};
  /// ---> Pulser
  TH1* fhPulserChCounts = nullptr;
  TH2* fhPulserChEvo    = nullptr;

  /// BMon specific histograms
  /// ---> Channel rate plots
  std::vector<UInt_t> fvuBmonHitCntChanMs           = {};
  std::vector<UInt_t> fvuBmonErrorCntChanMs         = {};
  std::vector<UInt_t> fvuBmonEvtLostCntChanMs       = {};
  std::vector<TH1*> fvhBmonMsgCntEvoChan            = {};
  std::vector<TH2*> fvhBmonMsgCntPerMsEvoChan       = {};
  std::vector<TH1*> fvhBmonHitCntEvoChan            = {};
  std::vector<TH2*> fvhBmonHitCntPerMsEvoChan       = {};
  std::vector<TH1*> fvhBmonErrorCntEvoChan          = {};
  std::vector<TH2*> fvhBmonErrorCntPerMsEvoChan     = {};
  std::vector<TH1*> fvhBmonEvtLostCntEvoChan        = {};
  std::vector<TH2*> fvhBmonEvtLostCntPerMsEvoChan   = {};
  std::vector<TProfile*> fvhBmonErrorFractEvoChan   = {};
  std::vector<TH2*> fvhBmonErrorFractPerMsEvoChan   = {};
  std::vector<TProfile*> fvhBmonEvtLostFractEvoChan = {};
  std::vector<TH2*> fvhBmonEvtLostFractPerMsEvoChan = {};
  /// ---> Channels maps without cuts
  TH1* fhBmonCompMapAll       = nullptr;
  TH2* fhBmonCompGet4         = nullptr;
  TH1* fhBmonGet4Map          = nullptr;
  TH2* fhBmonGet4MapEvo       = nullptr;
  TH1* fhBmonChannelMapAll    = nullptr;
  TH2* fhBmonChannelTotAll    = nullptr;
  TH2* fhBmonHitMapEvoAll     = nullptr;
  TH2* fhBmonHitTotEvoAll     = nullptr;
  TH1* fhBmonChanHitMapAll    = nullptr;
  TH2* fhBmonChanHitMapEvoAll = nullptr;
  /// ---> No Pulser cut
  TH1* fhBmonCompMap       = nullptr;
  TH1* fhBmonChannelMap    = nullptr;
  TH2* fhBmonHitMapEvo     = nullptr;
  TH2* fhBmonHitTotEvo     = nullptr;
  TH1* fhBmonChanHitMap    = nullptr;
  TH2* fhBmonChanHitMapEvo = nullptr;
  /// ---> No Pulser cut + Spill detection
  std::vector<TH1*> fvhBmonCompMapSpill    = {};
  std::vector<TH1*> fvhBmonChannelMapSpill = {};
  TH1* fhBmonHitsPerSpill                  = nullptr;
  /// ---> Global Rate
  TH1* fhBmonMsgCntEvo            = nullptr;
  TH1* fhBmonHitCntEvo            = nullptr;
  TH1* fhBmonErrorCntEvo          = nullptr;
  TH1* fhBmonLostEvtCntEvo        = nullptr;
  TProfile* fhBmonErrorFractEvo   = nullptr;
  TProfile* fhBmonLostEvtFractEvo = nullptr;
  /// ---> Global Rate per MS
  TH2* fhBmonMsgCntPerMsEvo       = nullptr;
  TH2* fhBmonHitCntPerMsEvo       = nullptr;
  TH2* fhBmonErrorCntPerMsEvo     = nullptr;
  TH2* fhBmonLostEvtCntPerMsEvo   = nullptr;
  TH2* fhBmonErrorFractPerMsEvo   = nullptr;
  TH2* fhBmonLostEvtFractPerMsEvo = nullptr;
  /// ---> Pulser
  TH1* fhBmonChannelMapPulser = nullptr;
  TH2* fhBmonHitMapEvoPulser  = nullptr;

  /// BMon micro-spills monitoring histograms
  double_t fdBmonMicrospillsTsLengthSec = 0.128;  // 128 ms
  uint32_t fuBmonMicrospillsNbBinsTs    = 0;
  uint32_t fuNbTsMicrospills            = 0;
  std::unique_ptr<double[]> fArrHitCounts;
  std::unique_ptr<double[]> fArrErrCounts;
  TH1* fhBmonMicrospillsDistHits       = nullptr;  // Only internal, not for users
  TH1* fhBmonMicrospillsDistErrs       = nullptr;  // Only internal, not for users
  TH2* fhBmonMicrospillsTsBinCntHits   = nullptr;
  TH2* fhBmonMicrospillsTsBinCntErrs   = nullptr;
  TH1* fhBmonMicrospillsTsMeanHits     = nullptr;
  TH1* fhBmonMicrospillsTsMeanErrs     = nullptr;
  TH1* fhBmonMicrospillsTsMedianHits   = nullptr;
  TH1* fhBmonMicrospillsTsMedianErrs   = nullptr;
  TH2* fhBmonMicrospillsTsBinRatioHits = nullptr;
  TH2* fhBmonMicrospillsTsBinRatioErrs = nullptr;
  TH2* fhBmonMicrospillsTsBinFractHits = nullptr;
  TH2* fhBmonMicrospillsTsBinFractErrs = nullptr;

  /// BMon Q-Factors monitoring histograms
  /// Hint: keep fractions of TS size and under 100 us
  double_t fdTsSizeNs                        = fdBmonMicrospillsTsLengthSec * 1e9;
  uint32_t fuQFactorMaxNbTs                  = 400;
  std::vector<double_t> fvdQfactorBinSizesNs = {20, 200, 1.28e3, 10.24e3, 25.6e3, 102.4e3, 204.8e3};  // 7 values
  /// Hint: keep fractions of TS size + multiples of bin size and above 10 us
  std::vector<double_t> fvdQfactorIntegrationNs     = {2.56e6};  // 1 value
  std::vector<uint32_t> fvuNbHistoCyclesPerTS       = {};
  std::vector<uint32_t> fvuQfactorIdxHistoCycleinTS = {};
  /// Dimension: same as BinSizes vector!!
  std::vector<double_t> fvdQfactorHistMax = {2000., 400., 40., 20., 20., 20., 20.};
  uint16_t fuQfactorNbPlots               = 0;
  std::vector<std::vector<uint32_t>> fvuQfactorNbBinsHisto;
  std::vector<uint32_t> fvuQfactorNbHistoCyclesPerTS;
  std::vector<std::vector<TH1*>> fvhBmonQfactHisto;
  std::vector<std::vector<TH1*>> fvhBmonQfactQval;
  std::vector<std::vector<TH1*>> fvhBmonQfactMean;
  std::vector<TH1*> fvhBmonQfactBinCountDistribution;
  std::vector<TH2*> fvhBmonQfactBinCountDistributionEvo;
  /// Q-Factor helper methods
  double_t ExtractQFactor(TH1* pHistoIn);
  double_t ExtractMean(TH1* pHistoIn);


  /// Canvases
  /// ---> Generic
  TCanvas* fcSummaryGet4s          = nullptr;
  std::vector<TCanvas*> fvcSumComp = {};
  /// ---> BMon
  TCanvas* fcBmonSummary             = nullptr;
  TCanvas* fcBmonSummaryMap          = nullptr;
  TCanvas* fcBmonHitMaps             = nullptr;
  TCanvas* fcBmonGenCntsPerMs        = nullptr;
  TCanvas* fcBmonSpillCounts         = nullptr;
  TCanvas* fcBmonSpillCountsHori     = nullptr;
  TCanvas* fcBmonSpillCompCountsHori = nullptr;
  /// ---> BMon Microspills monitoring
  TCanvas* fcBmonMicrospillsBinCnts  = nullptr;
  TCanvas* fcBmonMicrospillsRatios   = nullptr;
  TCanvas* fcBmonMicrospillsFraction = nullptr;
  /// ---> BMon Q-Factors monitoring
  TCanvas* fcBmonQFactorVal        = nullptr;
  TCanvas* fcBmonQFactorMean       = nullptr;
  TCanvas* fcBmonQFactorBinCntDist = nullptr;

  /// Spill detection
  Bool_t fbSpillOn                      = kTRUE;
  UInt_t fuCurrentSpillIdx              = 0;
  UInt_t fuCurrentSpillPlot             = 0;
  Double_t fdStartTimeSpill             = -1.0;
  Double_t fdBmonLastInterTime          = -1.0;
  UInt_t fuBmonCountsLastInter          = 0;
  UInt_t fuBmonNonPulserCountsLastInter = 0;

  /// For monitoring of internal processes.
  /// => Pointers should be filled with TH1*, TH2*, TProfile*, ...
  /// ==> To check if object N is of type T, use "T ObjectPointer = dynamic_cast<T>( fvpAllHistoPointers[N].first );" and check for nullptr
  /// ==> To get back the original class name use "fvpAllHistoPointers[N].first->ClassName()" which returns a const char * (e.g. "TH1I")
  /// ===> Usage example with feeding a THttpServer:
  /// ===> #include "TH2.h"
  /// ===> std::string sClassName = vHistos[ uHisto ].first.ClassName();
  /// ===> if( !strncmp( sClassName, "TH1", 3 ) )
  /// ===>    server->Register( vHistos[ uHisto ].second.data(), dynamic_cast< TH1 * >(vHistos[ uHisto ].first) );
  /// ===> else if( !strncmp( sClassName, "TH2", 3 ) )
  /// ===>    server->Register( vHistos[ uHisto ].second.data(), dynamic_cast< TH2 * >(vHistos[ uHisto ].first) );
  std::vector<std::pair<TNamed*, std::string>>
    fvpAllHistoPointers;  //! Vector of pointers to histograms + optional folder name
  std::vector<std::pair<TCanvas*, std::string>>
    fvpAllCanvasPointers;  //! Vector of pointers to canvases + optional folder name

  CbmTofUnpackMonitor(const CbmTofUnpackMonitor&);
  CbmTofUnpackMonitor operator=(const CbmTofUnpackMonitor&);

  ClassDef(CbmTofUnpackMonitor, 1)
};

#endif
