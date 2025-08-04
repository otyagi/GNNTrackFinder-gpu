/* Copyright (C) 2019-2020 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

// -----------------------------------------------------------------------------
// -----                                                                   -----
// -----                  CbmMcbm2018UnpackerAlgoSts                       -----
// -----               Created 26.01.2019 by P.-A. Loizeau                 -----
// -----                                                                   -----
// -----------------------------------------------------------------------------

#ifndef CbmMcbm2018UnpackerAlgoSts_H
#define CbmMcbm2018UnpackerAlgoSts_H

#include "CbmStar2019Algo.h"

// Data
#include "CbmStsDigi.h"

#include "StsXyterFinalHit.h"
#include "StsXyterMessage.h"

// CbmRoot

// C++11
#include <chrono>

// C/C++
#include <map>
#include <vector>

class CbmMcbm2018StsPar;
/*
class TCanvas;
class THttpServer;
*/
class TH1;
class TH2;
class TProfile;
class TProfile2D;

class CbmMcbm2018UnpackerAlgoSts : public CbmStar2019Algo<CbmStsDigi> {
public:
  CbmMcbm2018UnpackerAlgoSts();
  ~CbmMcbm2018UnpackerAlgoSts();

  virtual Bool_t Init();
  virtual void Reset();
  virtual void Finish();

  Bool_t InitContainers();
  Bool_t ReInitContainers();
  TList* GetParList();

  Bool_t InitParameters();

  std::vector<CbmStsDigi>& GetPulserVector() { return fPulserDigiVect; }
  void ClearPulserVector() { fPulserDigiVect.clear(); }

  Bool_t ProcessTs(const fles::Timeslice& ts);
  Bool_t ProcessTs(const fles::Timeslice& ts, size_t /*component*/) { return ProcessTs(ts); }
  Bool_t ProcessMs(const fles::Timeslice& ts, size_t uMsCompIdx, size_t uMsIdx);

  void AddMsComponentToList(size_t component, UShort_t usDetectorId);

  Bool_t CreateHistograms();
  Bool_t FillHistograms();
  Bool_t ResetHistograms();

  inline void SetMonitorMode(Bool_t bFlagIn = kTRUE) { fbMonitorMode = bFlagIn; }
  inline void SetTimeOffsetNs(Double_t dOffsetIn = 0.0) { fdTimeOffsetNs = dOffsetIn; }
  void SetTimeOffsetNsAsic(UInt_t uAsicIdx, Double_t dOffsetIn = 0.0);
  void MaskNoisyChannel(UInt_t uFeb, UInt_t uChan, Bool_t bMasked = kTRUE);
  void SetAdcCut(UInt_t uAdc) { fdAdcCut = uAdc; }

  /// => Quick and dirty hack for binning FW!!!
  void SetBinningFwFlag(Bool_t bEnable = kTRUE) { fbBinningFw = bEnable; }

  inline void SetVectCapInc(Double_t dIncFact) { fdCapacityIncFactor = dIncFact; }

  void SeparatePulserOutput(Bool_t bFlagIn) { fbPulserOutput = bFlagIn; }

private:
  /// Control flags
  Bool_t fbMonitorMode;       //! Switch ON the filling of a minimal set of histograms
  Bool_t fbDebugMonitorMode;  //! Switch ON the filling of a additional set of histograms
  std::vector<Bool_t> fvbMaskedComponents;
  /// => Quick and dirty hack for binning FW!!!
  Bool_t fbBinningFw = kFALSE;
  Bool_t fbPulserOutput = kTRUE;  //! If ON a separate output vector of digi is used for the pulser

  /// Settings from parameter file
  CbmMcbm2018StsPar* fUnpackPar;  //!
                                  /// Readout chain dimensions and mapping
  UInt_t fuNbModules;             //! Total number of STS modules in the setup
  std::vector<Int_t>
    fviModuleType;  //! Type of each module: 0 for connectors on the right, 1 for connectors on the left
  std::vector<Int_t> fviModAddress;         //! STS address for the first strip of each module
  UInt_t fuNrOfDpbs;                        //! Total number of STS DPBs in system
  std::map<UInt_t, UInt_t> fDpbIdIndexMap;  //! Map of DPB Identifier to DPB index
  std::vector<std::vector<Bool_t>>
    fvbCrobActiveFlag;   //! Array to hold the active flag for all CROBs, [ NbDpb ][ NbCrobPerDpb ]
  UInt_t fuNbFebs;       //! Number of FEBs with StsXyter ASICs
  UInt_t fuNbStsXyters;  //! Number of StsXyter ASICs
  std::vector<std::vector<std::vector<Int_t>>>
    fviFebModuleIdx;  //! Idx of the STS module for each FEB, [ NbDpb ][ NbCrobPerDpb ][ NbFebsPerCrob ], -1 if inactive
  std::vector<std::vector<std::vector<Int_t>>>
    fviFebModuleSide;  //! STS module side for each FEB, [ NbDpb ][ NbCrobPerDpb ][ NbFebsPerCrob ], 0 = P, 1 = N, -1 if inactive
  std::vector<std::vector<std::vector<Int_t>>>
    fviFebType;  //! FEB type, [ NbDpb ][ NbCrobPerDpb ][ NbFebsPerCrob ], 0 = A, 1 = B, -1 if inactive
  std::vector<bool> fvbFebPulser = {};  //! Pulser flag for each FEB, [ NbDpb * NbCrobPerDpb * NbFebsPerCrob ]
  std::vector<Int_t> fviFebAddress;     //! STS address for each FEB, [ NbDpb * NbCrobPerDpb * NbFebsPerCrob ]
  std::vector<Int_t> fviFebSide;        //! Module side for each FEB, [ NbDpb * NbCrobPerDpb * NbFebsPerCrob ]
  std::vector<Double_t> fvdFebAdcGain;  //! ADC gain in e-/b, [ NbDpb * NbCrobPerDpb * NbFebsPerCrob ]
  std::vector<Double_t> fvdFebAdcOffs;  //! ADC offset in e-, [ NbDpb * NbCrobPerDpb * NbFebsPerCrob ]

  /// User settings: Data correction parameters
  Double_t fdTimeOffsetNs;
  std::vector<Double_t> fvdTimeOffsetNsAsics;
  Bool_t fbUseChannelMask;
  std::vector<std::vector<bool>>
    fvvbMaskedChannels;  //! Vector of channel masks, [ NbFeb ][ NbCHanInFeb ], used only if fbUseChannelMask is true
  UInt_t fdAdcCut;

  /// Constants
  static const Int_t kiMaxNbFlibLinks = 32;

  /// Running indices
  /// TS/MS info
  ULong64_t fulCurrentTsIdx;
  ULong64_t fulCurrentMsIdx;
  Double_t fdTsStartTime;     //! Time in ns of current TS from the index of the first MS first component
  Double_t fdTsStopTimeCore;  //! End Time in ns of current TS Core from the index of the first MS first component
  Double_t fdMsTime;          //! Start Time in ns of current MS from its index field in header
  UInt_t fuMsIndex;           //! Index of current MS within the TS
                              /// Current data properties
  std::map<stsxyter::MessType, UInt_t> fmMsgCounter;
  UInt_t fuCurrentEquipmentId;              //! Current equipment ID, tells from which DPB the current MS is originating
  UInt_t fuCurrDpbId;                       //! Temp holder until Current equipment ID is properly filled in MS
  UInt_t fuCurrDpbIdx;                      //! Index of the DPB from which the MS currently unpacked is coming
  Int_t fiRunStartDateTimeSec;              //! Start of run time since "epoch" in s, for the plots with date as X axis
  Int_t fiBinSizeDatePlots;                 //! Bin size in s for the plots with date as X axis
                                            /// Data format control
  std::vector<ULong64_t> fvulCurrentTsMsb;  //! Current TS MSB for each DPB
  std::vector<UInt_t> fvuCurrentTsMsbCycle;  //! Current TS MSB cycle for DPB
                                             /// Starting state book-keeping
  Double_t fdStartTime;     /** Time of first valid hit (TS_MSB available), used as reference for evolution plots**/
  Double_t fdStartTimeMsSz; /** Time of first microslice, used as reference for evolution plots**/
  std::chrono::steady_clock::time_point
    ftStartTimeUnix; /** Time of run Start from UNIX system, used as reference for long evolution plots against reception time **/

  /// Hits time-sorting
  std::vector<stsxyter::FinalHit>
    fvmHitsInMs;  //! All hits (time in bins, ADC in bins, asic, channel) in last MS, sorted with "<" operator

  /// Pulser Digis output, needed as STS reco does not accept Digis without a valid STS address
  std::vector<CbmStsDigi> fPulserDigiVect = {};

  /// Duplicate hits suppression
  static const UInt_t kuMaxTsMsbDiffDuplicates = 8;
  std::vector<std::vector<UShort_t>> fvvusLastTsChan;  //! TS of last hit message for each channel, [ AsicIdx ][ Chan ]
  std::vector<std::vector<UShort_t>>
    fvvusLastAdcChan;  //! ADC of last hit message for each channel, [ AsicIdx ][ Chan ]
  std::vector<std::vector<UShort_t>>
    fvvusLastTsMsbChan;  //! TS MSB of last hit message for each channel, [ AsicIdx ][ Chan ]
  std::vector<std::vector<UShort_t>>
    fvvusLastTsMsbCycleChan;  //! TS MSB cycle of last hit message for each channel, [ AsicIdx ][ Chan ]

  /// Histograms
  TH1* fhDigisTimeInRun;  //!
                          /*
      std::vector< TH1* > fvhHitsTimeToTriggerRaw;       //! [sector]
      std::vector< TH1* > fvhMessDistributionInMs;       //! [sector], extra monitor for debug
      TH1 *               fhEventNbPerTs;                //!
      TCanvas *           fcTimeToTrigRaw;               //! All sectors
*/
  TH1* fhVectorSize            = nullptr;
  TH1* fhVectorCapacity        = nullptr;
  size_t fuTsMaxVectorSize     = 0;
  Double_t fdCapacityIncFactor = 1.1;


  TH1* fhMsCntEvo    = nullptr;
  TH2* fhMsErrorsEvo = nullptr;

  /*
	TProfile2D* fhPulserVsTsAB   = nullptr;
      UShort_t fusTsA = 0.0;
      Double_t fdTimeA = 0.0;
*/
  void ProcessHitInfo(const stsxyter::Message& mess, const UShort_t& usElinkIdx, const UInt_t& uAsicIdx,
                      const UInt_t& uMsIdx);
  void ProcessTsMsbInfo(const stsxyter::Message& mess, UInt_t uMessIdx = 0, UInt_t uMsIdx = 0);
  void ProcessEpochInfo(const stsxyter::Message& mess);
  void ProcessStatusInfo(const stsxyter::Message& mess, const UInt_t& uAsicIdx);

  CbmMcbm2018UnpackerAlgoSts(const CbmMcbm2018UnpackerAlgoSts&);
  CbmMcbm2018UnpackerAlgoSts operator=(const CbmMcbm2018UnpackerAlgoSts&);

  ClassDef(CbmMcbm2018UnpackerAlgoSts, 1)
};

#endif
